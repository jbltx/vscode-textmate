C Tokenization Implementation Plan for VSCode TextMate

Executive Summary

The C implementation has all infrastructure components (registry, grammar loading, state management, memory management) but lacks the core tokenization
algorithm. This plan details porting the TypeScript tokenization engine from src/grammar/tokenizeString.ts (~600 lines) to C.

---
Phase 1: Core Data Structures & Utilities

1.1 Line Tokenization Context

File: vscode_textmate_tokenize.c (new file)

Purpose: Container for tokenization state during a single line scan

C Structures Needed:
typedef struct {
    vtm_grammar_t *grammar;
    const char *line_text;
    uint32_t line_length;
    uint32_t line_pos;           // Current position in line
    int32_t anchor_pos;          // Anchor position for \G
    bool is_first_line;
    vtm_state_stack_t *stack;    // Current state stack
    vtm_token_list_t *tokens;    // Output token accumulator
    bool stop;                   // Stop flag for end of line
    uint32_t time_limit;         // Timeout in milliseconds
    uint64_t start_time;         // Start timestamp
} vtm_tokenize_context_t;

Key TypeScript Mapping:
- _tokenizeString() local variables → struct fields
- Eliminates parameter passing between functions
- Simplifies state management

---
1.2 Token Accumulator

Purpose: Dynamically grow token array during tokenization

C Structure:
typedef struct {
    vtm_token_t *tokens;        // For tokenizeLine
    uint32_t *binary_tokens;    // For tokenizeLine2
    uint32_t count;
    uint32_t capacity;
    bool emit_binary;
} vtm_token_list_t;

Operations:
- vtm_token_list_create(bool emit_binary)
- vtm_token_list_add_token(list, start, end, scopes)
- vtm_token_list_add_binary(list, start, metadata)
- vtm_token_list_destroy(list)

TypeScript Reference: LineTokens class (grammar.ts:945-1112)

---
1.3 Match Result Structure

Already defined in internal.h, but enhance:

typedef struct {
    int32_t matched_rule_id;     // Rule ID or VTM_END_RULE_ID
    vtm_capture_t *captures;     // Dynamic array
    uint32_t capture_count;
    bool priority_match;         // For injection priority
} vtm_match_result_t;

Add helper functions:
- vtm_match_result_create()
- vtm_match_result_destroy(result)
- vtm_match_result_has_advanced(result, line_pos) - Check if position moved

TypeScript Reference: IMatchResult (tokenizeString.ts:432-435)

---
Phase 2: Regex Compilation & Rule Scanning

2.1 Rule Compilation System

Purpose: Convert grammar rules to Oniguruma scanners with anchor support

Key Concept: TypeScript caches compiled patterns per rule; C must do the same

C Structure:
typedef struct {
    regex_t **regexes;           // Array of compiled Oniguruma regexes
    int32_t *rule_ids;           // Parallel array of rule IDs
    uint32_t count;
} vtm_scanner_t;

Implementation Functions:

2.1.1 vtm_rule_compile()

Signature:
vtm_scanner_t* vtm_rule_compile(
    vtm_rule_t *rule,
    vtm_grammar_t *grammar,
    const char *end_regex_source,  // For BeginEnd rules with back-refs
    bool allow_A,                  // Allow \A anchor
    bool allow_G                   // Allow \G anchor
);

Logic:
1. For MatchRule: Compile single pattern
2. For BeginEndRule:
  - Collect child patterns from patterns array
  - Add end pattern (first or last based on apply_end_pattern_last)
  - Handle back-references in end pattern
3. For BeginWhileRule: Compile child patterns only
4. For IncludeOnlyRule: Recursively collect patterns from referenced rules

Caching Strategy:
- Add vtm_compiled_rule_cache_t to each rule structure
- Cache 4 variants: A0_G0, A0_G1, A1_G0, A1_G1
- Only compile on first use

TypeScript Reference:
- Rule.compile() / compileAG() (rule.ts:86-88)
- prepareRuleSearch() (tokenizeString.ts:530-538)

---
2.1.2 Anchor Resolution

Purpose: Replace \A and \G in patterns based on context

Implementation:
char* vtm_resolve_anchors(const char *pattern, bool allow_A, bool allow_G) {
    // Scan pattern for \A and \G
    // Replace with literal match or unmatchable pattern \uFFFF
    // Build cache of 4 variants (A0_G0, A0_G1, A1_G0, A1_G1)
}

Rules:
- \A matches beginning of string (allow when isFirstLine)
- \G matches current position (allow when linePos == anchorPosition)
- Replace with \uFFFF (never matches) when not allowed

TypeScript Reference: RegExpSource.resolveAnchors() (rule.ts:716-734)

---
2.1.3 Scanner Execution

Purpose: Find next match in line text

vtm_match_result_t* vtm_scanner_find_next(
    vtm_scanner_t *scanner,
    const char *line_text,
    uint32_t start_pos
) {
    // Try each regex in scanner
    // Return earliest match (lowest start position)
    // Set matched_rule_id to corresponding rule ID
}

Optimization: Use Oniguruma's multi-regex search if available

---
2.2 Back-Reference Resolution

Purpose: Resolve \1, \2, etc. in end/while patterns

Implementation:
char* vtm_resolve_back_references(
    const char *pattern,
    const char *line_text,
    vtm_capture_t *captures,
    uint32_t capture_count
) {
    // Find \d+ patterns
    // Replace with escaped text from corresponding capture group
    // Return newly allocated string
}

Example:
- Pattern: \1> with capture[1] = "foo" → foo>
- Must escape regex special chars in captured text

TypeScript Reference: RegExpSource.resolveBackReferences() (rule.ts:656-664)

---
Phase 3: Core Tokenization Loop

3.1 Main Entry Point

Signature:
vtm_tokenize_result_t* vtm_tokenize_string(
    vtm_grammar_t *grammar,
    const char *line_text,
    bool is_first_line,
    uint32_t line_pos,
    vtm_state_stack_t *prev_stack,
    uint32_t time_limit
);

Algorithm (mirrors tokenizeString.ts:31-328):

1. Initialize context:
    - line_length = strlen(line_text)
    - anchor_pos = -1
    - stop = false
    - start_time = current_time_ms()

2. Check while conditions (if not first iteration):
    - Call vtm_check_while_conditions()
    - May pop stack layers and advance line_pos

3. Main loop while (!stop):
    a. Check timeout:
      - if (time_limit && elapsed > time_limit) return early

    b. Call scan_next():
      - Find next match or injection
      - Handle match/no-match cases
      - Update stack, line_pos, anchor_pos
      - May set stop = true

4. Return result with final stack and tokens

TypeScript Reference: _tokenizeString() (tokenizeString.ts:31-328)

---
3.2 While Condition Checking

Purpose: Pop BeginWhile rules whose while condition fails

Signature:
void vtm_check_while_conditions(
    vtm_tokenize_context_t *ctx
);

Algorithm:
1. Collect all BeginWhileRule nodes in stack (bottom to top)
2. For each while rule:
    a. Compile while pattern with back-refs resolved
    b. Try to match at current line_pos
    c. If match:
      - Handle while captures
      - Produce tokens
      - Update line_pos and anchor_pos
    d. If no match:
      - Pop stack up to and including this rule
      - Break loop

Edge Cases:
- Must process from bottom to top
- First failure stops checking
- While captures create tokens

TypeScript Reference: _checkWhileConditions() (tokenizeString.ts:335-390)

---
3.3 Scan Next Position

Purpose: Find and handle next match at current position

Signature:
void vtm_scan_next(vtm_tokenize_context_t *ctx);

Algorithm:
1. Call vtm_match_rule_or_injections()
2. If no match:
    - Produce token for rest of line
    - Set stop = true
    - Return

3. Extract: matched_rule_id, captures

4. Check if position advanced:
    - has_advanced = (captures[0].end > ctx->line_pos)

5. Branch on matched_rule_id:

    a. If matched_rule_id == VTM_END_RULE_ID:
      → Handle end pattern match (pop rule)

    b. Else (matched a begin or match rule):
      → Handle rule match (push or match-pop)

TypeScript Reference: scanNext() (tokenizeString.ts:74-327)

---
3.4 End Rule Handling

Purpose: Process matched end pattern of BeginEndRule

Algorithm:
void handle_end_rule(vtm_tokenize_context_t *ctx, vtm_match_result_t *match) {
    1. Get current rule (must be BeginEndRule)
    2. Produce token up to match start
    3. Handle end captures
    4. Produce token up to match end
    5. Pop stack:
        - Store anchor_pos from popped element
        - stack = stack->parent
    6. Detect endless loop:
        - If !has_advanced && enter_pos == line_pos:
          → Restore stack, produce to end, stop
    7. Update line_pos if advanced
}

TypeScript Reference: Lines 110-156 in scanNext()

---
3.5 Begin/Match Rule Handling

Purpose: Process matched begin or match pattern

Algorithm:
void handle_rule_match(vtm_tokenize_context_t *ctx, vtm_match_result_t *match) {
    1. Get matched rule from grammar
    2. Produce token up to match start
    3. Push rule onto stack (with scope name)

    4. Branch on rule type:

        a. BeginEndRule:
          - Handle begin captures
          - Produce token to match end
          - Update anchor_pos
          - Push content scope
          - Resolve end pattern back-refs if needed
          - Check endless loop (same rule, no advance)

        b. BeginWhileRule:
          - Handle begin captures
          - Produce token to match end
          - Update anchor_pos
          - Push content scope
          - Resolve while pattern back-refs if needed
          - Check endless loop

        c. MatchRule:
          - Handle captures
          - Produce token to match end
          - Pop rule immediately (match rules don't nest)
          - Check endless loop (no advance at all)

    5. Update line_pos if advanced
}

Endless Loop Detection:
- Case 1: BeginEnd pushed & popped without advancing
- Case 2: BeginEnd/BeginWhile pushed same rule without advancing
- Case 3: Match rule without advancing
- Action: Pop stack, produce to end of line, stop

TypeScript Reference: Lines 158-320 in scanNext()

---
Phase 4: Rule Matching & Injections

4.1 Match Rule or Injections

Purpose: Find next applicable rule (normal or injected)

Signature:
vtm_match_result_t* vtm_match_rule_or_injections(
    vtm_tokenize_context_t *ctx
);

Algorithm:
1. Try normal rule: match_result = vtm_match_rule(ctx)
2. Get grammar injections: injections = grammar->injections
3. If no injections: return match_result
4. Try injections: injection_result = vtm_match_injections(ctx)
5. If no injection_result: return match_result
6. If no match_result: return injection_result
7. Compare positions and priority:
    - If injection starts earlier: return injection
    - If injection starts at same position and has priority: return injection
    - Else: return match_result

TypeScript Reference: matchRuleOrInjections() (tokenizeString.ts:399-430)

---
4.2 Match Normal Rule

Purpose: Match current stack top rule

Signature:
vtm_match_result_t* vtm_match_rule(vtm_tokenize_context_t *ctx);

Algorithm:
1. Get rule from stack top: rule = stack->rule_id
2. Determine anchor flags:
    - allow_A = ctx->is_first_line
    - allow_G = (ctx->line_pos == ctx->anchor_pos)
3. Compile rule: scanner = vtm_rule_compile(rule, grammar, stack->end_rule, allow_A, allow_G)
4. Find match: result = vtm_scanner_find_next(scanner, line_text, line_pos)
5. Return result (or NULL if no match)

TypeScript Reference: matchRule() (tokenizeString.ts:437-467)

---
4.3 Match Injections

Purpose: Find best matching injection

Signature:
vtm_match_result_t* vtm_match_injections(
    vtm_tokenize_context_t *ctx,
    vtm_injection_t *injections,
    uint32_t injection_count
);

Algorithm:
1. Get current scope names from stack
2. Initialize: best_match = NULL, best_pos = MAX_INT
3. For each injection:
    a. Check if injection selector matches scopes
    b. If not: continue
    c. Compile injection rule
    d. Find match at current position
    e. If match starts earlier than best_pos:
      - best_match = match
      - best_pos = match.start
      - best_priority = injection.priority
    f. If match at line_pos: break (can't do better)
4. Set best_match.priority_match if priority == -1
5. Return best_match

Injection Priority:
- -1 (L): Left - takes precedence on tie
- 0 (default): Normal
- 1 (R): Right - doesn't take precedence on tie

TypeScript Reference: matchInjections() (tokenizeString.ts:469-522)

---
Phase 5: Capture Handling

5.1 Handle Captures

Purpose: Create tokens for capture groups within a match

Signature:
void vtm_handle_captures(
    vtm_tokenize_context_t *ctx,
    vtm_capture_rule_t **capture_rules,
    uint32_t capture_rule_count,
    vtm_capture_t *captures,
    uint32_t capture_count
);

Algorithm (Complex - requires careful porting):
1. Validate inputs
2. Initialize local_stack (for nested captures)
3. max_end = captures[0].end (don't process beyond main match)

4. For each capture (i = 0 to min(capture_rule_count, capture_count)):
    a. Skip if capture_rule is NULL or capture is empty
    b. Skip if capture starts beyond max_end

    c. Pop local_stack while top.end_pos <= capture.start

    d. Produce token up to capture.start (from local_stack or main stack)

    e. Check if capture requires retokenization:
      - If capture_rule->retokenize_captured_with_rule_id:
        → Recursively tokenize captured text
        → Continue to next capture

    f. If capture_rule has scope name:
      - Push onto local_stack with scope

5. Pop remaining local_stack entries (produce remaining tokens)

Retokenization: Some captures re-parse content with a different rule (e.g., string interpolation)

TypeScript Reference: handleCaptures() (tokenizeString.ts:561-632)

---
5.2 Local Stack Management

Purpose: Track nested scope layers within captures

C Structure:
typedef struct {
    vtm_scope_stack_t *scopes;
    uint32_t end_pos;
} vtm_local_stack_element_t;

typedef struct {
    vtm_local_stack_element_t *elements;
    uint32_t count;
    uint32_t capacity;
} vtm_local_stack_t;

Operations:
- Push when entering capture scope
- Pop when reaching end_pos
- Used to produce tokens with correct scope hierarchy

TypeScript Reference: LocalStackElement (tokenizeString.ts:634-642)

---
Phase 6: Grammar Injections

6.1 Injection Collection

Purpose: Build list of grammar rules that can inject into current grammar

Signature:
vtm_injection_t* vtm_grammar_collect_injections(
    vtm_grammar_t *grammar,
    uint32_t *out_count
);

C Structure:
typedef struct {
    char *debug_selector;        // For debugging
    vtm_scope_matcher_t *matcher; // Scope selector matcher
    int32_t priority;            // -1 (L), 0, 1 (R)
    int32_t rule_id;
} vtm_injection_t;

Algorithm:
1. Check grammar->injections field (from JSON)
2. For each injection in grammar:
    - Parse selector string
    - Create matcher function/structure
    - Add to list with priority
3. Query registry for external injection grammars
4. Sort by priority (-1 < 0 < 1)

TypeScript Reference:
- _collectInjections() (grammar.ts:168-227)
- collectInjections() (grammar.ts:57-69)

---
6.2 Scope Matching

Purpose: Determine if injection selector matches current scope stack

Selector Syntax Examples:
- "text.html" - Matches in HTML files
- "source.js meta.embedded" - Matches embedded JS
- "- text.xml" - Negative match (exclude)

Implementation:
bool vtm_scope_matcher_matches(
    vtm_scope_matcher_t *matcher,
    char **scope_names,
    uint32_t scope_count
);

Algorithm:
- Parse selector into terms
- Match each term against scope list (allows partial prefix matches)
- Support negative selectors (exclusions)

TypeScript Reference:
- createMatchers() (matcher.ts)
- nameMatcher() (grammar.ts:71-85)

---
Phase 7: Token Production & Output

7.1 Produce Token

Purpose: Add token to output with scope information

Signature:
void vtm_produce_token(
    vtm_tokenize_context_t *ctx,
    uint32_t end_index
);

Algorithm:
1. If end_index <= last_token_end: return (no-op)
2. Get current scopes from stack->content_name_scope_list
3. If emit_binary:
    - Compute metadata from scopes
    - Check if can merge with previous token (same metadata)
    - Add: [last_token_end, metadata] to binary array
4. Else:
    - Build scope string array
    - Add: {start: last_token_end, end: end_index, scopes: array}
5. Update last_token_end = end_index

TypeScript Reference: LineTokens.produce() (grammar.ts:985-1075)

---
7.2 Metadata Encoding (Binary Mode)

Purpose: Pack scope attributes into uint32

Encoding:
Bits  0-7:   Language ID
Bits  8-10:  Token Type (comment, string, etc.)
Bits 11-14:  Font Style (bold, italic, underline)
Bits 15-23:  Foreground Color ID
Bits 24-31:  Background Color ID

Implementation:
uint32_t vtm_encode_metadata(
    uint8_t language_id,
    uint8_t token_type,
    uint8_t font_style,
    uint16_t foreground,
    uint16_t background
) {
    return (language_id << 0) |
            (token_type << 8) |
            (font_style << 11) |
            (foreground << 15) |
            (background << 24);
}

TypeScript Reference: EncodedTokenAttributes.set() (encodedTokenAttributes.ts)

---
7.3 Final Result Assembly

Purpose: Convert token list to result structure

For tokenizeLine():
vtm_tokenize_result_t* vtm_assemble_result(vtm_tokenize_context_t *ctx) {
    // Copy tokens from ctx->tokens to result structure
    // Remove newline token if present
    // Set rule_stack to final ctx->stack
}

For tokenizeLine2():
vtm_tokenize_result2_t* vtm_assemble_result2(vtm_tokenize_context_t *ctx) {
    // Copy binary tokens to uint32 array
    // Remove newline token if present
    // Set rule_stack to final ctx->stack
}

---
Phase 8: Memory Management & Error Handling

8.1 Reference Counting

Already implemented for:
- vtm_state_stack_t (ref_count field)
- vtm_scope_stack_t (ref_count field)

Rules:
- Increment when storing reference
- Decrement when done with reference
- Free when ref_count reaches 0

---
8.2 Error Recovery

Strategies:

1. Regex compilation errors:
  - Log warning
  - Skip rule
  - Continue with remaining rules
2. Timeout:
  - Set stopped_early = true
  - Return current stack state
  - Editor can retry on next frame
3. Memory allocation failures:
  - Return NULL
  - Caller checks and handles
4. Endless loop detection:
  - Produce token to end of line
  - Stop tokenization
  - Prevents infinite loops in bad grammars

---
8.3 Cleanup Functions

Must implement proper cleanup for all allocated structures:

void vtm_scanner_destroy(vtm_scanner_t *scanner);
void vtm_match_result_destroy(vtm_match_result_t *result);
void vtm_token_list_destroy(vtm_token_list_t *list);
void vtm_local_stack_destroy(vtm_local_stack_t *stack);
void vtm_tokenize_context_destroy(vtm_tokenize_context_t *ctx);

---
Phase 9: Testing & Validation

9.1 Unit Tests

Test each component in isolation:

1. Anchor resolution: Test \A and \G handling
2. Back-reference resolution: Test \1, \2, etc.
3. Rule compilation: Test caching and variants
4. Scanner matching: Test multi-pattern matches
5. Capture handling: Test nested captures
6. While conditions: Test BeginWhile popping

---
9.2 Integration Tests

Test full tokenization:

1. Simple grammar: Single Match rules
2. Nested grammar: BeginEnd rules
3. While loops: BeginWhile rules
4. Injections: Grammar with injections
5. Captures: Rules with capture groups
6. Edge cases:
  - Empty lines
  - Very long lines
  - Deep nesting
  - Endless loop detection

---
9.3 Comparison Tests

Verify C output matches TypeScript:

1. Load same grammar in both implementations
2. Tokenize same test files
3. Compare token arrays:
  - Token count
  - Start/end positions
  - Scope names
  - Stack state between lines

Suggested Test Suite:
- Use existing test-cases/ directory
- Focus on test-cases/first-mate/ tests
- Compare against TypeScript output

---
Phase 10: Performance Optimization

10.1 Profiling

Measure:
- Time per line
- Memory allocations
- Regex compilation cache hits

---
10.2 Optimization Opportunities

1. String interning: Deduplicate scope names
2. Arena allocators: Batch allocations per line
3. SIMD: Fast string scanning for anchors
4. Lazy compilation: Only compile rules when matched
5. Scanner caching: Cache compiled scanners per rule

---
10.3 Memory Usage

Reduce allocations:
- Reuse tokenize context across lines
- Pool match result structures
- Preallocate common sizes

---
Implementation Order (Recommended)

Week 1: Foundation

1. ✅ Phase 1: Core data structures
2. ✅ Phase 7.1: Token production (basic)
3. ✅ Phase 2.1: Rule compilation (basic, no caching)

Week 2: Core Loop

4. ✅ Phase 3.1: Main tokenization loop skeleton
5. ✅ Phase 3.3: Scan next (basic)
6. ✅ Phase 4.2: Match normal rule
7. ✅ Phase 3.5: Handle match rules only

Week 3: Advanced Rules

8. ✅ Phase 3.4: Handle end rules
9. ✅ Phase 3.5: Handle BeginEnd/BeginWhile
10. ✅ Phase 2.2: Back-reference resolution
11. ✅ Phase 3.2: While condition checking

Week 4: Captures & Injections

12. ✅ Phase 5: Capture handling
13. ✅ Phase 6: Grammar injections
14. ✅ Phase 4.1: Match rule or injections

Week 5: Polish & Test

15. ✅ Phase 2.1: Anchor resolution caching
16. ✅ Phase 7.2: Binary metadata encoding
17. ✅ Phase 8: Memory management review
18. ✅ Phase 9: Testing & validation

Week 6: Optimization

19. ✅ Phase 10: Performance tuning
20. ✅ Final integration testing

---
Key Challenges & Mitigation

Challenge 1: Complexity

Issue: ~600 lines of dense TypeScript with many edge cases

Mitigation:
- Break into small, testable functions
- Port function-by-function with unit tests
- Start with simple cases, add complexity incrementally

---
Challenge 2: Memory Management

Issue: No garbage collection, must track all allocations

Mitigation:
- Use reference counting for shared structures
- Clear ownership rules for each structure
- Valgrind testing to catch leaks

---
Challenge 3: String Handling

Issue: C string manipulation is error-prone

Mitigation:
- Create helper library for common operations
- Always NULL-terminate strings
- Use strdup() and track ownership

---
Challenge 4: Regex Back-References

Issue: Dynamically modifying patterns is complex

Mitigation:
- Build comprehensive test suite first
- Cache resolved patterns when possible
- Escape special characters carefully

---
Challenge 5: State Stack Immutability

Issue: TypeScript uses immutable stacks, C must simulate

Mitigation:
- Use reference counting (already implemented)
- Never modify stack in place
- Create new stack nodes for push/pop

---
Success Criteria

Minimum Viable Implementation:

✅ Tokenize simple grammar (Match rules only)✅ Handle BeginEnd rules (basic nesting)✅ Correct token positions and counts✅ No memory leaks

Full Implementation:

✅ All rule types (Match, BeginEnd, BeginWhile, IncludeOnly)✅ Capture handling (including retokenization)✅ Grammar injections✅ Back-reference
resolution✅ While condition checking✅ Endless loop detection✅ Binary token format✅ Passes all existing TypeScript test cases

Production Ready:

✅ Performance within 2x of TypeScript✅ Memory usage < 10MB per file✅ No crashes on malformed grammars✅ Timeout handling for long lines

---
File Structure

ThirdParty/oniguruma/src/
├── vscode_textmate.h              (existing - public API)
├── vscode_textmate.c              (existing - registry, state, grammar)
├── vscode_textmate_internal.h    (existing - internal structures)
├── vscode_textmate_parser.c      (existing - JSON parsing)
├── vscode_textmate_tokenize.c    (NEW - main tokenization logic)
├── vscode_textmate_scanner.c     (NEW - rule compilation & scanning)
├── vscode_textmate_captures.c    (NEW - capture handling)
├── vscode_textmate_injections.c  (NEW - injection matching)
└── vscode_textmate_utils.c       (NEW - string helpers, scope matching)

---
Conclusion

This plan provides a structured, incremental approach to implementing the VSCode TextMate tokenization algorithm in C. By following the phases in order and
  testing at each step, the implementation can be built reliably while maintaining correctness and performance.

The key insight is that while the TypeScript code is ~600 lines, it represents distinct functional units that can be ported independently and tested in
isolation before integration.

Estimated total C code: ~2000-2500 lines (including helpers, error handling, and comments)

Estimated development time: 6-8 weeks for a single developer working full-time