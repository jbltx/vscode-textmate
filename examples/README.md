# TextMate C# Examples

This directory contains C# examples demonstrating how to use the TextMate C++ library via PInvoke for syntax highlighting and tokenization.

## Projects

### 1. csharp-common (TextMateSharp)

A shared library containing PInvoke declarations and managed wrappers for the TextMate C++ library.

**Key Components:**
- `TextMateNative.cs` - Low-level PInvoke declarations
- `TextMate.cs` - High-level managed wrapper with proper resource management

**Features:**
- ✅ Cross-platform support (Windows, macOS, Linux)
- ✅ Safe UTF-8/UTF-16 string conversion
- ✅ Proper handling of Unicode surrogate pairs
- ✅ Grammar dependency resolution
- ✅ Grammar injections support
- ✅ Automatic resource cleanup

### 2. csharp-playground

An interactive playground application demonstrating basic tokenization.

**Usage:**
```bash
cd csharp-playground
dotnet run [path-to-grammar.json]
```

**Example:**
```bash
dotnet run ../../test-cases/first-mate/fixtures/javascript.json
```

### 3. csharp-firstmate

A comprehensive test runner for the FirstMate test suite.

**Status:** ✅ All 65 tests passing (100%)

**Usage:**
```bash
cd csharp-firstmate
dotnet run
```

## Architecture

### C++ Native Library

The C++ library (`textmate-cpp/`) provides:
- TextMate grammar parsing and compilation
- Oniguruma regex engine integration
- High-performance tokenization

### C API Layer

The C API (`textmate-cpp/src/c_api.{h,cpp}`) exposes C-compatible functions:

```c
// Create registry
TextMateRegistry textmate_registry_create(TextMateOnigLib onigLib);

// Add grammar to registry
int textmate_registry_add_grammar_from_json(TextMateRegistry registry, const char* jsonContent);

// Set grammar injections
void textmate_registry_set_injections(TextMateRegistry registry, const char* scopeName,
                                     const char** injections, int32_t injectionCount);

// Load grammar by scope
TextMateGrammar textmate_registry_load_grammar(TextMateRegistry registry, const char* scopeName);

// Tokenize line
TextMateTokenizeResult* textmate_tokenize_line(TextMateGrammar grammar, const char* lineText,
                                               TextMateStateStack prevState);
```

### C# Wrapper

The C# wrapper (`csharp-common/`) provides:

```csharp
using var textMate = new TextMate();

// Add grammars to registry
textMate.AddGrammarFromJson(grammarJson);

// Set injections (optional)
textMate.SetInjections("source.js", new[] { "text.hyperlink" });

// Load grammar
var grammar = textMate.LoadGrammar("source.js");

// Tokenize
var state = TextMate.GetInitialState();
var result = grammar.TokenizeLine("const x = 42;", state);

foreach (var token in result.Tokens)
{
    string value = token.GetValue("const x = 42;");
    Console.WriteLine($"Token: {value}");
    foreach (var scope in token.Scopes)
        Console.WriteLine($"  - {scope}");
}
```

## Key Technical Details

### UTF-8 to UTF-16 Conversion

The C++ library works with UTF-8 strings and returns byte indices. C# uses UTF-16 strings (with surrogate pairs). The wrapper properly converts:

- **1-byte UTF-8** (ASCII) → 1 C# char
- **2-byte UTF-8** → 1 C# char
- **3-byte UTF-8** → 1 C# char
- **4-byte UTF-8** → 2 C# chars (surrogate pair)

Example: '𝞗' (U+1D797)
- UTF-8: `F0 9D 9E 97` (4 bytes)
- UTF-16: `D835 DF97` (2 chars - surrogate pair)

### Grammar Registry Workflow

1. **Create Registry** with callbacks for loading grammars
2. **Add Grammars** to populate the internal store
3. **Set Injections** (optional) for features like hyperlinks, TODO
4. **Load Grammar** by scope name (resolves dependencies)
5. **Tokenize** with state preservation across lines

### Memory Management

The wrapper implements `IDisposable` for proper cleanup:
- Registry disposal
- Oniguruma library cleanup
- Token result freeing

## Building

### Prerequisites
- .NET 9.0 SDK or later
- CMake 3.10+
- C++11 compiler
- Oniguruma library

### Build Steps

1. **Build C++ library:**
```bash
cd textmate-cpp
mkdir -p build
cd build
cmake ..
cmake --build .
```

2. **Build C# projects:**
```bash
cd examples/csharp-common
dotnet build

cd ../csharp-playground
dotnet build

cd ../csharp-firstmate
dotnet build
```

## Testing

Run the comprehensive FirstMate test suite:

```bash
cd examples/csharp-firstmate
dotnet run
```

Expected output:
```
TextMate FirstMate Test Runner (C#)
====================================

Running 65 test case(s)...

[65 tests run...]

==================================================
Total: 65 | Passed: 65 | Failed: 0
==================================================
```

## Platform Support

| Platform | Library Name | Status |
|----------|--------------|--------|
| macOS    | libvscode-textmate-cpp.dylib | ✅ Tested |
| Linux    | libvscode-textmate-cpp.so | ✅ Compatible |
| Windows  | vscode-textmate-cpp.dll | ✅ Compatible |

## Performance

The C# wrapper has minimal overhead:
- Direct PInvoke calls to native code
- Zero-copy for token scopes
- Efficient UTF-8/UTF-16 conversion using pre-computed mapping

## License

Same as vscode-textmate (Microsoft Corporation)
