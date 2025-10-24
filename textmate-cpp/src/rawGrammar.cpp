#include "rawGrammar.h"

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
    for (auto& pair : rules) {
        delete pair.second;
    }
    rules.clear();

    deleteIfNotNull(selfRule);
    deleteIfNotNull(baseRule);
}

// IRawGrammar destructor
IRawGrammar::~IRawGrammar() {
    deleteIfNotNull(repository);

    for (IRawRule* rule : patterns) {
        delete rule;
    }
    patterns.clear();

    if (injections != nullptr) {
        for (auto& pair : *injections) {
            delete pair.second;
        }
        delete injections;
        injections = nullptr;
    }

    deleteIfNotNull(injectionSelector);
    deleteIfNotNull(fileTypes);
    deleteIfNotNull(name);
    deleteIfNotNull(firstLineMatch);
}

} // namespace vscode_textmate
