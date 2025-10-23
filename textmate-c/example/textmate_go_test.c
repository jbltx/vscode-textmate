/*---------------------------------------------------------
 * Go grammar test for vscode-textmate C API
 * Tests parsing of a real, complex production grammar
 *--------------------------------------------------------*/

#include "vscode_textmate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read grammar file */
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

/* Print detailed token information */
void print_detailed_tokens(vtm_tokenize_result_t *result, const char *line) {
    printf("  Tokens: %u\n", result->token_count);
    for (uint32_t i = 0; i < result->token_count; i++) {
        vtm_token_t *token = &result->tokens[i];

        /* Extract token text */
        uint32_t len = token->end_index - token->start_index;
        char *token_text = (char*)malloc(len + 1);
        memcpy(token_text, line + token->start_index, len);
        token_text[len] = '\0';

        printf("    [%u-%u] \"%s\"\n",
               token->start_index, token->end_index, token_text);

        /* Print scopes */
        printf("      Scopes: ");
        for (uint32_t j = 0; j < token->scope_count; j++) {
            printf("%s", token->scopes[j]);
            if (j < token->scope_count - 1) {
                printf(" > ");
            }
        }
        printf("\n");

        free(token_text);
    }
}

int main(int argc, char **argv) {
    printf("=== Go Grammar Parser Test ===\n\n");

    /* Grammar file path */
    const char *grammar_path =
        "/Users/mickael.bonfill/dev/unity-app-ui/upm-packages/"
        "com.unity.dt.app-ui/PackageResources/Grammars/go.tmLanguage.json";

    /* Initialize oniguruma */
    int r = onig_init();
    if (r != ONIG_NORMAL) {
        fprintf(stderr, "Failed to initialize oniguruma\n");
        return 1;
    }

    printf("✓ Oniguruma initialized\n");

    /* Read grammar file */
    printf("Reading Go grammar from: %s\n", grammar_path);
    char *grammar_json = read_file(grammar_path);
    if (!grammar_json) {
        fprintf(stderr, "Failed to read grammar file\n");
        onig_end();
        return 1;
    }

    printf("✓ Grammar file loaded (%zu bytes)\n", strlen(grammar_json));

    /* Create registry */
    vtm_registry_t *registry = vtm_registry_create();
    if (!registry) {
        fprintf(stderr, "Failed to create registry\n");
        free(grammar_json);
        onig_end();
        return 1;
    }

    printf("✓ Registry created\n");

    /* Add Go grammar */
    printf("\nParsing Go grammar...\n");
    vtm_error_t err = vtm_registry_add_grammar_json(registry, "source.go", grammar_json);
    free(grammar_json);  /* No longer needed */

    if (err != VTM_OK) {
        fprintf(stderr, "✗ Failed to parse grammar: error code %d\n", err);
        vtm_registry_destroy(registry);
        onig_end();
        return 1;
    }

    printf("✓ Grammar parsed successfully!\n");

    /* Get the grammar */
    vtm_grammar_t *grammar = vtm_registry_get_grammar(registry, "source.go");
    if (!grammar) {
        fprintf(stderr, "✗ Failed to get grammar\n");
        vtm_registry_destroy(registry);
        onig_end();
        return 1;
    }

    printf("✓ Grammar retrieved (contains %u rules)\n\n",
           vtm_grammar_get_rule_count(grammar));

    /* Test Go code snippet */
    printf("=== Tokenizing Go Code ===\n\n");

    const char *go_code[] = {
        "package main",
        "",
        "import (",
        "    \"fmt\"",
        "    \"strings\"",
        ")",
        "",
        "// Calculate factorial",
        "func factorial(n int) int {",
        "    if n <= 1 {",
        "        return 1",
        "    }",
        "    return n * factorial(n-1)",
        "}",
        "",
        "func main() {",
        "    message := \"Hello, Go!\"",
        "    numbers := []int{1, 2, 3, 4, 5}",
        "    ",
        "    for i, num := range numbers {",
        "        fmt.Printf(\"Index %d: %d\\n\", i, num)",
        "    }",
        "    ",
        "    fmt.Println(strings.ToUpper(message))",
        "    fmt.Printf(\"Factorial of 5 is %d\\n\", factorial(5))",
        "}"
    };
    int num_lines = sizeof(go_code) / sizeof(go_code[0]);

    vtm_state_stack_t *state = vtm_state_stack_initial();
    int line_num = 1;

    for (int i = 0; i < num_lines; i++) {
        const char *line = go_code[i];

        if (strlen(line) == 0) {
            /* Skip empty lines in output */
            line_num++;
            continue;
        }

        printf("Line %d: %s\n", line_num, line);

        /* Tokenize */
        vtm_tokenize_result_t *result = vtm_grammar_tokenize_line(
            grammar,
            line,
            state,
            0  /* no time limit */
        );

        if (result) {
            print_detailed_tokens(result, line);

            /* Update state for next line */
            if (state != vtm_state_stack_initial()) {
                vtm_state_stack_destroy(state);
            }
            state = result->rule_stack;
            result->rule_stack = NULL;

            vtm_tokenize_result_destroy(result);
        } else {
            fprintf(stderr, "  ✗ Tokenization failed\n");
        }

        printf("\n");
        line_num++;
    }

    /* Print statistics */
    printf("=== Statistics ===\n");
    printf("Grammar rules: %u\n", vtm_grammar_get_rule_count(grammar));
    printf("Root rule ID: %d\n", vtm_grammar_get_root_rule_id(grammar));
    printf("Lines tokenized: %d\n", num_lines);

    /* Cleanup */
    if (state != vtm_state_stack_initial()) {
        vtm_state_stack_destroy(state);
    }
    vtm_registry_destroy(registry);
    onig_end();

    printf("\n✓ Test complete!\n");
    return 0;
}
