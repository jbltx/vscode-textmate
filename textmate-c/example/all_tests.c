/*---------------------------------------------------------
 * Comprehensive TextMate grammar test suite
 * Dynamically loads and runs all tests from tests.json
 *--------------------------------------------------------*/

#include "vscode_textmate.h"
#include "../cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Statistics */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int total_lines;
    int passed_lines;
    int failed_lines;
} test_stats_t;

/* Read file into memory */
static char* read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = (char*)malloc(fsize + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(content, 1, fsize, f);
    fclose(f);

    content[read_size] = '\0';
    return content;
}

/* Compare token scopes arrays */
static int compare_scopes(vtm_token_t *token, cJSON *expected_scopes) {
    int expected_count = cJSON_GetArraySize(expected_scopes);

    if (token->scope_count != expected_count) {
        return 0;
    }

    for (int i = 0; i < expected_count; i++) {
        cJSON *scope = cJSON_GetArrayItem(expected_scopes, i);
        const char *expected_scope = cJSON_GetStringValue(scope);

        if (!expected_scope || strcmp(token->scopes[i], expected_scope) != 0) {
            return 0;
        }
    }

    return 1;
}

/* Compare tokenization result with expected tokens */
static int compare_tokens(vtm_tokenize_result_t *result, const char *line, cJSON *expected_tokens) {
    int expected_count = cJSON_GetArraySize(expected_tokens);

    if (result->token_count != expected_count) {
        printf("      ✗ Token count mismatch: expected %d, got %u\n",
               expected_count, result->token_count);
        return 0;
    }

    for (int i = 0; i < expected_count; i++) {
        cJSON *expected_token = cJSON_GetArrayItem(expected_tokens, i);
        cJSON *expected_value = cJSON_GetObjectItem(expected_token, "value");
        cJSON *expected_scopes = cJSON_GetObjectItem(expected_token, "scopes");

        if (!expected_value || !expected_scopes) {
            printf("      ✗ Invalid expected token structure at index %d\n", i);
            return 0;
        }

        vtm_token_t *actual_token = &result->tokens[i];

        /* Extract actual token text */
        uint32_t len = actual_token->end_index - actual_token->start_index;
        char *token_text = (char*)malloc(len + 1);
        memcpy(token_text, line + actual_token->start_index, len);
        token_text[len] = '\0';

        const char *expected_text = cJSON_GetStringValue(expected_value);

        /* Compare value */
        if (strcmp(token_text, expected_text) != 0) {
            printf("      ✗ Token %d value mismatch:\n", i);
            printf("        Expected: \"%s\"\n", expected_text);
            printf("        Got:      \"%s\"\n", token_text);
            free(token_text);
            return 0;
        }

        /* Compare scopes */
        if (!compare_scopes(actual_token, expected_scopes)) {
            printf("      ✗ Token %d scopes mismatch for \"%s\"\n", i, token_text);
            printf("        Expected scopes: ");
            cJSON *scope;
            cJSON_ArrayForEach(scope, expected_scopes) {
                printf("%s ", cJSON_GetStringValue(scope));
            }
            printf("\n        Got scopes:      ");
            for (uint32_t j = 0; j < actual_token->scope_count; j++) {
                printf("%s ", actual_token->scopes[j]);
            }
            printf("\n");
            free(token_text);
            return 0;
        }

        free(token_text);
    }

    return 1;
}

/* Run a single test case */
static int run_test_case(cJSON *test_case, const char *test_dir, test_stats_t *stats) {
    stats->total_tests++;

    /* Extract test properties */
    cJSON *desc = cJSON_GetObjectItem(test_case, "desc");
    cJSON *grammar_path = cJSON_GetObjectItem(test_case, "grammarPath");
    cJSON *grammar_scope = cJSON_GetObjectItem(test_case, "grammarScopeName");
    cJSON *lines = cJSON_GetObjectItem(test_case, "lines");
    cJSON *grammars = cJSON_GetObjectItem(test_case, "grammars");
    cJSON *grammar_injections = cJSON_GetObjectItem(test_case, "grammarInjections");

    const char *test_desc = cJSON_GetStringValue(desc);
    if (!test_desc) test_desc = "Unknown test";

    printf("\n=== %s ===\n", test_desc);

    if (!lines || !cJSON_IsArray(lines)) {
        printf("✗ Invalid test case: missing or invalid 'lines' array\n");
        stats->failed_tests++;
        return 0;
    }

    if (!grammars || !cJSON_IsArray(grammars)) {
        printf("✗ Invalid test case: missing or invalid 'grammars' array\n");
        stats->failed_tests++;
        return 0;
    }

    /* Create registry for this test */
    vtm_registry_t *registry = vtm_registry_create();
    if (!registry) {
        printf("✗ Failed to create registry\n");
        stats->failed_tests++;
        return 0;
    }

    /* Load all dependency grammars */
    int loaded_grammars = 0;
    cJSON *grammar_file;
    cJSON_ArrayForEach(grammar_file, grammars) {
        const char *grammar_rel_path = cJSON_GetStringValue(grammar_file);
        if (!grammar_rel_path) continue;

        /* Build full path to grammar file */
        char grammar_full_path[1024];
        snprintf(grammar_full_path, sizeof(grammar_full_path),
                 "%s/%s", test_dir, grammar_rel_path);

        /* Read grammar JSON */
        char *grammar_json = read_file(grammar_full_path);
        if (!grammar_json) {
            printf("  Warning: Could not read grammar: %s\n", grammar_full_path);
            continue;
        }

        /* Parse to get scope name */
        cJSON *grammar_obj = cJSON_Parse(grammar_json);
        if (grammar_obj) {
            cJSON *scope_name = cJSON_GetObjectItem(grammar_obj, "scopeName");
            const char *scope_name_str = cJSON_GetStringValue(scope_name);

            if (scope_name_str) {
                vtm_error_t err = vtm_registry_add_grammar_json(registry, scope_name_str, grammar_json);
                if (err == VTM_OK) {
                    loaded_grammars++;
                } else {
                    printf("  Warning: Failed to add grammar %s (error %d)\n",
                           scope_name_str, err);
                }
            }

            cJSON_Delete(grammar_obj);
        }

        free(grammar_json);
    }

    /* Load grammar injections if specified */
    if (grammar_injections && cJSON_IsArray(grammar_injections)) {
        cJSON *injection;
        cJSON_ArrayForEach(injection, grammar_injections) {
            const char *injection_scope = cJSON_GetStringValue(injection);
            if (injection_scope) {
                /* Grammar should already be loaded in the dependency list */
                printf("  Note: Grammar injection: %s\n", injection_scope);
            }
        }
    }

    printf("  Loaded %d grammars\n", loaded_grammars);

    /* Determine which grammar to use for tokenization */
    const char *main_grammar_scope = NULL;

    if (grammar_scope) {
        main_grammar_scope = cJSON_GetStringValue(grammar_scope);
    } else if (grammar_path) {
        const char *grammar_rel_path = cJSON_GetStringValue(grammar_path);
        if (grammar_rel_path) {
            char grammar_full_path[1024];
            snprintf(grammar_full_path, sizeof(grammar_full_path),
                     "%s/%s", test_dir, grammar_rel_path);

            char *grammar_json = read_file(grammar_full_path);
            if (grammar_json) {
                cJSON *grammar_obj = cJSON_Parse(grammar_json);
                if (grammar_obj) {
                    cJSON *scope_name = cJSON_GetObjectItem(grammar_obj, "scopeName");
                    main_grammar_scope = cJSON_GetStringValue(scope_name);

                    if (main_grammar_scope) {
                        /* Make a copy since we'll free grammar_obj */
                        main_grammar_scope = strdup(main_grammar_scope);
                    }

                    cJSON_Delete(grammar_obj);
                }
                free(grammar_json);
            }
        }
    }

    if (!main_grammar_scope) {
        printf("✗ Could not determine main grammar scope\n");
        vtm_registry_destroy(registry);
        stats->failed_tests++;
        return 0;
    }

    /* Get the grammar */
    vtm_grammar_t *grammar = vtm_registry_get_grammar(registry, main_grammar_scope);
    if (!grammar) {
        printf("✗ Failed to get grammar: %s\n", main_grammar_scope);
        if (grammar_path) free((void*)main_grammar_scope);
        vtm_registry_destroy(registry);
        stats->failed_tests++;
        return 0;
    }

    printf("  Using grammar: %s\n", main_grammar_scope);

    /* Tokenize and compare each line */
    int test_passed = 1;
    vtm_state_stack_t *state = vtm_state_stack_initial();
    int line_num = 0;

    cJSON *line_obj;
    cJSON_ArrayForEach(line_obj, lines) {
        line_num++;
        stats->total_lines++;

        cJSON *line_text = cJSON_GetObjectItem(line_obj, "line");
        cJSON *expected_tokens = cJSON_GetObjectItem(line_obj, "tokens");

        if (!line_text || !expected_tokens) {
            printf("  Line %d: ✗ Invalid line structure\n", line_num);
            test_passed = 0;
            stats->failed_lines++;
            continue;
        }

        const char *line_str = cJSON_GetStringValue(line_text);

        /* Tokenize the line */
        vtm_tokenize_result_t *result = vtm_grammar_tokenize_line(
            grammar,
            line_str,
            state,
            0  /* no time limit */
        );

        if (!result) {
            printf("  Line %d: ✗ Tokenization failed\n", line_num);
            test_passed = 0;
            stats->failed_lines++;
            continue;
        }

        /* Compare with expected */
        int line_passed = compare_tokens(result, line_str, expected_tokens);

        if (line_passed) {
            printf("  Line %d: ✓\n", line_num);
            stats->passed_lines++;
        } else {
            printf("  Line %d: ✗ Token mismatch\n", line_num);
            printf("    Line: %s\n", line_str);
            test_passed = 0;
            stats->failed_lines++;
        }

        /* Update state for next line */
        if (state != vtm_state_stack_initial()) {
            vtm_state_stack_destroy(state);
        }
        state = result->rule_stack;
        result->rule_stack = NULL;

        vtm_tokenize_result_destroy(result);
    }

    /* Cleanup */
    if (state != vtm_state_stack_initial()) {
        vtm_state_stack_destroy(state);
    }

    if (grammar_path && main_grammar_scope) {
        free((void*)main_grammar_scope);
    }

    vtm_registry_destroy(registry);

    if (test_passed) {
        printf("✓ Test passed\n");
        stats->passed_tests++;
    } else {
        printf("✗ Test failed\n");
        stats->failed_tests++;
    }

    return test_passed;
}

int main(int argc, char **argv) {
    printf("=== VSCode TextMate Grammar Test Suite ===\n\n");

    /* Initialize oniguruma */
    int r = onig_init();
    if (r != ONIG_NORMAL) {
        fprintf(stderr, "Failed to initialize oniguruma\n");
        return 1;
    }

    printf("✓ Oniguruma initialized\n\n");

    /* Determine test directory */
    const char *test_dir = "test-cases/first-mate";
    const char *test_json_path = "test-cases/first-mate/tests.json";

    /* Read tests JSON file */
    printf("Reading test file: %s\n", test_json_path);
    char *tests_json = read_file(test_json_path);
    if (!tests_json) {
        fprintf(stderr, "Failed to read tests.json\n");
        onig_end();
        return 1;
    }

    printf("✓ Test file loaded (%zu bytes)\n", strlen(tests_json));

    /* Parse JSON */
    cJSON *tests_array = cJSON_Parse(tests_json);
    free(tests_json);

    if (!tests_array) {
        fprintf(stderr, "Failed to parse tests.json: %s\n", cJSON_GetErrorPtr());
        onig_end();
        return 1;
    }

    if (!cJSON_IsArray(tests_array)) {
        fprintf(stderr, "tests.json root is not an array\n");
        cJSON_Delete(tests_array);
        onig_end();
        return 1;
    }

    int num_tests = cJSON_GetArraySize(tests_array);
    printf("✓ Found %d test cases\n", num_tests);

    /* Run all tests */
    test_stats_t stats = {0};

    cJSON *test_case;
    cJSON_ArrayForEach(test_case, tests_array) {
        run_test_case(test_case, test_dir, &stats);
    }

    /* Print summary */
    printf("\n=== Test Summary ===\n");
    printf("Total tests:  %d\n", stats.total_tests);
    printf("Passed tests: %d\n", stats.passed_tests);
    printf("Failed tests: %d\n", stats.failed_tests);
    printf("Pass rate:    %.1f%%\n",
           stats.total_tests > 0 ? (100.0 * stats.passed_tests / stats.total_tests) : 0.0);
    printf("\n");
    printf("Total lines:  %d\n", stats.total_lines);
    printf("Passed lines: %d\n", stats.passed_lines);
    printf("Failed lines: %d\n", stats.failed_lines);
    printf("Line pass rate: %.1f%%\n",
           stats.total_lines > 0 ? (100.0 * stats.passed_lines / stats.total_lines) : 0.0);

    /* Cleanup */
    cJSON_Delete(tests_array);
    onig_end();

    printf("\n✓ Test suite complete!\n");

    return (stats.failed_tests == 0) ? 0 : 1;
}
