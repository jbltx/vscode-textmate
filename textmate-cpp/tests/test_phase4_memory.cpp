#include "../src/grammar.h"
#include "../src/rawGrammar.h"
#include "../src/parseRawGrammar.h"
#include "../src/onigLib.h"
#include "../src/theme.h"
#include <iostream>
#include <cassert>

using namespace vscode_textmate;

void test_simple_grammar_cleanup() {
    std::cout << "\n=== Test: Simple grammar cleanup ===" << std::endl;

    std::string grammarJson = R"({
        "scopeName": "test.cleanup",
        "patterns": [
            {
                "match": "keyword",
                "name": "keyword.test"
            }
        ]
    })";

    std::cout << "  Parsing grammar..." << std::endl;
    IRawGrammar* rawGrammar = parseRawGrammar(grammarJson.c_str(), nullptr);
    std::cout << "  Grammar parsed" << std::endl;

    IOnigLib* onigLib = new DefaultOnigLib();
    std::cout << "  OnigLib created" << std::endl;

    class TestThemeProvider : public IThemeProvider {
    public:
        StyleAttributes* themeMatch(ScopeStack* scopePath) override {
            return getDefaults();
        }
        StyleAttributes* getDefaults() override {
            return new StyleAttributes(0, 1, 0);
        }
    };
    TestThemeProvider* themeProvider = new TestThemeProvider();
    std::cout << "  ThemeProvider created" << std::endl;

    Grammar* grammar = new Grammar(
        rawGrammar->scopeName,
        rawGrammar,
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        themeProvider,
        onigLib
    );
    std::cout << "  Grammar created" << std::endl;

    // Tokenize a simple line
    ITokenizeLineResult result = grammar->tokenizeLine("keyword", nullptr);
    std::cout << "  Tokenized: " << result.tokens.size() << " tokens" << std::endl;

    // Now try to clean up
    std::cout << "  Deleting grammar..." << std::endl;
    std::cout << "    Calling delete grammar..." << std::endl;
    delete grammar;
    std::cout << "  ✅ Grammar deleted successfully!" << std::endl;

    std::cout << "  Deleting rawGrammar..." << std::endl;
    delete rawGrammar;
    std::cout << "  ✅ RawGrammar deleted successfully!" << std::endl;

    std::cout << "  ✅ Simple cleanup test passed!" << std::endl;

    delete onigLib;
    delete themeProvider;
}

void test_complex_grammar_cleanup() {
    std::cout << "\n=== Test: Complex grammar cleanup (5+ rules) ===" << std::endl;

    // This grammar has multiple rules with patterns, beginEnd, etc.
    std::string grammarJson = R"({
        "scopeName": "test.complex",
        "patterns": [
            {
                "match": "keyword",
                "name": "keyword.test"
            },
            {
                "match": "number",
                "name": "constant.numeric"
            },
            {
                "begin": "\"",
                "end": "\"",
                "name": "string.quoted.double",
                "patterns": [
                    {
                        "match": "\\\\.",
                        "name": "constant.character.escape"
                    }
                ]
            },
            {
                "match": "function",
                "name": "storage.type.function"
            },
            {
                "begin": "\\{",
                "end": "\\}",
                "name": "meta.block",
                "patterns": [
                    {
                        "include": "$self"
                    }
                ]
            }
        ]
    })";

    std::cout << "  Parsing complex grammar..." << std::endl;
    IRawGrammar* rawGrammar = parseRawGrammar(grammarJson.c_str(), nullptr);
    std::cout << "  Complex grammar parsed" << std::endl;

    IOnigLib* onigLib = new DefaultOnigLib();

    class TestThemeProvider : public IThemeProvider {
    public:
        StyleAttributes* themeMatch(ScopeStack* scopePath) override {
            return getDefaults();
        }
        StyleAttributes* getDefaults() override {
            return new StyleAttributes(0, 1, 0);
        }
    };
    TestThemeProvider* themeProvider = new TestThemeProvider();

    Grammar* grammar = new Grammar(
        rawGrammar->scopeName,
        rawGrammar,
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        themeProvider,
        onigLib
    );
    std::cout << "  Complex grammar created with " << grammar->getRuleCount() << " rules" << std::endl;

    // Tokenize a simple line
    ITokenizeLineResult result = grammar->tokenizeLine("keyword", nullptr);
    std::cout << "  Tokenized: " << result.tokens.size() << " tokens" << std::endl;

    // Now try to clean up - THIS IS WHERE IT HANGS
    std::cout << "  Deleting complex grammar..." << std::endl;
    std::cout << "    About to call delete on grammar..." << std::endl;
    std::cout << "    (If this hangs, the issue is in Grammar::dispose() or Rule::dispose())" << std::endl;
    delete grammar;
    std::cout << "  ✅ Complex grammar deleted successfully!" << std::endl;

    delete rawGrammar;
    std::cout << "  ✅ Complex cleanup test passed!" << std::endl;

    delete onigLib;
    delete themeProvider;
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Phase 4 Tests: Memory Management" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        test_simple_grammar_cleanup();
        test_complex_grammar_cleanup();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ ALL PHASE 4 TESTS PASSED!" << std::endl;
        std::cout << "========================================\n" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED WITH EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n❌ TEST FAILED WITH UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}
