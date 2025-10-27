#ifndef VSCODE_TEXTMATE_C_API_H
#define VSCODE_TEXTMATE_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Platform-specific export/import macros
#ifdef _WIN32
    #ifdef TEXTMATE_EXPORTS
        #define TEXTMATE_API __declspec(dllexport)
    #else
        #define TEXTMATE_API __declspec(dllimport)
    #endif
#else
    #define TEXTMATE_API __attribute__((visibility("default")))
#endif

// Opaque handle types
typedef void* TextMateRegistry;
typedef void* TextMateGrammar;
typedef void* TextMateStateStack;
typedef void* TextMateOnigLib;

// Token structure for marshalling
typedef struct {
    int32_t startIndex;
    int32_t endIndex;
    int32_t scopeDepth;
    char** scopes;  // Array of scope strings
} TextMateToken;

// Tokenize result structure
typedef struct {
    TextMateToken* tokens;
    int32_t tokenCount;
    TextMateStateStack ruleStack;
    int32_t stoppedEarly;
} TextMateTokenizeResult;

// Tokenize result for encoded tokens (tokenizeLine2)
typedef struct {
    uint32_t* tokens;  // Encoded token array
    int32_t tokenCount;
    TextMateStateStack ruleStack;
    int32_t stoppedEarly;
} TextMateTokenizeResult2;

// Initialize Oniguruma library
TEXTMATE_API TextMateOnigLib textmate_oniglib_create();

// Create registry with Oniguruma library
TEXTMATE_API TextMateRegistry textmate_registry_create(TextMateOnigLib onigLib);

// Dispose registry
TEXTMATE_API void textmate_registry_dispose(TextMateRegistry registry);

// Add grammar to registry from JSON file (does not return Grammar, just registers it)
TEXTMATE_API int textmate_registry_add_grammar_from_file(
    TextMateRegistry registry,
    const char* grammarPath
);

// Add grammar to registry from JSON string (does not return Grammar, just registers it)
TEXTMATE_API int textmate_registry_add_grammar_from_json(
    TextMateRegistry registry,
    const char* jsonContent
);

// Set grammar injections for a scope (call before loading the grammar)
TEXTMATE_API void textmate_registry_set_injections(
    TextMateRegistry registry,
    const char* scopeName,
    const char** injections,
    int32_t injectionCount
);

// Load grammar by scope name (after grammars have been added to registry)
// This properly resolves dependencies and includes
TEXTMATE_API TextMateGrammar textmate_registry_load_grammar(
    TextMateRegistry registry,
    const char* scopeName
);

// Get INITIAL state
TEXTMATE_API TextMateStateStack textmate_get_initial_state();

// Tokenize a line of text
TEXTMATE_API TextMateTokenizeResult* textmate_tokenize_line(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
);

// Tokenize a line of text with encoded tokens
TEXTMATE_API TextMateTokenizeResult2* textmate_tokenize_line2(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
);

// Free tokenize result
TEXTMATE_API void textmate_free_tokenize_result(TextMateTokenizeResult* result);

// Free tokenize result2
TEXTMATE_API void textmate_free_tokenize_result2(TextMateTokenizeResult2* result);

// Get scope name from grammar
TEXTMATE_API const char* textmate_grammar_get_scope_name(TextMateGrammar grammar);

// Dispose grammar
TEXTMATE_API void textmate_grammar_dispose(TextMateGrammar grammar);

// Dispose Oniguruma library
TEXTMATE_API void textmate_oniglib_dispose(TextMateOnigLib onigLib);

#ifdef __cplusplus
}
#endif

#endif // VSCODE_TEXTMATE_C_API_H
