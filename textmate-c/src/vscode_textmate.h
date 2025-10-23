/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 * C Port for vscode-textmate integration with oniguruma
 *--------------------------------------------------------*/

#ifndef VSCODE_TEXTMATE_H
#define VSCODE_TEXTMATE_H

#include "oniguruma.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct vtm_registry vtm_registry_t;
typedef struct vtm_grammar vtm_grammar_t;
typedef struct vtm_state_stack vtm_state_stack_t;
typedef struct vtm_token vtm_token_t;
typedef struct vtm_tokenize_result vtm_tokenize_result_t;

/* Error codes */
typedef enum {
    VTM_OK = 0,
    VTM_ERROR_NULL_POINTER = -1,
    VTM_ERROR_INVALID_GRAMMAR = -2,
    VTM_ERROR_GRAMMAR_NOT_FOUND = -3,
    VTM_ERROR_OUT_OF_MEMORY = -4,
    VTM_ERROR_ONIG_ERROR = -5,
    VTM_ERROR_INVALID_STATE = -6
} vtm_error_t;

/* Token type constants */
typedef enum {
    VTM_TOKEN_TYPE_OTHER = 0,
    VTM_TOKEN_TYPE_COMMENT = 1,
    VTM_TOKEN_TYPE_STRING = 2,
    VTM_TOKEN_TYPE_REGEX = 3
} vtm_token_type_t;

/* Font style constants */
typedef enum {
    VTM_FONT_STYLE_NONE = 0,
    VTM_FONT_STYLE_ITALIC = 1,
    VTM_FONT_STYLE_BOLD = 2,
    VTM_FONT_STYLE_UNDERLINE = 4
} vtm_font_style_t;

/* Token structure */
struct vtm_token {
    uint32_t start_index;
    uint32_t end_index;
    char **scopes;
    uint32_t scope_count;
};

/* Tokenize result structure */
struct vtm_tokenize_result {
    vtm_token_t *tokens;
    uint32_t token_count;
    vtm_state_stack_t *rule_stack;
    bool stopped_early;
};

/* Tokenize result for binary format (tokenizeLine2) */
typedef struct {
    uint32_t *tokens;        /* Array of [startIndex, metadata] pairs */
    uint32_t token_count;     /* Number of tokens (length is token_count * 2) */
    vtm_state_stack_t *rule_stack;
    bool stopped_early;
} vtm_tokenize_result2_t;

/* Registry API */

/**
 * Create a new registry for managing grammars
 */
vtm_registry_t* vtm_registry_create(void);

/**
 * Destroy a registry and free all associated resources
 */
void vtm_registry_destroy(vtm_registry_t *registry);

/**
 * Add a grammar to the registry from JSON string
 * @param scope_name The scope name for this grammar (e.g., "source.c")
 * @param json_grammar The grammar in JSON format
 * @return VTM_OK on success, error code otherwise
 */
vtm_error_t vtm_registry_add_grammar_json(
    vtm_registry_t *registry,
    const char *scope_name,
    const char *json_grammar
);

/**
 * Get a grammar by its scope name
 * @return Grammar pointer or NULL if not found
 */
vtm_grammar_t* vtm_registry_get_grammar(
    vtm_registry_t *registry,
    const char *scope_name
);

/* Grammar API */

/**
 * Get the number of rules in a grammar
 */
uint32_t vtm_grammar_get_rule_count(vtm_grammar_t *grammar);

/**
 * Get the root rule ID of a grammar
 */
int32_t vtm_grammar_get_root_rule_id(vtm_grammar_t *grammar);

/**
 * Get the scope name of a grammar
 */
const char* vtm_grammar_get_scope_name(vtm_grammar_t *grammar);

/**
 * Tokenize a line of text
 * @param grammar The grammar to use for tokenization
 * @param line_text The text to tokenize
 * @param prev_state The state from the previous line (or NULL for first line)
 * @param time_limit Time limit in milliseconds (0 for no limit)
 * @return Tokenization result (must be freed with vtm_tokenize_result_destroy)
 */
vtm_tokenize_result_t* vtm_grammar_tokenize_line(
    vtm_grammar_t *grammar,
    const char *line_text,
    vtm_state_stack_t *prev_state,
    uint32_t time_limit
);

/**
 * Tokenize a line of text (binary format)
 * Returns tokens as packed uint32 arrays with encoded metadata
 */
vtm_tokenize_result2_t* vtm_grammar_tokenize_line2(
    vtm_grammar_t *grammar,
    const char *line_text,
    vtm_state_stack_t *prev_state,
    uint32_t time_limit
);

/* State Stack API */

/**
 * Get the initial state stack (for the first line)
 */
vtm_state_stack_t* vtm_state_stack_initial(void);

/**
 * Clone a state stack (creates an independent copy)
 */
vtm_state_stack_t* vtm_state_stack_clone(vtm_state_stack_t *stack);

/**
 * Check if two state stacks are equal
 */
bool vtm_state_stack_equals(vtm_state_stack_t *stack1, vtm_state_stack_t *stack2);

/**
 * Get the depth of a state stack
 */
uint32_t vtm_state_stack_depth(vtm_state_stack_t *stack);

/**
 * Destroy a state stack
 */
void vtm_state_stack_destroy(vtm_state_stack_t *stack);

/* Result cleanup functions */

/**
 * Free a tokenize result
 */
void vtm_tokenize_result_destroy(vtm_tokenize_result_t *result);

/**
 * Free a tokenize result2 (binary format)
 */
void vtm_tokenize_result2_destroy(vtm_tokenize_result2_t *result);

/* Metadata extraction macros for tokenizeLine2 results */
#define VTM_METADATA_LANGUAGEID_OFFSET 0
#define VTM_METADATA_TOKEN_TYPE_OFFSET 8
#define VTM_METADATA_FONT_STYLE_OFFSET 11
#define VTM_METADATA_FOREGROUND_OFFSET 15
#define VTM_METADATA_BACKGROUND_OFFSET 24

#define VTM_METADATA_LANGUAGEID_MASK 0x000000FF
#define VTM_METADATA_TOKEN_TYPE_MASK 0x00000700
#define VTM_METADATA_FONT_STYLE_MASK 0x00007800
#define VTM_METADATA_FOREGROUND_MASK 0x007F8000
#define VTM_METADATA_BACKGROUND_MASK 0xFF800000

/* Helper macros to extract metadata */
#define VTM_GET_LANGUAGEID(metadata) ((metadata & VTM_METADATA_LANGUAGEID_MASK) >> VTM_METADATA_LANGUAGEID_OFFSET)
#define VTM_GET_TOKEN_TYPE(metadata) ((metadata & VTM_METADATA_TOKEN_TYPE_MASK) >> VTM_METADATA_TOKEN_TYPE_OFFSET)
#define VTM_GET_FONT_STYLE(metadata) ((metadata & VTM_METADATA_FONT_STYLE_MASK) >> VTM_METADATA_FONT_STYLE_OFFSET)
#define VTM_GET_FOREGROUND(metadata) ((metadata & VTM_METADATA_FOREGROUND_MASK) >> VTM_METADATA_FOREGROUND_OFFSET)
#define VTM_GET_BACKGROUND(metadata) ((metadata & VTM_METADATA_BACKGROUND_MASK) >> VTM_METADATA_BACKGROUND_OFFSET)

#ifdef __cplusplus
}
#endif

#endif /* VSCODE_TEXTMATE_H */
