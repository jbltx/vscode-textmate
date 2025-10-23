/*---------------------------------------------------------
 * vscode-textmate C implementation
 *--------------------------------------------------------*/

#include "vscode_textmate.h"
#include "vscode_textmate_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ========================================================================
 * Registry Implementation
 * ======================================================================== */

vtm_registry_t* vtm_registry_create(void) {
    vtm_registry_t *registry = (vtm_registry_t*)malloc(sizeof(vtm_registry_t));
    if (!registry) {
        return NULL;
    }

    registry->grammar_capacity = 16;
    registry->grammars = (vtm_grammar_t**)calloc(registry->grammar_capacity, sizeof(vtm_grammar_t*));
    registry->scope_names = (char**)calloc(registry->grammar_capacity, sizeof(char*));
    registry->grammar_count = 0;

    if (!registry->grammars || !registry->scope_names) {
        free(registry->grammars);
        free(registry->scope_names);
        free(registry);
        return NULL;
    }

    return registry;
}

void vtm_registry_destroy(vtm_registry_t *registry) {
    if (!registry) {
        return;
    }

    for (uint32_t i = 0; i < registry->grammar_count; i++) {
        if (registry->grammars[i]) {
            /* Decrement ref count, will be freed when reaches 0 */
            if (registry->grammars[i]->ref_count > 0) {
                registry->grammars[i]->ref_count--;
            }
            if (registry->grammars[i]->ref_count == 0) {
                /* Free grammar */
                for (uint32_t j = 0; j < registry->grammars[i]->rule_count; j++) {
                    vtm_rule_destroy(registry->grammars[i]->rules[j]);
                }
                free(registry->grammars[i]->rules);
                free(registry->grammars[i]->scope_name);
                vtm_repository_cache_destroy(registry->grammars[i]->repo_cache);
                free(registry->grammars[i]);
            }
        }
        free(registry->scope_names[i]);
    }

    free(registry->grammars);
    free(registry->scope_names);
    free(registry);
}

vtm_error_t vtm_registry_add_grammar_json(
    vtm_registry_t *registry,
    const char *scope_name,
    const char *json_grammar
) {
    if (!registry || !scope_name || !json_grammar) {
        return VTM_ERROR_NULL_POINTER;
    }

    /* Check if we need to grow the arrays */
    if (registry->grammar_count >= registry->grammar_capacity) {
        uint32_t new_capacity = registry->grammar_capacity * 2;
        vtm_grammar_t **new_grammars = (vtm_grammar_t**)realloc(
            registry->grammars,
            new_capacity * sizeof(vtm_grammar_t*)
        );
        char **new_scope_names = (char**)realloc(
            registry->scope_names,
            new_capacity * sizeof(char*)
        );

        if (!new_grammars || !new_scope_names) {
            return VTM_ERROR_OUT_OF_MEMORY;
        }

        registry->grammars = new_grammars;
        registry->scope_names = new_scope_names;
        registry->grammar_capacity = new_capacity;
    }

    /* Create new grammar */
    vtm_grammar_t *grammar = (vtm_grammar_t*)malloc(sizeof(vtm_grammar_t));
    if (!grammar) {
        return VTM_ERROR_OUT_OF_MEMORY;
    }

    grammar->scope_name = strdup(scope_name);
    grammar->root_rule_id = -1;
    grammar->rule_capacity = 64;
    grammar->rule_count = 0;
    grammar->rules = (vtm_rule_t**)calloc(grammar->rule_capacity, sizeof(vtm_rule_t*));
    grammar->language_id = 0;
    grammar->ref_count = 1;
    grammar->repo_cache = vtm_repository_cache_create();

    if (!grammar->scope_name || !grammar->rules || !grammar->repo_cache) {
        free(grammar->scope_name);
        free(grammar->rules);
        vtm_repository_cache_destroy(grammar->repo_cache);
        free(grammar);
        return VTM_ERROR_OUT_OF_MEMORY;
    }

    /* Parse JSON and populate grammar rules */
    vtm_error_t parse_result = vtm_parse_grammar_json(grammar, json_grammar);
    if (parse_result != VTM_OK) {
        free(grammar->scope_name);
        free(grammar->rules);
        vtm_repository_cache_destroy(grammar->repo_cache);
        free(grammar);
        return parse_result;
    }

    /* Add to registry */
    registry->grammars[registry->grammar_count] = grammar;
    registry->scope_names[registry->grammar_count] = strdup(scope_name);
    registry->grammar_count++;

    return VTM_OK;
}

vtm_grammar_t* vtm_registry_get_grammar(
    vtm_registry_t *registry,
    const char *scope_name
) {
    if (!registry || !scope_name) {
        return NULL;
    }

    for (uint32_t i = 0; i < registry->grammar_count; i++) {
        if (strcmp(registry->scope_names[i], scope_name) == 0) {
            return registry->grammars[i];
        }
    }

    return NULL;
}

/* ========================================================================
 * State Stack Implementation
 * ======================================================================== */

/* Static NULL state for INITIAL */
static vtm_state_stack_t vtm_initial_state = {
    .parent = NULL,
    .rule_id = -1,
    .enter_pos = -1,
    .anchor_pos = -1,
    .name_scope_list = NULL,
    .content_name_scope_list = NULL,
    .depth = 0,
    .ref_count = 1000000  /* Large value to prevent deallocation */
};

vtm_state_stack_t* vtm_state_stack_initial(void) {
    return &vtm_initial_state;
}

vtm_state_stack_t* vtm_state_stack_create(
    vtm_state_stack_t *parent,
    int32_t rule_id,
    int32_t enter_pos,
    int32_t anchor_pos,
    vtm_scope_stack_t *name_scope_list,
    vtm_scope_stack_t *content_name_scope_list
) {
    vtm_state_stack_t *stack = (vtm_state_stack_t*)malloc(sizeof(vtm_state_stack_t));
    if (!stack) {
        return NULL;
    }

    stack->parent = parent;
    stack->rule_id = rule_id;
    stack->enter_pos = enter_pos;
    stack->anchor_pos = anchor_pos;
    stack->name_scope_list = name_scope_list;
    stack->content_name_scope_list = content_name_scope_list;
    stack->depth = parent ? parent->depth + 1 : 1;
    stack->ref_count = 1;

    /* Retain parent and scope stacks */
    if (parent) {
        vtm_state_stack_retain(parent);
    }
    if (name_scope_list) {
        vtm_scope_stack_retain(name_scope_list);
    }
    if (content_name_scope_list) {
        vtm_scope_stack_retain(content_name_scope_list);
    }

    return stack;
}

vtm_state_stack_t* vtm_state_stack_clone(vtm_state_stack_t *stack) {
    if (!stack) {
        return NULL;
    }

    /* Just increment ref count for immutable stacks */
    vtm_state_stack_retain(stack);
    return stack;
}

bool vtm_state_stack_equals(vtm_state_stack_t *stack1, vtm_state_stack_t *stack2) {
    /* Fast path: same pointer */
    if (stack1 == stack2) {
        return true;
    }

    if (!stack1 || !stack2) {
        return false;
    }

    /* Compare depths first */
    if (stack1->depth != stack2->depth) {
        return false;
    }

    /* Walk up both stacks */
    while (stack1 && stack2) {
        if (stack1->rule_id != stack2->rule_id) {
            return false;
        }
        stack1 = stack1->parent;
        stack2 = stack2->parent;
    }

    return stack1 == stack2;  /* Both should be NULL */
}

uint32_t vtm_state_stack_depth(vtm_state_stack_t *stack) {
    return stack ? stack->depth : 0;
}

void vtm_state_stack_retain(vtm_state_stack_t *stack) {
    if (stack && stack != &vtm_initial_state) {
        stack->ref_count++;
    }
}

void vtm_state_stack_release(vtm_state_stack_t *stack) {
    if (!stack || stack == &vtm_initial_state) {
        return;
    }

    stack->ref_count--;
    if (stack->ref_count == 0) {
        vtm_state_stack_t *parent = stack->parent;
        if (stack->name_scope_list) {
            vtm_scope_stack_release(stack->name_scope_list);
        }
        if (stack->content_name_scope_list) {
            vtm_scope_stack_release(stack->content_name_scope_list);
        }
        free(stack);
        if (parent) {
            vtm_state_stack_release(parent);
        }
    }
}

void vtm_state_stack_destroy(vtm_state_stack_t *stack) {
    vtm_state_stack_release(stack);
}

vtm_rule_t* vtm_state_stack_get_rule(vtm_state_stack_t *stack, vtm_grammar_t *grammar) {
    if (!stack || !grammar) {
        return NULL;
    }

    return vtm_grammar_get_rule(grammar, stack->rule_id);
}

/* ========================================================================
 * Scope Stack Implementation
 * ======================================================================== */

vtm_scope_stack_t* vtm_scope_stack_create(const char *scope_name, vtm_scope_stack_t *parent) {
    if (!scope_name) {
        return NULL;
    }

    vtm_scope_stack_t *stack = (vtm_scope_stack_t*)malloc(sizeof(vtm_scope_stack_t));
    if (!stack) {
        return NULL;
    }

    stack->scope_name = strdup(scope_name);
    stack->parent = parent;
    stack->ref_count = 1;

    if (!stack->scope_name) {
        free(stack);
        return NULL;
    }

    if (parent) {
        vtm_scope_stack_retain(parent);
    }

    return stack;
}

vtm_scope_stack_t* vtm_scope_stack_push(vtm_scope_stack_t *stack, const char *scope_name) {
    return vtm_scope_stack_create(scope_name, stack);
}

void vtm_scope_stack_retain(vtm_scope_stack_t *stack) {
    if (stack) {
        stack->ref_count++;
    }
}

void vtm_scope_stack_release(vtm_scope_stack_t *stack) {
    if (!stack) {
        return;
    }

    stack->ref_count--;
    if (stack->ref_count == 0) {
        vtm_scope_stack_t *parent = stack->parent;
        free(stack->scope_name);
        free(stack);
        if (parent) {
            vtm_scope_stack_release(parent);
        }
    }
}

/* ========================================================================
 * Grammar Implementation
 * ======================================================================== */

uint32_t vtm_grammar_get_rule_count(vtm_grammar_t *grammar) {
    return grammar ? grammar->rule_count : 0;
}

int32_t vtm_grammar_get_root_rule_id(vtm_grammar_t *grammar) {
    return grammar ? grammar->root_rule_id : -1;
}

const char* vtm_grammar_get_scope_name(vtm_grammar_t *grammar) {
    return grammar ? grammar->scope_name : NULL;
}

/* ========================================================================
 * Repository Cache Implementation (for deduplication)
 * ======================================================================== */

vtm_repository_cache_t* vtm_repository_cache_create(void) {
    vtm_repository_cache_t *cache = (vtm_repository_cache_t*)malloc(sizeof(vtm_repository_cache_t));
    if (!cache) {
        return NULL;
    }

    cache->capacity = 64;
    cache->count = 0;
    cache->rule_names = (char**)calloc(cache->capacity, sizeof(char*));
    cache->rule_ids = (int32_t*)calloc(cache->capacity, sizeof(int32_t));

    if (!cache->rule_names || !cache->rule_ids) {
        free(cache->rule_names);
        free(cache->rule_ids);
        free(cache);
        return NULL;
    }

    return cache;
}

void vtm_repository_cache_destroy(vtm_repository_cache_t *cache) {
    if (!cache) {
        return;
    }

    for (uint32_t i = 0; i < cache->count; i++) {
        free(cache->rule_names[i]);
    }
    free(cache->rule_names);
    free(cache->rule_ids);
    free(cache);
}

int32_t vtm_repository_cache_find(vtm_repository_cache_t *cache, const char *rule_name) {
    if (!cache || !rule_name) {
        return -1;
    }

    for (uint32_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->rule_names[i], rule_name) == 0) {
            return cache->rule_ids[i];
        }
    }

    return -1;
}

void vtm_repository_cache_add(vtm_repository_cache_t *cache, const char *rule_name, int32_t rule_id) {
    if (!cache || !rule_name) {
        return;
    }

    /* Grow array if needed */
    if (cache->count >= cache->capacity) {
        uint32_t new_capacity = cache->capacity * 2;
        char **new_names = (char**)realloc(cache->rule_names, new_capacity * sizeof(char*));
        int32_t *new_ids = (int32_t*)realloc(cache->rule_ids, new_capacity * sizeof(int32_t));

        if (!new_names || !new_ids) {
            return;
        }

        cache->rule_names = new_names;
        cache->rule_ids = new_ids;
        cache->capacity = new_capacity;
    }

    cache->rule_names[cache->count] = strdup(rule_name);
    cache->rule_ids[cache->count] = rule_id;
    cache->count++;
}

/* ========================================================================
 * Grammar Implementation
 * ======================================================================== */

int32_t vtm_grammar_add_rule(vtm_grammar_t *grammar, vtm_rule_t *rule) {
    if (!grammar || !rule) {
        return -1;
    }

    /* Grow array if needed */
    if (grammar->rule_count >= grammar->rule_capacity) {
        uint32_t new_capacity = grammar->rule_capacity * 2;
        vtm_rule_t **new_rules = (vtm_rule_t**)realloc(
            grammar->rules,
            new_capacity * sizeof(vtm_rule_t*)
        );
        if (!new_rules) {
            return -1;
        }
        grammar->rules = new_rules;
        grammar->rule_capacity = new_capacity;
    }

    grammar->rules[grammar->rule_count] = rule;
    rule->id = grammar->rule_count;
    grammar->rule_count++;

    return rule->id;
}

vtm_rule_t* vtm_grammar_get_rule(vtm_grammar_t *grammar, int32_t rule_id) {
    if (!grammar || rule_id < 0 || (uint32_t)rule_id >= grammar->rule_count) {
        return NULL;
    }

    return grammar->rules[rule_id];
}

/* ========================================================================
 * Tokenization Implementation
 * ======================================================================== */

vtm_tokenize_result_t* vtm_grammar_tokenize_line(
    vtm_grammar_t *grammar,
    const char *line_text,
    vtm_state_stack_t *prev_state,
    uint32_t time_limit
) {
    if (!grammar || !line_text) {
        return NULL;
    }

    if (!prev_state) {
        prev_state = vtm_state_stack_initial();
    }

    vtm_tokenize_result_t *result = (vtm_tokenize_result_t*)malloc(sizeof(vtm_tokenize_result_t));
    if (!result) {
        return NULL;
    }

    /* Allocate initial token array */
    uint32_t token_capacity = 32;
    result->tokens = (vtm_token_t*)calloc(token_capacity, sizeof(vtm_token_t));
    result->token_count = 0;
    result->rule_stack = vtm_state_stack_clone(prev_state);
    result->stopped_early = false;

    if (!result->tokens) {
        free(result);
        return NULL;
    }

    /* Simple tokenization - for now just create one token for the whole line */
    /* TODO: Implement full tokenization logic */

    uint32_t line_len = strlen(line_text);
    if (line_len > 0) {
        result->tokens[0].start_index = 0;
        result->tokens[0].end_index = line_len;
        result->tokens[0].scope_count = 1;
        result->tokens[0].scopes = (char**)malloc(sizeof(char*));
        result->tokens[0].scopes[0] = strdup(grammar->scope_name);
        result->token_count = 1;
    }

    return result;
}

vtm_tokenize_result2_t* vtm_grammar_tokenize_line2(
    vtm_grammar_t *grammar,
    const char *line_text,
    vtm_state_stack_t *prev_state,
    uint32_t time_limit
) {
    if (!grammar || !line_text) {
        return NULL;
    }

    if (!prev_state) {
        prev_state = vtm_state_stack_initial();
    }

    vtm_tokenize_result2_t *result = (vtm_tokenize_result2_t*)malloc(sizeof(vtm_tokenize_result2_t));
    if (!result) {
        return NULL;
    }

    /* Allocate initial token array (2 uint32s per token) */
    uint32_t token_capacity = 64;
    result->tokens = (uint32_t*)calloc(token_capacity, sizeof(uint32_t));
    result->token_count = 0;
    result->rule_stack = vtm_state_stack_clone(prev_state);
    result->stopped_early = false;

    if (!result->tokens) {
        free(result);
        return NULL;
    }

    /* Simple tokenization - one token for whole line */
    /* TODO: Implement full tokenization logic */

    uint32_t line_len = strlen(line_text);
    if (line_len > 0) {
        result->tokens[0] = 0;           /* start index */
        result->tokens[1] = 0;           /* metadata (default) */
        result->token_count = 1;
    }

    return result;
}

void vtm_tokenize_result_destroy(vtm_tokenize_result_t *result) {
    if (!result) {
        return;
    }

    /* Free tokens */
    for (uint32_t i = 0; i < result->token_count; i++) {
        for (uint32_t j = 0; j < result->tokens[i].scope_count; j++) {
            free(result->tokens[i].scopes[j]);
        }
        free(result->tokens[i].scopes);
    }
    free(result->tokens);

    /* Release state stack */
    if (result->rule_stack) {
        vtm_state_stack_release(result->rule_stack);
    }

    free(result);
}

void vtm_tokenize_result2_destroy(vtm_tokenize_result2_t *result) {
    if (!result) {
        return;
    }

    free(result->tokens);

    if (result->rule_stack) {
        vtm_state_stack_release(result->rule_stack);
    }

    free(result);
}

/* ========================================================================
 * Rule Implementation
 * ======================================================================== */

vtm_rule_t* vtm_rule_create(int32_t id, vtm_rule_type_t type) {
    vtm_rule_t *rule = (vtm_rule_t*)calloc(1, sizeof(vtm_rule_t));
    if (!rule) {
        return NULL;
    }

    rule->id = id;
    rule->type = type;

    return rule;
}

void vtm_rule_destroy(vtm_rule_t *rule) {
    if (!rule) {
        return;
    }

    switch (rule->type) {
        case VTM_RULE_TYPE_MATCH:
            free(rule->data.match.name);
            free(rule->data.match.match_pattern);
            if (rule->data.match.match_regex) {
                onig_free(rule->data.match.match_regex);
            }
            for (uint32_t i = 0; i < rule->data.match.capture_count; i++) {
                if (rule->data.match.captures[i]) {
                    free(rule->data.match.captures[i]->name);
                    free(rule->data.match.captures[i]->content_name);
                    free(rule->data.match.captures[i]);
                }
            }
            free(rule->data.match.captures);
            break;

        case VTM_RULE_TYPE_BEGIN_END:
            free(rule->data.begin_end.name);
            free(rule->data.begin_end.content_name);
            free(rule->data.begin_end.begin_pattern);
            free(rule->data.begin_end.end_pattern);
            if (rule->data.begin_end.begin_regex) {
                onig_free(rule->data.begin_end.begin_regex);
            }
            if (rule->data.begin_end.end_regex) {
                onig_free(rule->data.begin_end.end_regex);
            }
            free(rule->data.begin_end.patterns.data);
            /* Free captures... */
            break;

        /* Handle other rule types */
        default:
            break;
    }

    free(rule);
}
