#include "c_api.h"
#include "main.h"
#include "parseRawGrammar.h"
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>

using namespace vscode_textmate;

// Helper function to read file contents
static std::string readFileContents(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to convert std::string to C string (caller must free)
static char* stringToCString(const std::string& str) {
    char* cstr = new char[str.length() + 1];
    std::strcpy(cstr, str.c_str());
    return cstr;
}

// Initialize Oniguruma library
TEXTMATE_API TextMateOnigLib textmate_oniglib_create() {
    try {
        IOnigLib* onigLib = new DefaultOnigLib();
        return static_cast<TextMateOnigLib>(onigLib);
    } catch (...) {
        return nullptr;
    }
}

// Helper class to manage registry with internal grammar storage
class ManagedRegistry {
public:
    Registry* registry;
    std::map<std::string, IRawGrammar*> preloadedGrammars;
    std::map<std::string, std::vector<std::string>> injections;

    ManagedRegistry(IOnigLib* onigLib) {
        RegistryOptions options;
        options.onigLib = onigLib;

        // Set up loadGrammar callback to return from preloaded grammars
        options.loadGrammar = [this](const ScopeName& scopeName) -> IRawGrammar* {
            auto it = preloadedGrammars.find(scopeName);
            if (it != preloadedGrammars.end()) {
                return it->second;
            }
            return nullptr;
        };

        // Set up getInjections callback to return configured injections
        options.getInjections = [this](const ScopeName& scopeName) -> std::vector<ScopeName> {
            auto it = injections.find(scopeName);
            if (it != injections.end()) {
                return it->second;
            }
            return std::vector<ScopeName>();
        };

        registry = new Registry(options);
    }

    ~ManagedRegistry() {
        if (registry) {
            delete registry;
        }
        // Note: Don't delete preloaded grammars as they're owned by the registry now
    }
};

// Create registry with Oniguruma library
TEXTMATE_API TextMateRegistry textmate_registry_create(TextMateOnigLib onigLib) {
    try {
        ManagedRegistry* managed = new ManagedRegistry(static_cast<IOnigLib*>(onigLib));
        return static_cast<TextMateRegistry>(managed);
    } catch (...) {
        return nullptr;
    }
}

// Dispose registry
TEXTMATE_API void textmate_registry_dispose(TextMateRegistry registry) {
    if (registry) {
        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        delete managed;
    }
}

// Add grammar to registry from JSON file (does not return Grammar, just registers it)
TEXTMATE_API int textmate_registry_add_grammar_from_file(
    TextMateRegistry registry,
    const char* grammarPath
) {
    if (!registry || !grammarPath) {
        return 0;
    }

    try {
        std::string content = readFileContents(grammarPath);
        if (content.empty()) {
            return 0;
        }

        std::string pathStr = grammarPath;
        IRawGrammar* rawGrammar = parseRawGrammar(content, &pathStr);
        if (!rawGrammar) {
            return 0;
        }

        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        managed->preloadedGrammars[rawGrammar->scopeName] = rawGrammar;

        return 1; // Success
    } catch (...) {
        return 0;
    }
}

// Add grammar to registry from JSON string (does not return Grammar, just registers it)
TEXTMATE_API int textmate_registry_add_grammar_from_json(
    TextMateRegistry registry,
    const char* jsonContent
) {
    if (!registry || !jsonContent) {
        return 0;
    }

    try {
        IRawGrammar* rawGrammar = parseRawGrammar(jsonContent, nullptr);
        if (!rawGrammar) {
            return 0;
        }

        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        managed->preloadedGrammars[rawGrammar->scopeName] = rawGrammar;

        return 1; // Success
    } catch (...) {
        return 0;
    }
}

// Set grammar injections for a scope (call before loading the grammar)
TEXTMATE_API void textmate_registry_set_injections(
    TextMateRegistry registry,
    const char* scopeName,
    const char** injections,
    int32_t injectionCount
) {
    if (!registry || !scopeName || !injections) {
        return;
    }

    try {
        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        std::vector<std::string> injectionsList;
        for (int32_t i = 0; i < injectionCount; i++) {
            if (injections[i]) {
                injectionsList.push_back(injections[i]);
            }
        }
        managed->injections[scopeName] = injectionsList;
    } catch (...) {
        // Ignore errors
    }
}

// Load grammar by scope name (after grammars have been added to registry)
TEXTMATE_API TextMateGrammar textmate_registry_load_grammar(
    TextMateRegistry registry,
    const char* scopeName
) {
    if (!registry || !scopeName) {
        return nullptr;
    }

    try {
        ManagedRegistry* managed = static_cast<ManagedRegistry*>(registry);
        Grammar* grammar = managed->registry->loadGrammar(scopeName);
        return static_cast<TextMateGrammar>(grammar);
    } catch (...) {
        return nullptr;
    }
}

// Get INITIAL state
TEXTMATE_API TextMateStateStack textmate_get_initial_state() {
    return static_cast<TextMateStateStack>(const_cast<StateStack*>(INITIAL));
}

// Tokenize a line of text
TEXTMATE_API TextMateTokenizeResult* textmate_tokenize_line(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
) {
    if (!grammar || !lineText) {
        return nullptr;
    }

    try {
        Grammar* gram = static_cast<Grammar*>(grammar);
        StateStack* state = static_cast<StateStack*>(prevState);

        ITokenizeLineResult result = gram->tokenizeLine(lineText, state);

        // Allocate result structure
        TextMateTokenizeResult* cResult = new TextMateTokenizeResult();
        cResult->tokenCount = result.tokens.size();
        cResult->tokens = new TextMateToken[cResult->tokenCount];
        cResult->ruleStack = static_cast<TextMateStateStack>(result.ruleStack);
        cResult->stoppedEarly = result.stoppedEarly ? 1 : 0;

        // Convert tokens
        for (int i = 0; i < cResult->tokenCount; i++) {
            const IToken& token = result.tokens[i];
            cResult->tokens[i].startIndex = token.startIndex;
            cResult->tokens[i].endIndex = token.endIndex;
            cResult->tokens[i].scopeDepth = token.scopes.size();

            // Allocate scope strings
            cResult->tokens[i].scopes = new char*[token.scopes.size()];
            for (size_t j = 0; j < token.scopes.size(); j++) {
                cResult->tokens[i].scopes[j] = stringToCString(token.scopes[j]);
            }
        }

        return cResult;
    } catch (...) {
        return nullptr;
    }
}

// Tokenize a line of text with encoded tokens
TEXTMATE_API TextMateTokenizeResult2* textmate_tokenize_line2(
    TextMateGrammar grammar,
    const char* lineText,
    TextMateStateStack prevState
) {
    if (!grammar || !lineText) {
        return nullptr;
    }

    try {
        Grammar* gram = static_cast<Grammar*>(grammar);
        StateStack* state = static_cast<StateStack*>(prevState);

        ITokenizeLineResult2 result = gram->tokenizeLine2(lineText, state);

        // Allocate result structure
        TextMateTokenizeResult2* cResult = new TextMateTokenizeResult2();
        cResult->tokenCount = result.tokens.size();
        cResult->tokens = new uint32_t[cResult->tokenCount];
        cResult->ruleStack = static_cast<TextMateStateStack>(result.ruleStack);
        cResult->stoppedEarly = result.stoppedEarly ? 1 : 0;

        // Copy tokens
        for (int i = 0; i < cResult->tokenCount; i++) {
            cResult->tokens[i] = result.tokens[i];
        }

        return cResult;
    } catch (...) {
        return nullptr;
    }
}

// Free tokenize result
TEXTMATE_API void textmate_free_tokenize_result(TextMateTokenizeResult* result) {
    if (result) {
        if (result->tokens) {
            for (int i = 0; i < result->tokenCount; i++) {
                if (result->tokens[i].scopes) {
                    for (int j = 0; j < result->tokens[i].scopeDepth; j++) {
                        delete[] result->tokens[i].scopes[j];
                    }
                    delete[] result->tokens[i].scopes;
                }
            }
            delete[] result->tokens;
        }
        delete result;
    }
}

// Free tokenize result2
TEXTMATE_API void textmate_free_tokenize_result2(TextMateTokenizeResult2* result) {
    if (result) {
        if (result->tokens) {
            delete[] result->tokens;
        }
        delete result;
    }
}

// Get scope name from grammar
TEXTMATE_API const char* textmate_grammar_get_scope_name(TextMateGrammar grammar) {
    // Not implemented yet - would require adding a getter method to Grammar class
    // For now, return nullptr
    return nullptr;
}

// Dispose grammar (Note: Grammar lifecycle is managed by Registry in the C++ implementation)
TEXTMATE_API void textmate_grammar_dispose(TextMateGrammar grammar) {
    // In the current C++ implementation, Grammar objects are owned by Registry
    // so we don't delete them here. This function is provided for API consistency
    // but doesn't do anything. If you want independent Grammar lifecycle,
    // you'll need to modify the C++ implementation.
}

// Dispose Oniguruma library
TEXTMATE_API void textmate_oniglib_dispose(TextMateOnigLib onigLib) {
    if (onigLib) {
        IOnigLib* lib = static_cast<IOnigLib*>(onigLib);
        delete lib;
    }
}
