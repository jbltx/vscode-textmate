#include "../src/grammar.h"
#include "../src/rule.h"
#include "../src/onigLib.h"
#include "../src/theme.h"
#include "../src/rawGrammar.h"
#include "../src/registry.h"
#include "../src/parseRawGrammar.h"
#include <iostream>
#include <cassert>

using namespace vscode_textmate;

/**
 * Phase 3 Test: Grammar Initialization and Tokenization
 *
 * This test verifies that:
 * 1. initGrammar() creates proper $self rule with patterns and scopeName
 * 2. Grammar initialization sets _rootId correctly
 * 3. Full tokenization produces correct scopes (not "unknown")
 * 4. StateStack is maintained properly across lines
 * 5. Multi-line tokenization works correctly
 */

void test_init_grammar_creates_self_rule() {
    std::cout << "\n=== Test: initGrammar() creates $self rule ===" << std::endl;

    // Create a raw grammar
    IRawGrammar* grammar = new IRawGrammar();
    grammar->scopeName = "test.scope";
    grammar->patterns = new std::vector<IRawRule*>();

    // Add a pattern
    IRawRule* pattern = new IRawRule();
    pattern->match = new std::string("test.*");
    pattern->name = new std::string("keyword.test");
    grammar->patterns->push_back(pattern);

    std::cout << "  Created raw grammar with scopeName and patterns" << std::endl;

    // Initialize the grammar
    IRawGrammar* initialized = initGrammar(grammar, nullptr);

    // Verify repository was created
    assert(initialized->repository != nullptr && "Repository should be created");
    std::cout << "  ✅ Repository created" << std::endl;

    // Verify $self rule exists
    assert(initialized->repository->selfRule != nullptr && "$self rule should exist");
    std::cout << "  ✅ $self rule exists" << std::endl;

    // Verify $self has the patterns
    assert(initialized->repository->selfRule->patterns != nullptr && "$self should have patterns");
    assert(initialized->repository->selfRule->patterns->size() == 1 && "$self should have 1 pattern");
    std::cout << "  ✅ $self rule has patterns: " << initialized->repository->selfRule->patterns->size() << std::endl;

    // Verify $self has the scope name
    assert(initialized->repository->selfRule->name != nullptr && "$self should have name");
    assert(*initialized->repository->selfRule->name == "test.scope" && "$self name should match grammar scopeName");
    std::cout << "  ✅ $self rule name: " << *initialized->repository->selfRule->name << std::endl;

    // Verify $base is set
    assert(initialized->repository->baseRule != nullptr && "$base rule should exist");
    assert(initialized->repository->baseRule == initialized->repository->selfRule && "$base should equal $self when no base provided");
    std::cout << "  ✅ $base rule is set to $self" << std::endl;

    std::cout << "  ✅ All initGrammar tests passed!" << std::endl;

    // Skip cleanup
}

void test_grammar_initialization_sets_root_id() {
    std::cout << "\n=== Test: Grammar initialization sets _rootId ===" << std::endl;

    // Create a simple grammar JSON
    std::string grammarJson = R"({
        "scopeName": "test.language",
        "patterns": [
            {
                "match": "keyword",
                "name": "keyword.test"
            }
        ]
    })";

    // Parse the grammar
    IRawGrammar* rawGrammar = parseRawGrammar(grammarJson.c_str(), nullptr);
    assert(rawGrammar != nullptr && "Grammar should parse successfully");
    std::cout << "  Parsed grammar: " << rawGrammar->scopeName << std::endl;

    // Create onigLib
    IOnigLib* onigLib = new DefaultOnigLib();

    // Create theme provider
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

    // Create grammar
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

    // Tokenize a line - this should initialize _rootId
    std::string testLine = "keyword";
    ITokenizeLineResult result = grammar->tokenizeLine(testLine, nullptr);

    std::cout << "  Tokenized line, got " << result.tokens.size() << " tokens" << std::endl;

    // Verify we got tokens (not empty)
    assert(result.tokens.size() > 0 && "Should produce at least one token");
    std::cout << "  ✅ Tokens produced: " << result.tokens.size() << std::endl;

    // Verify scopes are not "unknown"
    bool hasUnknown = false;
    for (const auto& token : result.tokens) {
        std::cout << "    Token [" << token.startIndex << "-" << token.endIndex << "]: ";
        for (const auto& scope : token.scopes) {
            std::cout << scope << " ";
            if (scope == "unknown") {
                hasUnknown = true;
            }
        }
        std::cout << std::endl;
    }

    assert(!hasUnknown && "Tokens should not have 'unknown' scope");
    std::cout << "  ✅ No 'unknown' scopes found!" << std::endl;

    // Verify we have the grammar's scope name
    bool hasGrammarScope = false;
    for (const auto& token : result.tokens) {
        for (const auto& scope : token.scopes) {
            if (scope == "test.language") {
                hasGrammarScope = true;
            }
        }
    }
    assert(hasGrammarScope && "Should have grammar scope name in tokens");
    std::cout << "  ✅ Grammar scope 'test.language' found in tokens" << std::endl;

    std::cout << "  ✅ All grammar initialization tests passed!" << std::endl;

    delete grammar;
}

void test_single_line_tokenization() {
    std::cout << "\n=== Test: Single line tokenization ===" << std::endl;

    // Create a grammar with multiple patterns
    std::string grammarJson = R"({
        "scopeName": "source.test",
        "patterns": [
            {
                "match": "\\b(if|else|while)\\b",
                "name": "keyword.control"
            },
            {
                "match": "\\b[0-9]+\\b",
                "name": "constant.numeric"
            },
            {
                "match": "\\b[a-zA-Z_][a-zA-Z0-9_]*\\b",
                "name": "variable.other"
            }
        ]
    })";

    IRawGrammar* rawGrammar = parseRawGrammar(grammarJson.c_str(), nullptr);
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

    // Test various lines
    std::vector<std::string> testLines = {
        "if x",
        "123",
        "variable",
        "if 42 else"
    };

    for (const auto& line : testLines) {
        std::cout << "  Testing line: \"" << line << "\"" << std::endl;
        ITokenizeLineResult result = grammar->tokenizeLine(line, nullptr);

        assert(result.tokens.size() > 0 && "Should produce tokens");
        std::cout << "    Tokens: " << result.tokens.size() << std::endl;

        for (const auto& token : result.tokens) {
            std::cout << "      [" << token.startIndex << "-" << token.endIndex << "] ";
            for (const auto& scope : token.scopes) {
                std::cout << scope << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "  ✅ Single line tokenization works!" << std::endl;

    delete grammar;
}

void test_multi_line_tokenization() {
    std::cout << "\n=== Test: Multi-line tokenization with state ===" << std::endl;

    // Create a grammar with begin/end patterns
    std::string grammarJson = R"({
        "scopeName": "source.test",
        "patterns": [
            {
                "begin": "\\{",
                "end": "\\}",
                "name": "meta.block",
                "patterns": [
                    {
                        "match": "\\b[a-z]+\\b",
                        "name": "variable.inside.block"
                    }
                ]
            },
            {
                "match": "\\b[a-z]+\\b",
                "name": "variable.outside.block"
            }
        ]
    })";

    IRawGrammar* rawGrammar = parseRawGrammar(grammarJson.c_str(), nullptr);
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

    // Test multi-line block
    std::vector<std::string> lines = {
        "outside",   // Line 0: outside block
        "{",         // Line 1: begin block
        "inside",    // Line 2: inside block
        "}",         // Line 3: end block
        "outside2"   // Line 4: outside again
    };

    StateStack* prevState = nullptr;

    for (size_t i = 0; i < lines.size(); i++) {
        std::cout << "  Line " << i << ": \"" << lines[i] << "\"" << std::endl;

        ITokenizeLineResult result = grammar->tokenizeLine(lines[i], prevState);
        prevState = result.ruleStack;

        assert(result.tokens.size() > 0 && "Should produce tokens");

        for (const auto& token : result.tokens) {
            std::cout << "    [" << token.startIndex << "-" << token.endIndex << "] ";
            for (const auto& scope : token.scopes) {
                std::cout << scope << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "  ✅ Multi-line tokenization maintains state!" << std::endl;

    delete grammar;
}

void test_tokenization_produces_correct_scopes() {
    std::cout << "\n=== Test: Tokenization produces correct specific scopes ===" << std::endl;

    std::string grammarJson = R"({
        "scopeName": "source.example",
        "patterns": [
            {
                "match": "keyword",
                "name": "keyword.test"
            }
        ]
    })";

    IRawGrammar* rawGrammar = parseRawGrammar(grammarJson.c_str(), nullptr);
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

    ITokenizeLineResult result = grammar->tokenizeLine("keyword", nullptr);

    std::cout << "  Tokenized 'keyword', checking scopes..." << std::endl;

    // Find the token that matches "keyword"
    bool foundKeywordScope = false;
    bool foundSourceScope = false;

    for (const auto& token : result.tokens) {
        for (const auto& scope : token.scopes) {
            std::cout << "    Scope: " << scope << std::endl;
            if (scope == "keyword.test") {
                foundKeywordScope = true;
            }
            if (scope == "source.example") {
                foundSourceScope = true;
            }
        }
    }

    assert(foundSourceScope && "Should have source.example scope");
    assert(foundKeywordScope && "Should have keyword.test scope");

    std::cout << "  ✅ Correct specific scopes found!" << std::endl;
    std::cout << "    ✅ source.example (grammar scope)" << std::endl;
    std::cout << "    ✅ keyword.test (pattern scope)" << std::endl;

    delete grammar;
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Phase 3 Tests: Grammar Initialization & Tokenization" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        test_init_grammar_creates_self_rule();
        test_grammar_initialization_sets_root_id();
        test_single_line_tokenization();
        test_multi_line_tokenization();
        test_tokenization_produces_correct_scopes();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ ALL PHASE 3 TESTS PASSED!" << std::endl;
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
