# VSCode TextMate C++ Port

This is a C++11 port of the TypeScript vscode-textmate library. It provides TextMate grammar parsing and tokenization capabilities as a shared library.

## Features

- ✅ JSON grammar file parsing (using RapidJSON)
- ✅ TextMate grammar compilation and rule management
- ✅ Text tokenization with scope tracking
- ✅ Theme support with color mapping
- ✅ Oniguruma regex integration
- ✅ StateStack for multi-line tokenization
- ⚠️  Simplified tokenization algorithm (functional but not feature-complete)

## Architecture

The port closely follows the TypeScript implementation structure:

```
textmate-cpp/
├── src/
│   ├── types.h                          # Common type definitions
│   ├── utils.h/.cpp                     # Utility functions
│   ├── onigLib.h/.cpp                   # Oniguruma wrapper
│   ├── rawGrammar.h/.cpp                # Grammar data structures
│   ├── parseRawGrammar.h/.cpp           # JSON grammar parser
│   ├── theme.h/.cpp                     # Theme and styling
│   ├── encodedTokenAttributes.h/.cpp    # Token metadata encoding
│   ├── rule.h/.cpp                      # Grammar rules (Match, BeginEnd, etc.)
│   ├── matcher.h/.cpp                   # Scope matching logic
│   ├── grammarDependencies.h/.cpp       # Dependency resolution
│   ├── basicScopesAttributeProvider.h/.cpp # Scope attributes
│   ├── tokenizeString.h/.cpp            # Core tokenization
│   ├── grammar.h/.cpp                   # Grammar class
│   ├── registry.h/.cpp                  # Grammar registry
│   └── main.h/.cpp                      # Public API
├── tests/
│   └── test_grammar.cpp                 # Basic test suite
└── CMakeLists.txt
```

## Dependencies

- **C++11** compiler (GCC, Clang, or MSVC)
- **Oniguruma** (from ThirdParty/oniguruma)
- **RapidJSON** (from ThirdParty/rapidjson, header-only)
- **CMake** 3.10 or later

## Building

### 1. Build Oniguruma (if not already built)

```bash
cd ../ThirdParty/oniguruma
./autogen.sh
./configure
make
```

### 2. Build vscode-textmate-cpp

```bash
cd textmate-cpp
mkdir build
cd build
cmake ..
make
```

This will create:
- `lib/libvscode-textmate-cpp.so` (or .dylib on macOS, .dll on Windows)
- `bin/test_grammar` (test executable)

### 3. Run tests

```bash
./bin/test_grammar
```

## Usage Example

```cpp
#include "main.h"
#include <iostream>

using namespace vscode_textmate;

int main() {
    // 1. Initialize Oniguruma
    IOnigLib* onigLib = new DefaultOnigLib();

    // 2. Parse a grammar
    std::string grammarJson = R"({
        "scopeName": "source.cpp",
        "patterns": [
            {"name": "comment.line", "match": "//.*$"}
        ]
    })";

    IRawGrammar* grammar = parseJSONGrammar(grammarJson, nullptr);

    // 3. Create registry
    RegistryOptions options;
    options.onigLib = onigLib;
    options.loadGrammar = [grammar](const ScopeName& scopeName) {
        return (scopeName == "source.cpp") ? grammar : nullptr;
    };

    Registry* registry = new Registry(options);

    // 4. Load grammar
    Grammar* grammarObj = registry->addGrammar(grammar);

    // 5. Tokenize text
    std::string line = "// This is a comment";
    ITokenizeLineResult result = grammarObj->tokenizeLine(line, nullptr);

    // 6. Process tokens
    for (const auto& token : result.tokens) {
        std::cout << "Token [" << token.startIndex << "-"
                  << token.endIndex << "]: ";
        for (const auto& scope : token.scopes) {
            std::cout << scope << " ";
        }
        std::cout << std::endl;
    }

    // Cleanup
    delete registry;
    delete grammar;
    delete onigLib;

    return 0;
}
```

## API Overview

### Core Classes

- **`Registry`**: Manages grammars and theme, main entry point
- **`Grammar`**: Compiled grammar with tokenization methods
- **`StateStack`**: Maintains tokenization state across lines
- **`ITokenizeLineResult`**: Contains tokens with scope information

### Key Functions

- **`parseRawGrammar()`**: Parse grammar from JSON string
- **`Registry::loadGrammar()`**: Load and compile a grammar
- **`Grammar::tokenizeLine()`**: Tokenize a single line
- **`Grammar::tokenizeLine2()`**: Tokenize with binary token format

## Current Limitations

1. **Simplified Tokenization**: The core tokenization algorithm (`tokenizeString.cpp`) is simplified. It doesn't implement:
   - Complete pattern matching
   - Capture group handling
   - While/End rule matching
   - Injection processing

2. **Missing Features**:
   - PLIST grammar format support
   - Full grammar dependency resolution
   - Complete matcher implementation
   - Debug mode and logging

3. **Performance**: Not optimized - focus is on correctness and portability

## Extending the Implementation

To complete the tokenization algorithm, refer to the TypeScript implementation in `src/grammar/tokenizeString.ts` and port the `_tokenizeString()` function with:

1. Pattern compilation and caching
2. Oniguruma scanner usage with proper options
3. Capture group processing
4. Stack push/pop operations
5. Scope name resolution
6. While-condition checking

## Testing

The test suite demonstrates:
1. Oniguruma initialization
2. JSON grammar parsing
3. Registry creation
4. Grammar loading
5. Basic tokenization

Add more comprehensive tests in `textmate-cpp/tests/`.

## Contributing

When adding features:
1. Follow the TypeScript implementation closely
2. Maintain C++11 compatibility
3. Add appropriate error handling
4. Update tests
5. Document public APIs

## License

Copyright (C) Microsoft Corporation. All rights reserved.

This is a port of the original TypeScript implementation. See the main repository LICENSE file for details.
