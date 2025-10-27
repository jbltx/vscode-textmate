#include "../src/main.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

using namespace vscode_textmate;

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Grammar holder - loads and holds grammars for the test
class GrammarHolder {
private:
    std::string basePath;
    std::map<std::string, IRawGrammar*> grammarByScope;

public:
    GrammarHolder(const std::string& path) : basePath(path) {}

    ~GrammarHolder() {
        for (auto& pair : grammarByScope) {
            delete pair.second;
        }
    }

    std::string loadGrammar(const std::string& grammarPath) {
        try {
            std::string fullPath = basePath + "/" + grammarPath;
            std::string grammarContent = readFile(fullPath);

            if (grammarPath.find(".json") != std::string::npos) {
                IRawGrammar* grammar = parseJSONGrammar(grammarContent, nullptr);
                if (grammar) {
                    std::string scopeName = grammar->scopeName;
                    grammarByScope[scopeName] = grammar;
                    return scopeName;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading grammar " << grammarPath << ": " << e.what() << std::endl;
        }
        return "";
    }

    IRawGrammar* getGrammarByScope(const std::string& scopeName) {
        auto it = grammarByScope.find(scopeName);
        if (it != grammarByScope.end()) {
            return it->second;
        }
        return nullptr;
    }
};

int main() {
    // Create onig lib
    DefaultOnigLib* onigLib = new DefaultOnigLib();

    // Create grammar holder
    GrammarHolder holder("../../test-cases/first-mate");

    // Load all necessary grammars for PHP
    holder.loadGrammar("fixtures/php.json");
    holder.loadGrammar("fixtures/html.json");

    // Create registry with loadGrammar callback
    RegistryOptions options;
    options.onigLib = onigLib;
    options.loadGrammar = [&holder](const std::string& scopeName) -> IRawGrammar* {
        return holder.getGrammarByScope(scopeName);
    };
    options.getInjections = nullptr;

    Registry registry(options);

    // Load the main grammar
    Grammar* grammar = registry.loadGrammar("text.html.php");

    if (!grammar) {
        std::cerr << "Failed to load grammar\n";
        return 1;
    }

    std::cout << "\n=== TEST #42: PHP Function ===\n";
    std::string line = "<?php public final function meth() {} ?>";
    std::cout << "Input: \"" << line << "\"\n";
    std::cout << "Expected: 17 tokens\n\n";

    std::cout << "DEBUG: About to call tokenizeLine...\n";
    std::cout.flush();

    StateStack* state = nullptr;
    ITokenizeLineResult result;

    try {
        result = grammar->tokenizeLine(line, state);
        std::cout << "DEBUG: tokenizeLine returned successfully\n";
        std::cout.flush();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during tokenizeLine: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "ERROR: Unknown exception during tokenizeLine\n";
        return 1;
    }

    std::cout << "Actual: " << result.tokens.size() << " tokens\n\n";
    std::cout << "Tokens:\n";
    for (size_t i = 0; i < result.tokens.size(); i++) {
        const auto& token = result.tokens[i];
        std::string value = line.substr(token.startIndex, token.endIndex - token.startIndex);
        std::cout << "  [" << i << "] value=\"" << value << "\" (length=" << value.length() << ")\n";
        std::cout << "      scopes: ";
        for (size_t j = 0; j < token.scopes.size(); j++) {
            if (j > 0) std::cout << ", ";
            std::cout << token.scopes[j];
        }
        std::cout << "\n";
    }

    std::cout << "DEBUG: Test completed successfully\n";

    // Cleanup
    delete onigLib;

    return 0;
}
