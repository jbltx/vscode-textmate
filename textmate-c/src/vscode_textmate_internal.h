/*---------------------------------------------------------
 * Internal structures for vscode-textmate C implementation
 *--------------------------------------------------------*/

#ifndef VSCODE_TEXTMATE_INTERNAL_H
#define VSCODE_TEXTMATE_INTERNAL_H

#include "vscode_textmate.h"
#include "oniguruma.h"
#include <stdint.h>
#include <stdbool.h>

/* Maximum sizes */
#define VTM_MAX_SCOPE_DEPTH 128
#define VTM_MAX_CAPTURES 32
#define VTM_MAX_PATTERNS 256
#define VTM_MAX_RULES 4096

/* Rule types */
typedef enum {
    VTM_RULE_TYPE_MATCH,
    VTM_RULE_TYPE_BEGIN_END,
    VTM_RULE_TYPE_BEGIN_WHILE,
    VTM_RULE_TYPE_INCLUDE_ONLY,
    VTM_RULE_TYPE_CAPTURE
} vtm_rule_type_t;

/* Rule ID constants */
#define VTM_END_RULE_ID -1
#define VTM_WHILE_RULE_ID -2

/* Forward declarations */
typedef struct vtm_rule vtm_rule_t;
typedef struct vtm_compiled_rule vtm_compiled_rule_t;
typedef struct vtm_scope_stack vtm_scope_stack_t;

/* String helper structure */
typedef struct {
    char *data;
    uint32_t length;
    uint32_t capacity;
} vtm_string_t;

/* Dynamic array for strings */
typedef struct {
    char **data;
    uint32_t count;
    uint32_t capacity;
} vtm_string_array_t;

/* Dynamic array for rule IDs */
typedef struct {
    int32_t *data;
    uint32_t count;
    uint32_t capacity;
} vtm_rule_id_array_t;

/* Scope stack - immutable linked list */
struct vtm_scope_stack {
    char *scope_name;
    struct vtm_scope_stack *parent;
    uint32_t ref_count;
};

/* State stack implementation */
struct vtm_state_stack {
    struct vtm_state_stack *parent;
    int32_t rule_id;
    int32_t enter_pos;
    int32_t anchor_pos;
    vtm_scope_stack_t *name_scope_list;
    vtm_scope_stack_t *content_name_scope_list;
    uint32_t depth;
    uint32_t ref_count;
};

/* Capture information */
typedef struct {
    int32_t start;
    int32_t end;
    int32_t length;
} vtm_capture_t;

/* Match result */
typedef struct {
    int32_t matched_rule_id;
    vtm_capture_t captures[VTM_MAX_CAPTURES];
    uint32_t capture_count;
} vtm_match_result_t;

/* Compiled rule structure */
struct vtm_compiled_rule {
    regex_t **regexes;
    uint32_t regex_count;
    int32_t *rule_ids;
};

/* Capture rule */
typedef struct {
    char *name;
    char *content_name;
    int32_t retokenize_captured_with_rule_id;
} vtm_capture_rule_t;

/* Match rule */
typedef struct {
    char *name;
    char *match_pattern;
    regex_t *match_regex;
    vtm_capture_rule_t **captures;
    uint32_t capture_count;
} vtm_match_rule_t;

/* Begin-End rule */
typedef struct {
    char *name;
    char *content_name;
    char *begin_pattern;
    char *end_pattern;
    regex_t *begin_regex;
    regex_t *end_regex;
    vtm_capture_rule_t **begin_captures;
    uint32_t begin_capture_count;
    vtm_capture_rule_t **end_captures;
    uint32_t end_capture_count;
    vtm_rule_id_array_t patterns;
    bool apply_end_pattern_last;
} vtm_begin_end_rule_t;

/* Begin-While rule */
typedef struct {
    char *name;
    char *content_name;
    char *begin_pattern;
    char *while_pattern;
    regex_t *begin_regex;
    regex_t *while_regex;
    vtm_capture_rule_t **begin_captures;
    uint32_t begin_capture_count;
    vtm_capture_rule_t **while_captures;
    uint32_t while_capture_count;
    vtm_rule_id_array_t patterns;
} vtm_begin_while_rule_t;

/* Generic rule structure */
struct vtm_rule {
    int32_t id;
    vtm_rule_type_t type;
    union {
        vtm_match_rule_t match;
        vtm_begin_end_rule_t begin_end;
        vtm_begin_while_rule_t begin_while;
        vtm_capture_rule_t capture;
    } data;
};

/* Repository cache for deduplication */
typedef struct {
    char **rule_names;        /* Names of parsed repository rules */
    int32_t *rule_ids;        /* Corresponding rule IDs */
    uint32_t count;           /* Number of cached rules */
    uint32_t capacity;        /* Capacity of arrays */
} vtm_repository_cache_t;

/* Grammar structure */
struct vtm_grammar {
    char *scope_name;
    int32_t root_rule_id;
    vtm_rule_t **rules;
    uint32_t rule_count;
    uint32_t rule_capacity;
    int32_t language_id;
    uint32_t ref_count;
    vtm_repository_cache_t *repo_cache;  /* Cache for repository rules */
};

/* Registry structure */
struct vtm_registry {
    vtm_grammar_t **grammars;
    char **scope_names;
    uint32_t grammar_count;
    uint32_t grammar_capacity;
};

/* Function prototypes for internal use */

/* String helpers */
vtm_string_t* vtm_string_create(const char *initial);
void vtm_string_destroy(vtm_string_t *str);
void vtm_string_append(vtm_string_t *str, const char *append);

/* String array helpers */
vtm_string_array_t* vtm_string_array_create(void);
void vtm_string_array_destroy(vtm_string_array_t *arr);
void vtm_string_array_push(vtm_string_array_t *arr, const char *str);

/* Scope stack helpers */
vtm_scope_stack_t* vtm_scope_stack_create(const char *scope_name, vtm_scope_stack_t *parent);
vtm_scope_stack_t* vtm_scope_stack_push(vtm_scope_stack_t *stack, const char *scope_name);
void vtm_scope_stack_retain(vtm_scope_stack_t *stack);
void vtm_scope_stack_release(vtm_scope_stack_t *stack);

/* State stack helpers */
vtm_state_stack_t* vtm_state_stack_create(
    vtm_state_stack_t *parent,
    int32_t rule_id,
    int32_t enter_pos,
    int32_t anchor_pos,
    vtm_scope_stack_t *name_scope_list,
    vtm_scope_stack_t *content_name_scope_list
);
vtm_state_stack_t* vtm_state_stack_push(
    vtm_state_stack_t *stack,
    int32_t rule_id,
    int32_t enter_pos,
    int32_t anchor_pos,
    vtm_scope_stack_t *name_scope_list,
    vtm_scope_stack_t *content_name_scope_list
);
void vtm_state_stack_retain(vtm_state_stack_t *stack);
void vtm_state_stack_release(vtm_state_stack_t *stack);
vtm_rule_t* vtm_state_stack_get_rule(vtm_state_stack_t *stack, vtm_grammar_t *grammar);

/* Rule helpers */
vtm_rule_t* vtm_rule_create(int32_t id, vtm_rule_type_t type);
void vtm_rule_destroy(vtm_rule_t *rule);
vtm_compiled_rule_t* vtm_rule_compile(vtm_rule_t *rule, vtm_grammar_t *grammar);
void vtm_compiled_rule_destroy(vtm_compiled_rule_t *compiled);

/* Grammar helpers */
int32_t vtm_grammar_add_rule(vtm_grammar_t *grammar, vtm_rule_t *rule);
vtm_rule_t* vtm_grammar_get_rule(vtm_grammar_t *grammar, int32_t rule_id);

/* Repository cache helpers */
vtm_repository_cache_t* vtm_repository_cache_create(void);
void vtm_repository_cache_destroy(vtm_repository_cache_t *cache);
int32_t vtm_repository_cache_find(vtm_repository_cache_t *cache, const char *rule_name);
void vtm_repository_cache_add(vtm_repository_cache_t *cache, const char *rule_name, int32_t rule_id);

/* Grammar parsing */
vtm_error_t vtm_parse_grammar_json(vtm_grammar_t *grammar, const char *json_string);

/* Tokenization */
vtm_match_result_t* vtm_match_rule(
    vtm_grammar_t *grammar,
    const char *line_text,
    uint32_t line_pos,
    vtm_state_stack_t *stack,
    int32_t anchor_position
);

#endif /* VSCODE_TEXTMATE_INTERNAL_H */
