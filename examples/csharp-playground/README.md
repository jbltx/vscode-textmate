# TextMate C# Playground

This is a C# example application that demonstrates how to use the TextMate C++ library via PInvoke for syntax highlighting and tokenization.

## Prerequisites

- .NET 8.0 SDK or later
- The TextMate C++ library must be built first

## Building the C++ Library

Before running this example, you need to build the TextMate C++ library:

```bash
cd ../../textmate-cpp
mkdir -p build
cd build
cmake ..
cmake --build .
```

This will create the native library at `textmate-cpp/build/lib/libvscode-textmate-cpp.dylib` (macOS) or `vscode-textmate-cpp.dll` (Windows).

## Running the Example

From this directory:

```bash
# Run with default JavaScript grammar
dotnet run

# Run with a specific grammar file
dotnet run ../../test-cases/first-mate/fixtures/javascript.json
dotnet run ../../test-cases/first-mate/fixtures/typescript.json
```

## Project Structure

- **TextMateNative.cs** - Low-level PInvoke declarations for the C API
- **TextMate.cs** - High-level managed wrapper classes
- **Program.cs** - Example application demonstrating tokenization
- **TextMatePlayground.csproj** - C# project file

## How It Works

1. The C++ library exposes a C API (defined in `textmate-cpp/src/c_api.h`)
2. C# uses PInvoke to call these C functions
3. The managed wrapper classes provide a clean, type-safe C# API
4. The example loads a TextMate grammar and tokenizes sample code

## Example Output

```
TextMate C# Playground
=====================

Loading grammar: ../../test-cases/first-mate/fixtures/javascript.json
Grammar loaded successfully!

Tokenizing JavaScript code:
---------------------------

Line: function hello(name) {
  [  0:  8] 'function'
            - source.js
            - meta.function.js
            - storage.type.function.js
  ...

✓ Tokenization completed successfully!
```

## API Overview

### TextMate Class

```csharp
using var textMate = new TextMate();
var grammar = textMate.LoadGrammarFromFile("path/to/grammar.json");
```

### Tokenization

```csharp
var state = TextMate.GetInitialState();
var result = grammar.TokenizeLine("const x = 42;", state);

foreach (var token in result.Tokens)
{
    Console.WriteLine($"Token: {token.StartIndex}..{token.EndIndex}");
    foreach (var scope in token.Scopes)
    {
        Console.WriteLine($"  Scope: {scope}");
    }
}

// Update state for next line
state = result.RuleStack;
```

## Cross-Platform Support

The example is designed to work on:
- **macOS**: Uses `libvscode-textmate-cpp.dylib`
- **Windows**: Uses `vscode-textmate-cpp.dll`
- **Linux**: Uses `libvscode-textmate-cpp.so`

The C# runtime will automatically load the correct library based on the platform.

## Troubleshooting

### Library Not Found

If you get a `DllNotFoundException`, ensure:
1. The C++ library has been built successfully
2. The library file is copied to the output directory (handled automatically by the .csproj)
3. On macOS/Linux, you may need to set `DYLD_LIBRARY_PATH` or `LD_LIBRARY_PATH`

### Tokenization Fails

If tokenization fails:
1. Verify the grammar file path is correct
2. Check that the grammar file is valid JSON
3. Ensure the Oniguruma library is linked correctly with the C++ library
