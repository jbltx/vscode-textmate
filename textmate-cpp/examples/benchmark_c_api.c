#include "../src/c_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Simple file reader
char* read_file(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = (char*)malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';

    fclose(file);
    return content;
}

// Split lines
char** split_lines(const char* content, int* line_count) {
    int count = 1;
    for (const char* p = content; *p; p++) {
        if (*p == '\n') count++;
    }

    char** lines = (char**)malloc(count * sizeof(char*));
    int idx = 0;
    const char* start = content;

    for (const char* p = content; ; p++) {
        if (*p == '\n' || *p == '\0') {
            int len = p - start;
            lines[idx] = (char*)malloc(len + 1);
            memcpy(lines[idx], start, len);
            lines[idx][len] = '\0';
            idx++;

            if (*p == '\0') break;
            start = p + 1;
        }
    }

    *line_count = count;
    return lines;
}

int main() {
    printf("TextMate C API Benchmark\n");
    printf("========================\n\n");

    const char* test_file = "../../benchmark/large.js.txt";
    const char* grammar_file = "../../benchmark/JavaScript.tmLanguage.json";

    // Read files
    char* content = read_file(test_file);
    if (!content) {
        printf("Failed to read test file\n");
        return 1;
    }

    char* grammar_json = read_file(grammar_file);
    if (!grammar_json) {
        printf("Failed to read grammar file\n");
        return 1;
    }

    // Split lines
    int line_count = 0;
    char** lines = split_lines(content, &line_count);
    printf("File has %d lines\n\n", line_count);

    // Create library and registry
    TextMateOnigLib onigLib = textmate_oniglib_create();
    TextMateRegistry registry = textmate_registry_create(onigLib);

    // Add grammar
    textmate_registry_add_grammar_from_json(registry, grammar_json);

    // Load grammar
    TextMateGrammar grammar = textmate_registry_load_grammar(registry, "source.js");
    if (!grammar) {
        printf("Failed to load grammar\n");
        return 1;
    }

    // Warmup
    printf("Warmup run...\n");
    TextMateStateStack state = textmate_get_initial_state();
    for (int i = 0; i < line_count; i++) {
        TextMateTokenizeResult* result = textmate_tokenize_line(grammar, lines[i], state);
        state = result->ruleStack;
        textmate_free_tokenize_result(result);
    }

    // Benchmark
    printf("Benchmark run...\n");
    clock_t start = clock();

    state = textmate_get_initial_state();
    int total_tokens = 0;
    for (int i = 0; i < line_count; i++) {
        TextMateTokenizeResult* result = textmate_tokenize_line(grammar, lines[i], state);
        state = result->ruleStack;
        total_tokens += result->tokenCount;
        textmate_free_tokenize_result(result);
    }

    clock_t end = clock();
    double elapsed_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    printf("\n");
    printf("Results:\n");
    printf("  Lines:      %d\n", line_count);
    printf("  Tokens:     %d\n", total_tokens);
    printf("  Time:       %.0f ms\n", elapsed_ms);
    printf("  Lines/sec:  %.0f\n", line_count / (elapsed_ms / 1000.0));
    printf("  Tokens/sec: %.0f\n", total_tokens / (elapsed_ms / 1000.0));
    printf("\n");

    // Cleanup
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
    free(content);
    free(grammar_json);
    textmate_registry_dispose(registry);
    textmate_oniglib_dispose(onigLib);

    return 0;
}
