#ifndef VSCODE_TEXTMATE_MATCHER_H
#define VSCODE_TEXTMATE_MATCHER_H

#include "types.h"
#include <string>
#include <vector>
#include <functional>

namespace vscode_textmate {

// Matcher function type
template<typename T>
using Matcher = std::function<bool(const T&)>;

// MatcherWithPriority structure
template<typename T>
struct MatcherWithPriority {
    Matcher<T> matcher;
    int priority; // -1, 0, or 1

    MatcherWithPriority() : priority(0) {}
    MatcherWithPriority(const Matcher<T>& m, int p) : matcher(m), priority(p) {}
};

// Create matchers from a selector string
template<typename T>
std::vector<MatcherWithPriority<T>> createMatchers(
    const std::string& selector,
    std::function<bool(const std::vector<std::string>&, const T&)> matchesName
);

// Helper functions
bool isIdentifier(const std::string& token);

// Tokenizer class
class SelectorTokenizer {
private:
    std::string _input;
    size_t _position;

public:
    explicit SelectorTokenizer(const std::string& input);
    std::string* next();
};

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_MATCHER_H
