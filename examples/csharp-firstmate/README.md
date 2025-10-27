# TextMate FirstMate C# Test Runner

This project runs the FirstMate test suite from `test-cases/first-mate/tests.json` using the C# TextMate wrapper over the native C++ library.

## Status

✅ **All 65 tests passing (100%)**

## Prerequisites

- .NET 9.0 SDK or later
- The TextMate C++ library must be built first

## Building the C++ Library

```bash
cd ../../textmate-cpp
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Running the Tests

```bash
cd examples/csharp-firstmate
dotnet run
```

Or run with a specific test file:

```bash
dotnet run path/to/tests.json
```

## Test Unicode

To test Unicode handling specifically:

```bash
dotnet run -- --test-unicode
```

## Architecture

The test runner:

1. **Parses test cases** from `tests.json` which contains:
   - `desc`: Test description
   - `grammars`: Array of grammar files to load
   - `grammarPath`: Main grammar file
   - `grammarScopeName`: Main grammar scope
   - `grammarInjections`: Optional grammar injections
   - `lines`: Array of lines to tokenize with expected tokens

2. **Loads all grammars** into the registry first (so dependencies can be resolved)

3. **Sets up injections** if specified (for features like hyperlink detection, TODO highlighting)

4. **Loads the main grammar** by scope name through the registry

5. **Tokenizes each line** and compares:
   - Token count
   - Token values (extracted text)
   - Token scopes (array of scope names)

## Key Implementation Details

### UTF-8 to UTF-16 Conversion

The C++ library returns byte indices in UTF-8 encoding, but C# strings use UTF-16. The wrapper properly converts between these:

- **UTF-8**: Variable-length encoding (1-4 bytes per character)
- **UTF-16**: C# uses 1-2 char values per character (surrogate pairs for U+10000+)

For example, the character '𝞗' (U+1D797):
- UTF-8: 4 bytes (F0 9D 9E 97)
- UTF-16: 2 chars (D835 DF97) - surrogate pair

The conversion algorithm:
1. Converts the C# string to UTF-8 bytes
2. Builds a mapping from UTF-8 byte index to UTF-16 char index
3. Properly accounts for surrogate pairs (4-byte UTF-8 → 2-char UTF-16)
4. Uses this mapping to convert token indices returned by C++

### Grammar Injections

Some tests require grammar injections (e.g., hyperlink detection in comments). The C API supports this through:

```csharp
textmate.SetInjections(scopeName, new[] { "text.hyperlink" });
```

This is set before loading the grammar, allowing the registry to properly configure injections.

### Empty Token Filtering

Following the TypeScript implementation, empty tokens are filtered out for non-empty lines to match expected test behavior.

## Test Results Breakdown

- **Total Tests**: 65
- **Passed**: 65 (100%)
- **Failed**: 0

All test categories passing:
- ✅ Basic tokenization
- ✅ Multi-line constructs
- ✅ Grammar includes and dependencies
- ✅ Grammar injections (hyperlinks, TODO)
- ✅ Unicode handling (including surrogate pairs)
- ✅ Nested captures
- ✅ Begin/end rules
- ✅ While rules
- ✅ Balanced bracket detection

## Comparison with Other Implementations

This C# implementation achieves 100% compatibility with:
- TypeScript reference implementation (src/tests/tokenization.test.ts)
- C++ implementation (textmate-cpp/tests/test_first_mate.cpp)

The test suite validates that all three implementations produce identical tokenization results.
