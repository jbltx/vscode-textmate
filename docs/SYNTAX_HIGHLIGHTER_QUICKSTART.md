# SyntaxHighlighter Quick Start Guide

## 5-Minute Setup

### Step 1: Include Headers

```cpp
#include "syntax_highlighter.h"
#include "registry.h"
#include "theme.h"
```

### Step 2: Load Grammar and Theme

```cpp
using namespace vscode_textmate;

// Create registry and load grammar
auto registry = std::make_shared<Registry>();
auto grammar = registry->loadGrammar("source.javascript");

// Load theme from file or create from raw data
auto theme = Theme::createFromFile("themes/monokai.json");
if (!theme) {
    throw std::runtime_error("Failed to load theme");
}
```

### Step 3: Create Highlighter

```cpp
auto highlighter = std::make_shared<SyntaxHighlighter>(grammar, theme);
```

### Step 4: Set Document

```cpp
std::vector<std::string> code = {
    "const x = 5;",
    "console.log(x);"
};
highlighter->setDocument(code);
```

### Step 5: Get Highlighted Content

```cpp
auto highlighted = highlighter->getHighlightedLine(0);

for (const auto& token : highlighted.tokens) {
    std::cout << "Text: '" << highlighted.content.substr(
        token.startIndex, token.endIndex - token.startIndex)
        << "'" << std::endl;
    std::cout << "  Scope: " << token.scopes[0] << std::endl;
    std::cout << "  Color: " << token.foregroundColor << std::endl;
    std::cout << "  Bold: " << ((token.fontStyle & 2) ? "yes" : "no") << std::endl;
}
```

## Complete Working Example

```cpp
#include "syntax_highlighter.h"
#include "registry.h"
#include "theme.h"
#include <iostream>
#include <iomanip>

using namespace vscode_textmate;

void printHighlightedLine(const HighlightedLine& line) {
    std::cout << std::setw(40) << std::setfill('=') << "=" << std::endl;
    std::cout << "Line " << line.lineIndex << ": " << line.content << std::endl;
    std::cout << std::setw(40) << std::setfill('=') << "=" << std::endl;

    for (size_t i = 0; i < line.tokens.size(); ++i) {
        const auto& token = line.tokens[i];
        std::string text = line.content.substr(
            token.startIndex,
            token.endIndex - token.startIndex
        );

        std::cout << "Token #" << i << ":" << std::endl;
        std::cout << "  Text:          '" << text << "'" << std::endl;
        std::cout << "  Range:         [" << token.startIndex << "-"
                  << token.endIndex << "]" << std::endl;
        std::cout << "  Scope:         " << token.scopes[0] << std::endl;
        std::cout << "  Color:         " << token.foregroundColor << std::endl;
        std::cout << "  Font Style:    ";

        if (token.fontStyle & (int)FontStyle::Bold) {
            std::cout << "Bold ";
        }
        if (token.fontStyle & (int)FontStyle::Italic) {
            std::cout << "Italic ";
        }
        if (token.fontStyle & (int)FontStyle::Underline) {
            std::cout << "Underline ";
        }
        if (token.fontStyle & (int)FontStyle::Strikethrough) {
            std::cout << "Strikethrough ";
        }
        if (token.fontStyle == (int)FontStyle::None) {
            std::cout << "(none)";
        }
        std::cout << std::endl;

        std::cout << "  Token Type:    ";
        switch (token.tokenType) {
            case StandardTokenType::Comment:
                std::cout << "Comment"; break;
            case StandardTokenType::String:
                std::cout << "String"; break;
            case StandardTokenType::RegEx:
                std::cout << "RegEx"; break;
            default:
                std::cout << "Other";
        }
        std::cout << std::endl << std::endl;
    }
}

int main() {
    try {
        // Load grammar and theme
        auto registry = std::make_shared<Registry>();
        auto grammar = registry->loadGrammar("source.javascript");

        if (!grammar) {
            std::cerr << "Failed to load JavaScript grammar" << std::endl;
            return 1;
        }

        auto theme = Theme::createFromFile("themes/monokai.json");
        if (!theme) {
            std::cerr << "Failed to load theme" << std::endl;
            return 1;
        }

        // Create highlighter
        auto highlighter = std::make_shared<SyntaxHighlighter>(grammar, theme);

        // Sample JavaScript code
        std::vector<std::string> code = {
            "// Fibonacci function",
            "function fibonacci(n) {",
            "    if (n <= 1) return n;",
            "    return fibonacci(n-1) + fibonacci(n-2);",
            "}",
            "",
            "const result = fibonacci(10);",
            "console.log(result);  // Output: 55"
        };

        // Highlight entire document
        highlighter->setDocument(code);

        // Print first few lines
        for (int i = 0; i < 3; ++i) {
            auto line = highlighter->getHighlightedLine(i);
            printHighlightedLine(line);
        }

        // Demo: Edit a line and see highlighting update
        std::cout << "\n[Editing line 6...]\n" << std::endl;
        highlighter->editLine(6, "const result = fibonacci(15);");
        auto editedLine = highlighter->getHighlightedLine(6);
        printHighlightedLine(editedLine);

        // Get metadata
        auto metadata = highlighter->getMetadata();
        std::cout << "\nMetadata:" << std::endl;
        std::cout << "  Total Lines: " << metadata.lineCount << std::endl;
        std::cout << "  Cached Lines: " << metadata.cachedLineCount << std::endl;
        std::cout << "  Theme Colors: " << metadata.themeColorCount << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

## Common Operations

### Query Multiple Lines

```cpp
// Get lines 0-9
auto lines = highlighter->getHighlightedRange(0, 9);
for (const auto& line : lines) {
    renderLine(line);
}
```

### Edit Document

```cpp
// Edit single line
highlighter->editLine(5, "const y = 20;");

// Insert lines
std::vector<std::string> newLines = {"if (true) {", "    return 5;"};
highlighter->insertLines(3, newLines);

// Remove lines
highlighter->removeLines(3, 2);
```

### Switch Theme

```cpp
auto darkTheme = Theme::createFromFile("themes/dark.json");
highlighter->setTheme(darkTheme);

// Now all queries return colors from new theme
auto line = highlighter->getHighlightedLine(0);
```

### Enable/Disable Cache

```cpp
// With cache (default, faster)
auto highlighter1 = std::make_shared<SyntaxHighlighter>(grammar, theme, true);

// Without cache (memory efficient)
auto highlighter2 = std::make_shared<SyntaxHighlighter>(grammar, theme, false);
```

## Integration Patterns

### With Text Editor UI

```cpp
class EditorView {
    std::shared_ptr<SyntaxHighlighter> highlighter;

    void renderDocument() {
        int startLine = viewport.getStartLine();
        int endLine = viewport.getEndLine();

        auto lines = highlighter->getHighlightedRange(startLine, endLine);
        for (const auto& line : lines) {
            renderLineWithHighlighting(line);
        }
    }

    void handleEdit(int line, const std::string& newContent) {
        highlighter->editLine(line, newContent);
        renderDocument();
    }
};
```

### With Language Server

```cpp
class LSPServer {
    std::map<std::string, std::shared_ptr<SyntaxHighlighter>> documents;

    void onDidOpenTextDocument(const std::string& uri,
                              const std::string& text) {
        auto lines = splitIntoLines(text);
        auto highlighter = std::make_shared<SyntaxHighlighter>(grammar, theme);
        highlighter->setDocument(lines);
        documents[uri] = highlighter;
    }

    void onDidChangeTextDocument(const std::string& uri,
                                int line,
                                const std::string& newContent) {
        auto it = documents.find(uri);
        if (it != documents.end()) {
            it->second->editLine(line, newContent);
        }
    }
};
```

## Performance Tips

### For Large Documents

```cpp
// Disable cache for memory-constrained systems
auto highlighter = std::make_shared<SyntaxHighlighter>(
    grammar, theme, false);

// Or use batch queries instead of single-line queries
auto lines = highlighter->getHighlightedRange(0, 999);  // Better than 1000 single queries
```

### For Frequent Theme Switching

```cpp
// Pre-load themes
std::map<std::string, Theme*> themes;
themes["light"] = Theme::createFromFile("themes/light.json");
themes["dark"] = Theme::createFromFile("themes/dark.json");

// Switch is now O(1)
highlighter->setTheme(themes["dark"]);
```

### For Real-Time Editing

```cpp
// Use incremental edits instead of full document reload
highlighter->editLine(currentLine, newContent);  // Fast: ~5-50ms

// Instead of:
std::vector<std::string> allLines = getFullDocument();
highlighter->setDocument(allLines);  // Slow: ~500ms+ for large docs
```

## Troubleshooting

### Colors Not Showing

```cpp
// Check if theme loaded successfully
if (!theme) {
    std::cerr << "Theme failed to load" << std::endl;
}

// Verify token has colors
auto line = highlighter->getHighlightedLine(0);
if (line.tokens.empty()) {
    std::cerr << "No tokens found" << std::endl;
}

if (line.tokens[0].foregroundColor.empty()) {
    std::cerr << "No color for token" << std::endl;
}
```

### Incorrect Tokenization

```cpp
// Check token scopes
auto line = highlighter->getHighlightedLine(0);
std::cout << "First token scope: " << line.tokens[0].scopes[0] << std::endl;

// Compare with expected scope
if (line.tokens[0].scopes[0] != "keyword.control") {
    std::cerr << "Unexpected scope" << std::endl;
}
```

## Next Steps

1. Read [SYNTAX_HIGHLIGHTER_README.md](../src/SYNTAX_HIGHLIGHTER_README.md) for complete API documentation
2. Check [SESSION_API_README.md](../src/SESSION_API_README.md) for incremental tokenization details
3. Explore example code in `examples/` directory
4. Run tests: `./tests/test_syntax_highlighter`

---

**Quick Reference:**

| Task | Code |
|------|------|
| Create | `SyntaxHighlighter(grammar, theme)` |
| Set doc | `highlighter->setDocument(lines)` |
| Get line | `highlighter->getHighlightedLine(0)` |
| Edit | `highlighter->editLine(0, "new content")` |
| Switch theme | `highlighter->setTheme(newTheme)` |
| Metadata | `highlighter->getMetadata()` |
| Clear cache | `highlighter->clearCache()` |
