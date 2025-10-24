#include "../src/main.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace vscode_textmate;
using namespace rapidjson;

// Helper function to read file content
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to parse JSON file
Document parseJSONFile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    IStreamWrapper isw(ifs);
    Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError()) {
        throw std::runtime_error("JSON parse error in file: " + filename);
    }

    return doc;
}

// Convert JSON array to vector of strings (for scopes)
std::vector<std::string> jsonArrayToStringVector(const Value& arr) {
    std::vector<std::string> result;
    if (!arr.IsArray()) return result;

    for (SizeType i = 0; i < arr.Size(); i++) {
        if (arr[i].IsString()) {
            result.push_back(arr[i].GetString());
        }
    }
    return result;
}

// Compare two scope arrays
bool compareScopesArrays(const std::vector<std::string>& expected, const std::vector<std::string>& actual) {
    if (expected.size() != actual.size()) {
        return false;
    }

    for (size_t i = 0; i < expected.size(); i++) {
        if (expected[i] != actual[i]) {
            return false;
        }
    }

    return true;
}

// Grammar cache
class GrammarCache {
private:
    std::map<std::string, IRawGrammar*> grammars;
    std::string basePath;

public:
    GrammarCache(const std::string& path) : basePath(path) {}

    ~GrammarCache() {
        for (auto& pair : grammars) {
            delete pair.second;
        }
    }

    IRawGrammar* loadGrammar(const std::string& grammarPath) {
        // Check cache first
        if (grammars.find(grammarPath) != grammars.end()) {
            return grammars[grammarPath];
        }

        // Load grammar from file
        try {
            std::string fullPath = basePath + "/" + grammarPath;
            std::string grammarContent = readFile(fullPath);

            IRawGrammar* grammar = nullptr;
            // Only JSON grammars are supported in C++ implementation
            if (grammarPath.find(".json") != std::string::npos) {
                grammar = parseJSONGrammar(grammarContent, nullptr);
            } else {
                std::cerr << "Warning: Only JSON grammars are supported, skipping " << grammarPath << std::endl;
            }

            if (grammar) {
                grammars[grammarPath] = grammar;
                return grammar;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading grammar " << grammarPath << ": " << e.what() << std::endl;
        }

        return nullptr;
    }

    IRawGrammar* getGrammar(const std::string& scopeName) {
        for (auto& pair : grammars) {
            if (pair.second && pair.second->scopeName == scopeName) {
                return pair.second;
            }
        }
        return nullptr;
    }

    void loadGrammars(const Value& grammarList) {
        if (!grammarList.IsArray()) return;

        for (SizeType i = 0; i < grammarList.Size(); i++) {
            if (grammarList[i].IsString()) {
                loadGrammar(grammarList[i].GetString());
            }
        }
    }
};

// Test a single line
bool testLine(Grammar* grammar,
              const std::string& line,
              const Value& expectedTokens,
              StateStack* prevState,
              StateStack** outState,
              int& tokensChecked) {

    ITokenizeLineResult result = grammar->tokenizeLine(line, prevState);

    if (outState) {
        *outState = result.ruleStack;
    }

    // Build a map of expected tokens by their value (text content)
    std::map<std::string, std::vector<std::string>> expectedTokenMap;

    for (SizeType i = 0; i < expectedTokens.Size(); i++) {
        const Value& token = expectedTokens[i];
        if (token.HasMember("value") && token.HasMember("scopes")) {
            std::string value = token["value"].GetString();
            std::vector<std::string> scopes = jsonArrayToStringVector(token["scopes"]);
            expectedTokenMap[value] = scopes;
        }
    }

    // Compare tokens
    for (const auto& token : result.tokens) {
        std::string tokenText = line.substr(token.startIndex, token.endIndex - token.startIndex);

        auto it = expectedTokenMap.find(tokenText);
        if (it != expectedTokenMap.end()) {
            if (!compareScopesArrays(it->second, token.scopes)) {
                std::cerr << "      Token \"" << tokenText << "\" has wrong scopes" << std::endl;
                std::cerr << "        Expected: ";
                for (const auto& scope : it->second) {
                    std::cerr << scope << " ";
                }
                std::cerr << std::endl;
                std::cerr << "        Actual: ";
                for (const auto& scope : token.scopes) {
                    std::cerr << scope << " ";
                }
                std::cerr << std::endl;
                return false;
            }
            tokensChecked++;
        }
    }

    return true;
}

// Run a single test case
bool runTestCase(const Value& testCase, GrammarCache& cache, IOnigLib* onigLib, int testNum) {
    std::string desc = testCase.HasMember("desc") ? testCase["desc"].GetString() : "Unknown";

    std::cout << "Running " << desc << "..." << std::endl;

    try {
        // Load all required grammars
        std::cout << "  Loading grammars..." << std::endl;
        if (testCase.HasMember("grammars")) {
            cache.loadGrammars(testCase["grammars"]);
        }
        std::cout << "  Grammars loaded" << std::endl;

        // Get the main grammar
        IRawGrammar* mainGrammar = nullptr;

        if (testCase.HasMember("grammarPath")) {
            std::string grammarPath = testCase["grammarPath"].GetString();
            std::cout << "  Loading main grammar from path: " << grammarPath << std::endl;
            mainGrammar = cache.loadGrammar(grammarPath);
        } else if (testCase.HasMember("grammarScopeName")) {
            std::string scopeName = testCase["grammarScopeName"].GetString();
            std::cout << "  Loading main grammar by scope: " << scopeName << std::endl;
            mainGrammar = cache.getGrammar(scopeName);
        }

        if (!mainGrammar) {
            std::cerr << "  FAILED: Could not load main grammar" << std::endl;
            return false;
        }

        std::cout << "  Main grammar loaded: " << mainGrammar->scopeName << std::endl;

        // Create registry
        std::cout << "  Creating registry..." << std::endl;
        RegistryOptions options;
        options.onigLib = onigLib;
        options.loadGrammar = [&cache](const ScopeName& scopeName) -> IRawGrammar* {
            return cache.getGrammar(scopeName);
        };

        Registry registry(options);
        std::cout << "  Registry created" << std::endl;

        std::cout << "  Adding grammar to registry..." << std::endl;
        Grammar* grammar = registry.addGrammar(mainGrammar);
        std::cout << "  Grammar added" << std::endl;

        if (!grammar) {
            std::cerr << "  FAILED: Could not create grammar object" << std::endl;
            return false;
        }

        // Process grammar injections if specified
        if (testCase.HasMember("grammarInjections")) {
            const Value& injections = testCase["grammarInjections"];
            if (injections.IsArray()) {
                for (SizeType i = 0; i < injections.Size(); i++) {
                    if (injections[i].IsString()) {
                        std::string injectScope = injections[i].GetString();
                        IRawGrammar* injectGrammar = cache.getGrammar(injectScope);
                        if (injectGrammar) {
                            // Add injection grammar to registry
                            registry.addGrammar(injectGrammar);
                        }
                    }
                }
            }
        }

        // Test each line
        if (!testCase.HasMember("lines")) {
            std::cerr << "  FAILED: No lines in test case" << std::endl;
            return false;
        }

        const Value& lines = testCase["lines"];
        StateStack* prevState = nullptr;
        int totalTokensChecked = 0;

        std::cout << "  Testing " << lines.Size() << " lines..." << std::endl;

        for (SizeType i = 0; i < lines.Size(); i++) {
            const Value& lineTest = lines[i];

            if (!lineTest.HasMember("line") || !lineTest.HasMember("tokens")) {
                std::cerr << "  FAILED: Invalid line test at index " << i << std::endl;
                return false;
            }

            std::string line = lineTest["line"].GetString();
            const Value& expectedTokens = lineTest["tokens"];

            std::cout << "    Line " << i << ": \"" << line << "\"" << std::endl;

            StateStack* newState = nullptr;
            int tokensChecked = 0;

            if (!testLine(grammar, line, expectedTokens, prevState, &newState, tokensChecked)) {
                std::cerr << "  FAILED at line " << i << ": \"" << line << "\"" << std::endl;
                return false;
            }

            totalTokensChecked += tokensChecked;
            prevState = newState;
        }

        std::cout << "  PASSED (" << totalTokensChecked << " tokens checked)" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "  FAILED with exception: " << e.what() << std::endl;
        return false;
    }
}

int main(int argc, char** argv) {
    std::cout << "vscode-textmate C++ First-Mate Tests" << std::endl;
    std::cout << "=====================================" << std::endl << std::endl;

    try {
        // Determine paths (relative to build directory where test is run from)
        std::string testCasesPath = "../../test-cases/first-mate";
        std::string testsJsonPath = testCasesPath + "/tests.json";

        // Allow override via command line
        if (argc > 1) {
            testsJsonPath = argv[1];
        }

        std::cout << "Loading tests from: " << testsJsonPath << std::endl << std::endl;

        // Parse tests.json
        Document testsDoc = parseJSONFile(testsJsonPath);

        if (!testsDoc.IsArray()) {
            std::cerr << "ERROR: tests.json is not an array" << std::endl;
            return 1;
        }

        std::cout << "Found " << testsDoc.Size() << " test cases" << std::endl << std::endl;

        // Initialize Oniguruma
        IOnigLib* onigLib = new DefaultOnigLib();

        // Create grammar cache
        GrammarCache cache(testCasesPath);

        // Run all tests
        int passed = 0;
        int failed = 0;

        for (SizeType i = 0; i < testsDoc.Size(); i++) {
            std::cout << "Test #" << (i + 1) << ": ";

            if (runTestCase(testsDoc[i], cache, onigLib, i + 1)) {
                passed++;
            } else {
                failed++;
            }

            std::cout << std::endl;
        }

        // Summary
        std::cout << "=====================================" << std::endl;
        std::cout << "Tests passed: " << passed << "/" << testsDoc.Size() << std::endl;
        std::cout << "Tests failed: " << failed << "/" << testsDoc.Size() << std::endl;
        std::cout << "=====================================" << std::endl;

        // Cleanup
        delete onigLib;

        return (failed == 0) ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
}
