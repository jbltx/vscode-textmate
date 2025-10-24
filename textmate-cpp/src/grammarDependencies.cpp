#include "grammarDependencies.h"
#include "registry.h"

namespace vscode_textmate {

IncludeReference parseInclude(const std::string& include) {
    if (include == "$base" || include == "$self") {
        return IncludeReference(
            include == "$base" ? IncludeReferenceKind::Base : IncludeReferenceKind::Self
        );
    }

    if (include[0] == '#') {
        // Relative reference: #ruleName
        return IncludeReference(
            IncludeReferenceKind::RelativeReference,
            "",
            include.substr(1)
        );
    }

    size_t sharpIndex = include.find('#');
    if (sharpIndex != std::string::npos) {
        // Top level repository reference: scopeName#ruleName
        return IncludeReference(
            IncludeReferenceKind::TopLevelRepositoryReference,
            include.substr(0, sharpIndex),
            include.substr(sharpIndex + 1)
        );
    }

    // Top level reference: scopeName
    return IncludeReference(
        IncludeReferenceKind::TopLevelReference,
        include,
        ""
    );
}

ScopeDependencyProcessor::ScopeDependencyProcessor(SyncRegistry* repo, const std::string& initialScopeName)
    : _repo(repo), _initialScopeName(initialScopeName) {
    Q.push(AbsoluteRuleReference(initialScopeName, ""));
}

void ScopeDependencyProcessor::processQueue() {
    // Simplified implementation
    // In full version, this would process dependencies and add to queue
    while (!Q.empty()) {
        Q.pop();
    }
}

} // namespace vscode_textmate
