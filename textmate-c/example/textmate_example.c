/*---------------------------------------------------------
 * Example usage of vscode-textmate C API
 *--------------------------------------------------------*/

#include "vscode_textmate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple C grammar example */
const char *c_grammar = "{"
    "\"scopeName\": \"source.c\","
    "\"patterns\": ["
    "  {"
    "    \"match\": \"\\\\b(if|else|while|for|return)\\\\b\","
    "    \"name\": \"keyword.control.c\""
    "  },"
    "  {"
    "    \"match\": \"//.*$\","
    "    \"name\": \"comment.line.double-slash.c\""
    "  },"
    "  {"
    "    \"match\": \"\\\"[^\\\"]*\\\"\","
    "    \"name\": \"string.quoted.double.c\""
    "  }"
    "]"
"}";

void print_tokens(vtm_tokenize_result_t *result) {
    printf("Tokens: %u\n", result->token_count);
    for (uint32_t i = 0; i < result->token_count; i++) {
        vtm_token_t *token = &result->tokens[i];
        printf("  [%u-%u]: ", token->start_index, token->end_index);
        for (uint32_t j = 0; j < token->scope_count; j++) {
            printf("%s", token->scopes[j]);
            if (j < token->scope_count - 1) {
                printf(", ");
            }
        }
        printf("\n");
    }
    printf("Stopped early: %s\n\n", result->stopped_early ? "yes" : "no");
}

void print_tokens2(vtm_tokenize_result2_t *result) {
    printf("Tokens (binary format): %u\n", result->token_count);
    for (uint32_t i = 0; i < result->token_count; i++) {
        uint32_t start = result->tokens[i * 2];
        uint32_t metadata = result->tokens[i * 2 + 1];

        uint32_t lang_id = VTM_GET_LANGUAGEID(metadata);
        uint32_t token_type = VTM_GET_TOKEN_TYPE(metadata);
        uint32_t font_style = VTM_GET_FONT_STYLE(metadata);
        uint32_t foreground = VTM_GET_FOREGROUND(metadata);
        uint32_t background = VTM_GET_BACKGROUND(metadata);

        printf("  [%u]: lang=%u type=%u style=%u fg=%u bg=%u\n",
               start, lang_id, token_type, font_style, foreground, background);
    }
    printf("Stopped early: %s\n\n", result->stopped_early ? "yes" : "no");
}

int main(int argc, char **argv) {
    printf("=== vscode-textmate C API Example ===\n\n");

    /* Initialize oniguruma */
    int r = onig_init();
    if (r != ONIG_NORMAL) {
        fprintf(stderr, "Failed to initialize oniguruma\n");
        return 1;
    }

    /* Create a registry */
    vtm_registry_t *registry = vtm_registry_create();
    if (!registry) {
        fprintf(stderr, "Failed to create registry\n");
        onig_end();
        return 1;
    }

    printf("Registry created successfully\n");

    /* Add C grammar to registry */
    vtm_error_t err = vtm_registry_add_grammar_json(registry, "source.c", c_grammar);
    if (err != VTM_OK) {
        fprintf(stderr, "Failed to add grammar: %d\n", err);
        vtm_registry_destroy(registry);
        onig_end();
        return 1;
    }

    printf("Grammar added successfully\n\n");

    /* Get the grammar */
    vtm_grammar_t *grammar = vtm_registry_get_grammar(registry, "source.c");
    if (!grammar) {
        fprintf(stderr, "Failed to get grammar\n");
        vtm_registry_destroy(registry);
        onig_end();
        return 1;
    }

    /* Test tokenization */
    const char *test_lines[] = {
        "int main() {",
        "    // This is a comment",
        "    printf(\"Hello, world!\");",
        "    return 0;",
        "}"
    };
    int num_lines = sizeof(test_lines) / sizeof(test_lines[0]);

    printf("=== Tokenizing C code ===\n\n");

    vtm_state_stack_t *state = vtm_state_stack_initial();

    for (int i = 0; i < num_lines; i++) {
        printf("Line %d: %s\n", i + 1, test_lines[i]);

        /* Tokenize using tokenizeLine */
        vtm_tokenize_result_t *result = vtm_grammar_tokenize_line(
            grammar,
            test_lines[i],
            state,
            0  /* no time limit */
        );

        if (result) {
            print_tokens(result);

            /* Update state for next line */
            if (state != vtm_state_stack_initial()) {
                vtm_state_stack_destroy(state);
            }
            state = result->rule_stack;
            result->rule_stack = NULL;  /* Prevent double-free */

            vtm_tokenize_result_destroy(result);
        } else {
            fprintf(stderr, "Tokenization failed\n");
        }
    }

    printf("\n=== Tokenizing with binary format ===\n\n");

    /* Reset state */
    if (state != vtm_state_stack_initial()) {
        vtm_state_stack_destroy(state);
    }
    state = vtm_state_stack_initial();

    for (int i = 0; i < num_lines; i++) {
        printf("Line %d: %s\n", i + 1, test_lines[i]);

        /* Tokenize using tokenizeLine2 */
        vtm_tokenize_result2_t *result = vtm_grammar_tokenize_line2(
            grammar,
            test_lines[i],
            state,
            0
        );

        if (result) {
            print_tokens2(result);

            /* Update state for next line */
            if (state != vtm_state_stack_initial()) {
                vtm_state_stack_destroy(state);
            }
            state = result->rule_stack;
            result->rule_stack = NULL;

            vtm_tokenize_result2_destroy(result);
        }
    }

    /* Cleanup */
    if (state != vtm_state_stack_initial()) {
        vtm_state_stack_destroy(state);
    }
    vtm_registry_destroy(registry);
    onig_end();

    printf("\n=== Example complete ===\n");
    return 0;
}
