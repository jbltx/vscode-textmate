#include "rawGrammar.h"
#include <iostream>

namespace vscode_textmate {

// IRawCapturesMap destructor
IRawCapturesMap::~IRawCapturesMap() {
    for (auto& pair : captures) {
        delete pair.second;
    }
    captures.clear();
}

// IRawRule destructor
IRawRule::~IRawRule() {
    deleteIfNotNull(id);
    deleteIfNotNull(include);
    deleteIfNotNull(name);
    deleteIfNotNull(contentName);
    deleteIfNotNull(match);
    deleteIfNotNull(captures);
    deleteIfNotNull(begin);
    deleteIfNotNull(beginCaptures);
    deleteIfNotNull(end);
    deleteIfNotNull(endCaptures);
    deleteIfNotNull(whilePattern);
    deleteIfNotNull(whileCaptures);
    deleteIfNotNull(applyEndPatternLast);

    if (patterns != nullptr) {
        for (IRawRule* rule : *patterns) {
            delete rule;
        }
        delete patterns;
        patterns = nullptr;
    }

    deleteIfNotNull(repository);
}

// IRawRepositoryMap destructor
IRawRepositoryMap::~IRawRepositoryMap() {
    std::cerr << "DEBUG: IRawRepositoryMap destructor, rules.size()=" << rules.size() << std::endl;
    for (auto& pair : rules) {
        std::cerr << "DEBUG:   Deleting rule '" << pair.first << "'" << std::endl;
        delete pair.second;
    }
    rules.clear();
    std::cerr << "DEBUG: Rules cleared" << std::endl;

    // IMPORTANT: Only delete selfRule. DO NOT delete baseRule!
    // When baseRule != selfRule, baseRule points to an external grammar's rule
    // that is owned by that grammar. Deleting it here would cause a double-free
    // when the external grammar is destroyed.
    std::cerr << "DEBUG: baseIsSameAsSelf=" << (baseRule == selfRule) << std::endl;

    std::cerr << "DEBUG: Deleting selfRule..." << std::endl;
    deleteIfNotNull(selfRule);
    std::cerr << "DEBUG: selfRule deleted" << std::endl;

    // baseRule is NOT owned by this repository, so we don't delete it
    std::cerr << "DEBUG: baseRule is not owned by this repository, not deleting" << std::endl;
    baseRule = nullptr;  // Just nullify the pointer

    std::cerr << "DEBUG: IRawRepositoryMap destructor finished" << std::endl;
}

// Helper method to get a rule by name
IRawRule* IRawRepositoryMap::getRule(const std::string& name) const {
    if (name == "$self") {
        return selfRule;
    }
    if (name == "$base") {
        return baseRule;
    }
    auto it = rules.find(name);
    if (it != rules.end()) {
        return it->second;
    }
    return nullptr;
}

// IRawGrammar destructor
IRawGrammar::~IRawGrammar() {
    std::cerr << "DEBUG: IRawGrammar destructor start" << std::endl;

    std::cerr << "DEBUG: Deleting repository..." << std::endl;
    deleteIfNotNull(repository);
    std::cerr << "DEBUG: Repository deleted" << std::endl;

    // patterns is now a pointer (inherited from IRawRule)
    if (patterns != nullptr) {
        std::cerr << "DEBUG: Deleting patterns, count=" << patterns->size() << std::endl;
        for (size_t i = 0; i < patterns->size(); i++) {
            std::cerr << "DEBUG:   Deleting pattern " << i << std::endl;
            delete (*patterns)[i];
        }
        delete patterns;
        patterns = nullptr;
        std::cerr << "DEBUG: Patterns deleted" << std::endl;
    }

    if (injections != nullptr) {
        std::cerr << "DEBUG: Deleting injections..." << std::endl;
        for (auto& pair : *injections) {
            delete pair.second;
        }
        delete injections;
        injections = nullptr;
        std::cerr << "DEBUG: Injections deleted" << std::endl;
    }

    deleteIfNotNull(injectionSelector);
    deleteIfNotNull(fileTypes);
    deleteIfNotNull(name);
    deleteIfNotNull(firstLineMatch);
    std::cerr << "DEBUG: IRawGrammar destructor finished" << std::endl;
}

} // namespace vscode_textmate
