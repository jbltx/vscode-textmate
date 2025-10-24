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

    // Check if baseRule and selfRule point to the same object BEFORE deleting anything
    bool baseIsSameAsSelf = (baseRule == selfRule);
    std::cerr << "DEBUG: baseIsSameAsSelf=" << baseIsSameAsSelf << std::endl;

    std::cerr << "DEBUG: Deleting selfRule..." << std::endl;
    deleteIfNotNull(selfRule);
    std::cerr << "DEBUG: selfRule deleted" << std::endl;

    // Don't delete baseRule if it pointed to selfRule (avoid double-free)
    std::cerr << "DEBUG: Checking baseRule..." << std::endl;
    if (baseRule != nullptr && !baseIsSameAsSelf) {
        std::cerr << "DEBUG: Deleting baseRule (was different from selfRule)..." << std::endl;
        delete baseRule;
        baseRule = nullptr;
    } else {
        std::cerr << "DEBUG: baseRule is nullptr or was same as selfRule, skipping" << std::endl;
    }
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

    std::cerr << "DEBUG: Deleting patterns, count=" << patterns.size() << std::endl;
    for (size_t i = 0; i < patterns.size(); i++) {
        std::cerr << "DEBUG:   Deleting pattern " << i << std::endl;
        delete patterns[i];
    }
    patterns.clear();
    std::cerr << "DEBUG: Patterns deleted" << std::endl;

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
