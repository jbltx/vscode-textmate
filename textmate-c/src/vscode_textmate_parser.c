/*---------------------------------------------------------
 * Grammar parser using cJSON
 * Based on the TypeScript implementation in vscode-textmate
 *--------------------------------------------------------*/

#include "vscode_textmate.h"
#include "vscode_textmate_internal.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations */
static vtm_rule_t* vtm_parse_rule_from_json(
    cJSON *json_rule,
    vtm_grammar_t *grammar,
    cJSON *repository
);
static vtm_rule_t* vtm_parse_repository_rule(
    const char *rule_name,
    vtm_grammar_t *grammar,
    cJSON *repository
);
static vtm_capture_rule_t** vtm_parse_captures_from_json(
    cJSON *json_captures,
    vtm_grammar_t *grammar,
    cJSON *repository,
    uint32_t *count
);
static int32_t* vtm_parse_patterns_from_json(
    cJSON *json_patterns,
    vtm_grammar_t *grammar,
    cJSON *repository,
    uint32_t *count
);
static char* vtm_get_string_safe(cJSON *obj, const char *key);

/* Helper: safely get string from JSON object */
static char* vtm_get_string_safe(cJSON *obj, const char *key) {
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsString(item) && item->valuestring) {
        return strdup(item->valuestring);
    }
    return NULL;
}

/* Helper: compile regex pattern */
static regex_t* vtm_compile_regex(const char *pattern) {
    if (!pattern) {
        return NULL;
    }

    regex_t *regex = NULL;
    OnigErrorInfo err_info;

    int r = onig_new(
        &regex,
        (const OnigUChar*)pattern,
        (const OnigUChar*)(pattern + strlen(pattern)),
        ONIG_OPTION_NONE,
        ONIG_ENCODING_UTF8,
        ONIG_SYNTAX_DEFAULT,
        &err_info
    );

    if (r != ONIG_NORMAL) {
        char err_buf[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str((OnigUChar*)err_buf, r, &err_info);
        fprintf(stderr, "Regex compilation error: %s (pattern: %s)\n", err_buf, pattern);
        return NULL;
    }

    return regex;
}

/* Parse captures from JSON */
static vtm_capture_rule_t** vtm_parse_captures_from_json(
    cJSON *json_captures,
    vtm_grammar_t *grammar,
    cJSON *repository,
    uint32_t *count
) {
    *count = 0;

    if (!json_captures || !cJSON_IsObject(json_captures)) {
        return NULL;
    }

    /* Find maximum capture ID */
    uint32_t max_capture_id = 0;
    cJSON *capture = NULL;
    cJSON_ArrayForEach(capture, json_captures) {
        if (capture->string) {
            int capture_id = atoi(capture->string);
            if (capture_id > (int)max_capture_id) {
                max_capture_id = capture_id;
            }
        }
    }

    if (max_capture_id == 0) {
        return NULL;
    }

    /* Allocate array */
    vtm_capture_rule_t **captures = (vtm_capture_rule_t**)calloc(
        max_capture_id + 1,
        sizeof(vtm_capture_rule_t*)
    );

    if (!captures) {
        return NULL;
    }

    *count = max_capture_id + 1;

    /* Fill capture rules */
    cJSON_ArrayForEach(capture, json_captures) {
        if (!capture->string) {
            continue;
        }

        int capture_id = atoi(capture->string);
        if (capture_id < 0 || capture_id > (int)max_capture_id) {
            continue;
        }

        vtm_capture_rule_t *capture_rule = (vtm_capture_rule_t*)calloc(
            1,
            sizeof(vtm_capture_rule_t)
        );

        if (capture_rule) {
            capture_rule->name = vtm_get_string_safe(capture, "name");
            capture_rule->content_name = vtm_get_string_safe(capture, "contentName");

            /* Check if capture has patterns (retokenize) */
            cJSON *patterns = cJSON_GetObjectItem(capture, "patterns");
            if (patterns) {
                /* Create a temporary rule for retokenization */
                vtm_rule_t *retok_rule = vtm_parse_rule_from_json(
                    capture,
                    grammar,
                    repository
                );
                if (retok_rule) {
                    capture_rule->retokenize_captured_with_rule_id = retok_rule->id;
                } else {
                    capture_rule->retokenize_captured_with_rule_id = 0;
                }
            } else {
                capture_rule->retokenize_captured_with_rule_id = 0;
            }

            captures[capture_id] = capture_rule;
        }
    }

    return captures;
}

/* Parse patterns array from JSON */
static int32_t* vtm_parse_patterns_from_json(
    cJSON *json_patterns,
    vtm_grammar_t *grammar,
    cJSON *repository,
    uint32_t *count
) {
    *count = 0;

    if (!json_patterns || !cJSON_IsArray(json_patterns)) {
        return NULL;
    }

    uint32_t pattern_count = cJSON_GetArraySize(json_patterns);
    if (pattern_count == 0) {
        return NULL;
    }

    int32_t *rule_ids = (int32_t*)malloc(pattern_count * sizeof(int32_t));
    if (!rule_ids) {
        return NULL;
    }

    uint32_t actual_count = 0;
    for (uint32_t i = 0; i < pattern_count; i++) {
        cJSON *pattern = cJSON_GetArrayItem(json_patterns, i);
        if (!pattern) {
            continue;
        }

        /* Check for include */
        cJSON *include = cJSON_GetObjectItem(pattern, "include");
        if (include && cJSON_IsString(include)) {
            const char *include_str = include->valuestring;

            /* Handle different include types */
            if (strcmp(include_str, "$self") == 0 || strcmp(include_str, "$base") == 0) {
                /* Self/base reference - handled later */
                /* For now, skip */
                continue;
            } else if (include_str[0] == '#') {
                /* Repository reference */
                const char *rule_name = include_str + 1;
                vtm_rule_t *rule = vtm_parse_repository_rule(rule_name, grammar, repository);
                if (rule) {
                    rule_ids[actual_count++] = rule->id;
                    free(rule);  /* Free the wrapper, not the actual rule */
                }
            } else {
                /* External grammar reference - not yet supported */
                continue;
            }
        } else {
            /* Direct pattern */
            vtm_rule_t *rule = vtm_parse_rule_from_json(
                pattern,
                grammar,
                repository
            );
            if (rule) {
                rule_ids[actual_count++] = rule->id;
            }
        }
    }

    *count = actual_count;

    if (actual_count == 0) {
        free(rule_ids);
        return NULL;
    }

    return rule_ids;
}

/* Parse a repository rule with caching */
static vtm_rule_t* vtm_parse_repository_rule(
    const char *rule_name,
    vtm_grammar_t *grammar,
    cJSON *repository
) {
    if (!rule_name || !grammar || !repository) {
        return NULL;
    }

    /* Check cache first */
    int32_t cached_id = vtm_repository_cache_find(grammar->repo_cache, rule_name);
    if (cached_id == -2) {
        /* -2 means currently being parsed (recursive reference) - skip to avoid infinite loop */
        fprintf(stderr, "Debug:     Skipping recursive reference to: %s\n", rule_name);
        return NULL;
    } else if (cached_id >= 0) {
        fprintf(stderr, "Debug:     Using cached repository rule: %s (id=%d)\n", rule_name, cached_id);
        /* Return a pseudo-rule with the cached ID */
        vtm_rule_t *cached_rule = (vtm_rule_t*)malloc(sizeof(vtm_rule_t));
        if (cached_rule) {
            cached_rule->id = cached_id;
            return cached_rule;
        }
        return NULL;
    }

    /* Not in cache, need to parse */
    cJSON *repo_rule = cJSON_GetObjectItem(repository, rule_name);
    if (!repo_rule) {
        fprintf(stderr, "Warning: Repository rule not found: %s\n", rule_name);
        return NULL;
    }

    /* Add a placeholder to cache BEFORE parsing to handle recursive references */
    /* Use -2 as placeholder for "parsing in progress" */
    vtm_repository_cache_add(grammar->repo_cache, rule_name, -2);

    fprintf(stderr, "Debug:     Parsing repository rule: %s\n", rule_name);
    vtm_rule_t *rule = vtm_parse_rule_from_json(repo_rule, grammar, repository);

    if (rule) {
        /* Update cache with actual rule ID */
        /* Find the cache entry and update it */
        for (uint32_t i = 0; i < grammar->repo_cache->count; i++) {
            if (strcmp(grammar->repo_cache->rule_names[i], rule_name) == 0) {
                grammar->repo_cache->rule_ids[i] = rule->id;
                fprintf(stderr, "Debug:     Cached repository rule: %s (id=%d)\n", rule_name, rule->id);
                break;
            }
        }
    }

    return rule;
}

/* Parse a single rule from JSON */
static vtm_rule_t* vtm_parse_rule_from_json(
    cJSON *json_rule,
    vtm_grammar_t *grammar,
    cJSON *repository
) {
    if (!json_rule) {
        return NULL;
    }

    /* Check for match rule */
    cJSON *match = cJSON_GetObjectItem(json_rule, "match");
    if (match && cJSON_IsString(match)) {
        /* Create match rule */
        vtm_rule_t *rule = vtm_rule_create(-1, VTM_RULE_TYPE_MATCH);
        if (!rule) {
            return NULL;
        }

        rule->data.match.name = vtm_get_string_safe(json_rule, "name");
        rule->data.match.match_pattern = strdup(match->valuestring);

        /* Compile regex */
        rule->data.match.match_regex = vtm_compile_regex(match->valuestring);

        /* Parse captures */
        cJSON *captures = cJSON_GetObjectItem(json_rule, "captures");
        rule->data.match.captures = vtm_parse_captures_from_json(
            captures,
            grammar,
            repository,
            &rule->data.match.capture_count
        );

        /* Add to grammar */
        int32_t rule_id = vtm_grammar_add_rule(grammar, rule);
        if (rule_id < 0) {
            vtm_rule_destroy(rule);
            return NULL;
        }

        fprintf(stderr, "Debug: Created MATCH rule #%d: %s\n", rule_id, rule->data.match.name ? rule->data.match.name : "(unnamed)");
        return rule;
    }

    /* Check for begin/end rule */
    cJSON *begin = cJSON_GetObjectItem(json_rule, "begin");
    cJSON *end = cJSON_GetObjectItem(json_rule, "end");
    cJSON *while_pattern = cJSON_GetObjectItem(json_rule, "while");

    if (begin && cJSON_IsString(begin)) {
        if (while_pattern && cJSON_IsString(while_pattern)) {
            /* Begin/While rule */
            vtm_rule_t *rule = vtm_rule_create(-1, VTM_RULE_TYPE_BEGIN_WHILE);
            if (!rule) {
                return NULL;
            }

            rule->data.begin_while.name = vtm_get_string_safe(json_rule, "name");
            rule->data.begin_while.content_name = vtm_get_string_safe(json_rule, "contentName");
            rule->data.begin_while.begin_pattern = strdup(begin->valuestring);
            rule->data.begin_while.while_pattern = strdup(while_pattern->valuestring);

            /* Compile regexes */
            rule->data.begin_while.begin_regex = vtm_compile_regex(begin->valuestring);
            rule->data.begin_while.while_regex = vtm_compile_regex(while_pattern->valuestring);

            /* Parse captures */
            cJSON *begin_captures = cJSON_GetObjectItem(json_rule, "beginCaptures");
            cJSON *captures = cJSON_GetObjectItem(json_rule, "captures");
            cJSON *while_captures = cJSON_GetObjectItem(json_rule, "whileCaptures");

            rule->data.begin_while.begin_captures = vtm_parse_captures_from_json(
                begin_captures ? begin_captures : captures,
                grammar,
                repository,
                &rule->data.begin_while.begin_capture_count
            );

            rule->data.begin_while.while_captures = vtm_parse_captures_from_json(
                while_captures ? while_captures : captures,
                grammar,
                repository,
                &rule->data.begin_while.while_capture_count
            );

            /* Parse patterns */
            cJSON *patterns = cJSON_GetObjectItem(json_rule, "patterns");
            rule->data.begin_while.patterns.data = vtm_parse_patterns_from_json(
                patterns,
                grammar,
                repository,
                &rule->data.begin_while.patterns.count
            );
            rule->data.begin_while.patterns.capacity = rule->data.begin_while.patterns.count;

            /* Add to grammar */
            int32_t rule_id = vtm_grammar_add_rule(grammar, rule);
            if (rule_id < 0) {
                vtm_rule_destroy(rule);
                return NULL;
            }

            fprintf(stderr, "Debug: Created BEGIN/WHILE rule #%d: %s\n", rule_id, rule->data.begin_while.name ? rule->data.begin_while.name : "(unnamed)");
            return rule;
        } else {
            /* Begin/End rule */
            vtm_rule_t *rule = vtm_rule_create(-1, VTM_RULE_TYPE_BEGIN_END);
            if (!rule) {
                return NULL;
            }

            rule->data.begin_end.name = vtm_get_string_safe(json_rule, "name");
            rule->data.begin_end.content_name = vtm_get_string_safe(json_rule, "contentName");
            rule->data.begin_end.begin_pattern = strdup(begin->valuestring);
            rule->data.begin_end.end_pattern = end && cJSON_IsString(end) ?
                strdup(end->valuestring) : strdup("\\uFFFF");

            /* Compile regexes */
            rule->data.begin_end.begin_regex = vtm_compile_regex(begin->valuestring);
            rule->data.begin_end.end_regex = vtm_compile_regex(rule->data.begin_end.end_pattern);

            /* Parse captures */
            cJSON *begin_captures = cJSON_GetObjectItem(json_rule, "beginCaptures");
            cJSON *end_captures = cJSON_GetObjectItem(json_rule, "endCaptures");
            cJSON *captures = cJSON_GetObjectItem(json_rule, "captures");

            rule->data.begin_end.begin_captures = vtm_parse_captures_from_json(
                begin_captures ? begin_captures : captures,
                grammar,
                repository,
                &rule->data.begin_end.begin_capture_count
            );

            rule->data.begin_end.end_captures = vtm_parse_captures_from_json(
                end_captures ? end_captures : captures,
                grammar,
                repository,
                &rule->data.begin_end.end_capture_count
            );

            /* Parse patterns */
            cJSON *patterns = cJSON_GetObjectItem(json_rule, "patterns");
            rule->data.begin_end.patterns.data = vtm_parse_patterns_from_json(
                patterns,
                grammar,
                repository,
                &rule->data.begin_end.patterns.count
            );
            rule->data.begin_end.patterns.capacity = rule->data.begin_end.patterns.count;

            /* Apply end pattern last? */
            cJSON *apply_end_last = cJSON_GetObjectItem(json_rule, "applyEndPatternLast");
            rule->data.begin_end.apply_end_pattern_last =
                apply_end_last && cJSON_IsTrue(apply_end_last);

            /* Add to grammar */
            int32_t rule_id = vtm_grammar_add_rule(grammar, rule);
            if (rule_id < 0) {
                vtm_rule_destroy(rule);
                return NULL;
            }

            fprintf(stderr, "Debug: Created BEGIN/END rule #%d: %s\n", rule_id, rule->data.begin_end.name ? rule->data.begin_end.name : "(unnamed)");
            return rule;
        }
    }

    /* If no match or begin, could be include-only rule (has patterns but no match/begin) */
    cJSON *patterns = cJSON_GetObjectItem(json_rule, "patterns");
    if (patterns && cJSON_IsArray(patterns)) {
        /* Parse all nested patterns recursively */
        uint32_t pattern_count = cJSON_GetArraySize(patterns);
        fprintf(stderr, "Debug: Processing patterns-only rule with %u patterns\n", pattern_count);
        for (uint32_t i = 0; i < pattern_count; i++) {
            cJSON *pattern = cJSON_GetArrayItem(patterns, i);
            if (pattern) {
                /* Check for include in nested pattern */
                cJSON *nested_include = cJSON_GetObjectItem(pattern, "include");
                if (nested_include && cJSON_IsString(nested_include)) {
                    const char *include_str = nested_include->valuestring;
                    fprintf(stderr, "Debug:   Pattern %u includes: %s\n", i, include_str);

                    /* Handle repository reference */
                    if (include_str[0] == '#' && repository) {
                        const char *rule_name = include_str + 1;
                        vtm_rule_t *rule = vtm_parse_repository_rule(rule_name, grammar, repository);
                        if (rule) {
                            free(rule);  /* Free the wrapper */
                        }
                    }
                } else {
                    /* Direct pattern rule */
                    vtm_parse_rule_from_json(pattern, grammar, repository);
                }
            }
        }
        /* Return NULL since this isn't a standalone rule, just a container */
        return NULL;
    }

    return NULL;
}

/* Main parsing function */
vtm_error_t vtm_parse_grammar_json(
    vtm_grammar_t *grammar,
    const char *json_string
) {
    if (!grammar || !json_string) {
        return VTM_ERROR_NULL_POINTER;
    }

    /* Parse JSON */
    cJSON *root = cJSON_Parse(json_string);
    if (!root) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr) {
            fprintf(stderr, "JSON parse error before: %s\n", error_ptr);
        }
        return VTM_ERROR_INVALID_GRAMMAR;
    }

    /* Get repository (may be NULL) */
    cJSON *repository = cJSON_GetObjectItem(root, "repository");

    /* Parse patterns array (top-level rules) */
    cJSON *patterns = cJSON_GetObjectItem(root, "patterns");
    if (!patterns || !cJSON_IsArray(patterns)) {
        cJSON_Delete(root);
        return VTM_ERROR_INVALID_GRAMMAR;
    }

    /* Create root rule that contains all patterns */
    vtm_rule_t *root_rule = vtm_rule_create(-1, VTM_RULE_TYPE_MATCH);
    if (!root_rule) {
        cJSON_Delete(root);
        return VTM_ERROR_OUT_OF_MEMORY;
    }

    root_rule->data.match.name = strdup(grammar->scope_name);
    root_rule->data.match.match_pattern = strdup(".*");
    root_rule->data.match.match_regex = vtm_compile_regex(".*");
    root_rule->data.match.captures = NULL;
    root_rule->data.match.capture_count = 0;

    grammar->root_rule_id = vtm_grammar_add_rule(grammar, root_rule);

    /* Parse all patterns (including repository includes) */
    uint32_t pattern_count = cJSON_GetArraySize(patterns);
    for (uint32_t i = 0; i < pattern_count; i++) {
        cJSON *pattern = cJSON_GetArrayItem(patterns, i);
        if (!pattern) {
            continue;
        }

        /* Check if this is an include reference */
        cJSON *include = cJSON_GetObjectItem(pattern, "include");
        if (include && cJSON_IsString(include)) {
            const char *include_str = include->valuestring;

            /* Handle repository reference */
            if (include_str[0] == '#' && repository) {
                const char *rule_name = include_str + 1;
                fprintf(stderr, "Debug: Parsing top-level repository include: %s\n", rule_name);
                vtm_rule_t *rule = vtm_parse_repository_rule(rule_name, grammar, repository);
                if (rule) {
                    free(rule);  /* Free the wrapper */
                }
            } else if (strcmp(include_str, "$self") == 0 || strcmp(include_str, "$base") == 0) {
                /* Self/base reference - skip for now */
                continue;
            } else {
                /* External grammar - not yet supported */
                fprintf(stderr, "Warning: External grammar include not yet supported: %s\n", include_str);
            }
        } else {
            /* Direct pattern */
            vtm_parse_rule_from_json(pattern, grammar, repository);
        }
    }

    cJSON_Delete(root);

    return VTM_OK;
}
