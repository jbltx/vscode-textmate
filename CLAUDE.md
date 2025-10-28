# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

VSCode TextMate is an interpreter for TextMate grammar files that use the oniguruma dialect for regular expressions. This library is used in VS Code to provide syntax highlighting and tokenization. It supports loading grammar files from JSON or PLIST format.

## Git

You can check diff against base branch, but you should never try to commit anything yourself.

## Build and Test Commands

### Development Workflow
- `npm install` - Install dependencies
- `npm run watch` - Compile TypeScript in watch mode (use this during active development)
- `npm run compile` - One-time TypeScript compilation to `out/` directory
- `npm test` - Run all tests via Mocha (tests are in `out/tests/all.test.js` after compilation)
- `npm run benchmark` - Run performance benchmarks
- `npm run inspect -- PATH_TO_GRAMMAR PATH_TO_FILE` - Troubleshoot grammar tokenization

### Publishing
- `npm run bundle` - Build production bundle via webpack and generate API definitions
- `npm run prepublishOnly` - Full build pipeline (compile + webpack + API extraction)

### Note on Test Files
To run a single test, you need to modify `src/tests/all.test.ts` to import only the specific test file you want to run, then recompile with `npm run compile` before running `npm test`.

## Architecture

### Core Tokenization Flow

The library follows a multi-phase architecture for tokenizing source code:

1. **Registry Layer** (`src/main.ts`, `src/registry.ts`)
   - `Registry` is the main entry point - it manages grammar loading and theme application
   - `SyncRegistry` maintains the internal cache of loaded grammars and raw grammar definitions
   - The registry handles dependency resolution when grammars include other grammars

2. **Grammar Compilation** (`src/grammar/grammar.ts`)
   - `Grammar` class compiles raw TextMate grammar rules into an efficient tokenization state machine
   - Manages rule hierarchies, injections, and balanced bracket detection
   - Each grammar maintains a `_ruleId2desc` map and `_rootId` for the starting rule

3. **Rule System** (`src/rule.ts`)
   - Abstract `Rule` base class with concrete implementations: `CaptureRule`, `BeginEndRule`, `BeginWhileRule`, `MatchRule`, `IncludeOnlyRule`
   - Rules are compiled on-demand into `CompiledRule` objects containing Oniguruma scanners
   - Rules support captures (named token scopes) and back-references

4. **Tokenization** (`src/grammar/tokenizeString.ts`)
   - `_tokenizeString()` is the core tokenization algorithm
   - Uses a stack-based approach (`StateStack`) to track nested scopes
   - Each line is tokenized independently but carries over state via `ruleStack`

5. **Theme Application** (`src/theme.ts`)
   - `Theme` class uses a trie structure (`ThemeTrieElement`) to efficiently match scope paths to style attributes
   - Themes map scope selectors to font styles and colors
   - Supports inheritance where inner scopes can override outer scope styles

### Key State Management

- **StateStack** (`StateStackImpl` in `src/grammar/grammar.ts`): Immutable stack representing the current tokenization state. Must be passed between line tokenizations to maintain context for multi-line constructs (strings, comments, etc.).
- **INITIAL**: Starting state constant for the first line (`StateStackImpl.NULL`)

### Oniguruma Integration

The library abstracts regex operations through `IOnigLib` interface (`src/onigLib.ts`). The actual implementation is provided by `vscode-oniguruma` WASM module, which must be loaded asynchronously before creating a `Registry`.

### Grammar Dependencies

`ScopeDependencyProcessor` (`src/grammar/grammarDependencies.ts`) handles the dependency graph when grammars reference other grammars via `include` directives. The system processes includes in these forms:
- `#ruleName` - reference to repository rule in same grammar
- `source.lang` - reference to external grammar
- `source.lang#ruleName` - reference to specific rule in external grammar

### Token Encoding

Two tokenization APIs are provided:
- `tokenizeLine()` - Returns tokens with string scope arrays (easier to debug)
- `tokenizeLine2()` - Returns tokens as `Uint32Array` with encoded metadata (more efficient for editors)

Token metadata encoding is handled by `src/encodedTokenAttributes.ts` and includes language ID, token type, font style, and color information packed into 32-bit integers.

## Important Implementation Details

### Right-to-Left (RTL) Text Handling

The tokenizer has special logic for RTL languages (e.g., Arabic, Hebrew) in `containsRTL()` utility. RTL content affects whether consecutive equal tokens should be merged (see recent commit about RTL token merging).

### Balanced Bracket Tracking

`BalancedBracketSelectors` allows configuring which scopes should have bracket matching enabled/disabled, used by editors for features like bracket pair colorization.

### Grammar Injections

Grammars can be "injected" into other grammars at specific scope selectors (e.g., injecting JavaScript syntax into HTML `<script>` tags). The `Injection` interface and `collectInjections()` handle this with priority levels (-1, 0, 1 for L/default/R positioning).

## File Organization

- `src/main.ts` - Public API surface
- `src/grammar/` - Core grammar and tokenization logic
- `src/tests/` - Test suites and testing utilities
- `test-cases/` - Test fixtures with grammar files and expected tokenization outputs
- `benchmark/` - Performance benchmarking utilities
- `release/` - Webpack bundled output (generated)
- `out/` - TypeScript compiler output (generated)

# C++ Port

A C++ port of this library is maintained at:
- `textmate-cpp/`

The C++ port aims to provide similar functionality for TextMate grammar parsing and tokenization, optimized for performance in C++ applications. To verify correctness, the C++ port uses the same test cases as the TypeScript version.

**IMPORTANT**: Always compare the C++ implementation's output against the TypeScript version to ensure consistency.

## Build

The C++ port uses CMake for building. To build the project, run the following commands:

```bash
cd textmate-cpp
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Testing

### Main tests - First Mate

To run the tests for the C++ port, execute the following command from the `build` directory:

```bash
./tests/test_first_mate
```

This will run the test suite using `test-cases/first-mate/tests.json` as the source for test inputs,
and `test-cases/first-mate/fixtures/*.json` as the source for grammar fixtures.

### Other tests

When required, additional tests can be written and run individually. For example:

**Session API tests:**
```bash
./tests/test_session
```

**Theme tests:**
```bash
./tests/test_theme
```

**SyntaxHighlighter tests:**
```bash
./tests/test_syntax_highlighter
```

When editing tests, ensure to update the CMakeLists.txt file located in the `tests` directory.

NEVER USE `timeout` nor `gtimeout` CLI COMMANDS TO RUN TESTS, IT IS NOT SUPPORTED BY ALL PLATFORMS.

## C++ Port Architecture

### Core Components

The C++ port includes several key systems:

1. **Session API** (`session.h/cpp`, `session_c_api.h`)
   - Provides stateful, incremental tokenization
   - Automatic state management without manual state passing
   - Perfect for text editors with frequent edits
   - Reference counting for memory safety

2. **Theme System** (`theme.h/cpp`, `theme_c_api.h`)
   - Trie-based scope matching for efficient color lookup
   - StyleAttributes for font styles and colors
   - ColorMap for color pooling
   - Supports theme inheritance

3. **SyntaxHighlighter** (`syntax_highlighter.h/cpp`, `syntax_highlighter_c_api.h`) - NEW
   - High-level API combining Session + Theme
   - Automatic styling resolution
   - Complete highlighting information (colors, fonts, token types)
   - Built-in caching with version tracking
   - Perfect for general-purpose syntax highlighting
   - Full C API for language bindings

### Design Philosophy

- **Stateless API** (`tokenizeLine()`) - For performance-critical, custom rendering
- **Session API** (`SessionImpl`) - For incremental edits with automatic state management
- **SyntaxHighlighter** - For convenient, color-aware syntax highlighting

## Documentation

### TypeScript Implementation
- See `src/` for TypeScript source
- API reference in generated `out/` after building

### C++ Implementation

- **Session API**: `textmate-cpp/src/SESSION_API_README.md`
- **Theme System**: `textmate-cpp/src/theme.h` (header comments)
- **SyntaxHighlighter**: `textmate-cpp/src/SYNTAX_HIGHLIGHTER_README.md`
- **Quick Start**: `textmate-cpp/examples/SYNTAX_HIGHLIGHTER_QUICKSTART.md`
- **Implementation Summary**: `SYNTAX_HIGHLIGHTER_IMPLEMENTATION.md`

### C# Bindings

- **Implementation**: `examples/csharp-common/TextMate.cs`
- **Documentation**: `examples/csharp-common/README.md`
- **Quick Start**: `examples/SESSION_API_QUICKSTART.md`

### Build & Test Notes

All C++ tests build with no warnings:
```bash
cd textmate-cpp && mkdir -p build && cd build
cmake .. && cmake --build .
./tests/test_syntax_highlighter    # Verify SyntaxHighlighter
./tests/test_session               # Verify Session API
./tests/test_theme                 # Verify Theme system
```

For integration testing, compare C++ output against TypeScript version using test-cases.