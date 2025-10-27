#include "../src/main.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace vscode_textmate;

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    // Load grammar
    std::string grammarPath = "../../test-cases/first-mate/fixtures/javascript.json";
    std::string grammarContent = readFile(grammarPath);
    IRawGrammar* rawGrammar = parseJSONGrammar(grammarContent, nullptr);

    if (!rawGrammar) {
        std::cerr << "Failed to load grammar\n";
        return 1;
    }

    std::cout << "Grammar loaded: " << rawGrammar->scopeName << "\n";

    // Create registry and grammar
    RegistryOptions options;
    Registry registry(options);
    Grammar* grammar = registry.addGrammar(rawGrammar);

    if (!grammar) {
        std::cerr << "Failed to create grammar\n";
        return 1;
    }

    std::cout << "\n=== Test Line 0 ===\n";
    std::string line0 = "// line comment";
    std::cout << "Input: \"" << line0 << "\"\n";
    StateStack* state0 = nullptr;
    auto result0 = grammar->tokenizeLine(line0, state0, 0);
    std::cout << "Tokens:\n";
    for (const auto& token : result0.tokens) {
        std::cout << "  [" << token.startIndex << "-" << token.endIndex << "] \""
                  << line0.substr(token.startIndex, token.endIndex - token.startIndex) << "\"\n";
        std::cout << "    Scopes: ";
        for (const auto& scope : token.scopes) {
            std::cout << scope << ", ";
        }
        std::cout << "\n";
    }

    std::cout << "\n=== Test Line 1 ===\n";
    std::string line1 = " // second line comment with a single leading space";
    std::cout << "Input: \"" << line1 << "\"\n";
    std::cout << "Expected: First token should be \" \" with scope punctuation.whitespace.comment.leading.js\n";
    auto result1 = grammar->tokenizeLine(line1, result0.ruleStack, 0);
    std::cout << "Actual tokens:\n";
    for (const auto& token : result1.tokens) {
        std::cout << "  [" << token.startIndex << "-" << token.endIndex << "] \""
                  << line1.substr(token.startIndex, token.endIndex - token.startIndex) << "\"\n";
        std::cout << "    Scopes: ";
        for (const auto& scope : token.scopes) {
            std::cout << scope << ", ";
        }
        std::cout << "\n";
    }

    // Clean up
    grammar->dispose();
    delete grammar;
    delete rawGrammar;

    return 0;
}
