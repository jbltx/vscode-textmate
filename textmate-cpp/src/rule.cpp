#include "rule.h"
#include "grammarDependencies.h"
#include <stdexcept>

namespace vscode_textmate {

// CompiledRule implementation

CompiledRule::CompiledRule() : scanner(nullptr) {}

CompiledRule::~CompiledRule() {
    dispose();
}

void CompiledRule::dispose() {
    if (scanner != nullptr) {
        scanner->dispose();
        delete scanner;
        scanner = nullptr;
    }
}

// RegExpSourceList implementation

RegExpSourceList::RegExpSourceList()
    : _hasAnchors(false), _cached(nullptr) {
}

RegExpSourceList::~RegExpSourceList() {
    dispose();
}

void RegExpSourceList::push(RegexSource* item) {
    _items.push_back(item);
    if (item->hasAnchor) {
        _hasAnchors = true;
    }
}

void RegExpSourceList::unshift(RegexSource* item) {
    _items.insert(_items.begin(), item);
    if (item->hasAnchor) {
        _hasAnchors = true;
    }
}

void RegExpSourceList::setSource(int index, const std::string& newSource) {
    if (index >= 0 && index < static_cast<int>(_items.size())) {
        _items[index]->source = newSource;
    }
}

int RegExpSourceList::length() const {
    return _items.size();
}

CompiledRule* RegExpSourceList::compile(IOnigLib* onigLib) {
    return compileAG(onigLib, false, false);
}

CompiledRule* RegExpSourceList::compileAG(IOnigLib* onigLib, bool allowA, bool allowG) {
    if (!_cached) {
        std::vector<std::string> sources;
        for (auto* item : _items) {
            sources.push_back(item->source);
        }

        _cached = new CompiledRule();
        _cached->scanner = onigLib->createOnigScanner(sources);

        for (auto* item : _items) {
            _cached->rules.push_back(item->ruleId);
        }
    }
    return _cached;
}

void RegExpSourceList::dispose() {
    if (_cached) {
        _cached->dispose();
        delete _cached;
        _cached = nullptr;
    }
}

// Rule base class implementation

Rule::Rule(ILocation* location_, RuleId id_,
          const std::string* name, const std::string* contentName)
    : location(location_), id(id_),
      _name(name ? new std::string(*name) : nullptr),
      _contentName(contentName ? new std::string(*contentName) : nullptr) {

    _nameIsCapturing = RegexSource::hasCaptures(_name);
    _contentNameIsCapturing = RegexSource::hasCaptures(_contentName);
}

Rule::~Rule() {
    delete location;
    delete _name;
    delete _contentName;
}

std::string Rule::getDebugName() const {
    std::string loc = location ? (basename(location->filename) + ":" + std::to_string(location->line)) : "unknown";
    return "Rule#" + std::to_string(ruleIdToNumber(id)) + " @ " + loc;
}

std::string* Rule::getName(const std::string* lineText,
                           const std::vector<IOnigCaptureIndex>* captureIndices) const {
    if (!_nameIsCapturing || _name == nullptr || lineText == nullptr || captureIndices == nullptr) {
        return _name ? new std::string(*_name) : nullptr;
    }
    return new std::string(RegexSource::replaceCaptures(*_name, *lineText, *captureIndices));
}

std::string* Rule::getContentName(const std::string& lineText,
                                  const std::vector<IOnigCaptureIndex>& captureIndices) const {
    if (!_contentNameIsCapturing || _contentName == nullptr) {
        return _contentName ? new std::string(*_contentName) : nullptr;
    }
    return new std::string(RegexSource::replaceCaptures(*_contentName, lineText, captureIndices));
}

// CaptureRule implementation

CaptureRule::CaptureRule(ILocation* location_, RuleId id_,
                        const std::string* name, const std::string* contentName,
                        RuleId retokenizeCapturedWithRuleId_)
    : Rule(location_, id_, name, contentName),
      retokenizeCapturedWithRuleId(retokenizeCapturedWithRuleId_) {
}

void CaptureRule::dispose() {
    // Nothing specific to dispose
}

void CaptureRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    throw std::runtime_error("Not supported!");
}

CompiledRule* CaptureRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                  const std::string* endRegexSource) {
    throw std::runtime_error("Not supported!");
}

CompiledRule* CaptureRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                    const std::string* endRegexSource,
                                    bool allowA, bool allowG) {
    throw std::runtime_error("Not supported!");
}

// MatchRule implementation

MatchRule::MatchRule(ILocation* location_, RuleId id_,
                    const std::string* name, const std::string& match,
                    const std::vector<CaptureRule*>& captures_)
    : Rule(location_, id_, name, nullptr),
      _match(match, id_),
      captures(captures_),
      _cachedCompiledPatterns(nullptr) {
}

MatchRule::~MatchRule() {
    for (auto* capture : captures) {
        delete capture;
    }
}

void MatchRule::dispose() {
    if (_cachedCompiledPatterns) {
        _cachedCompiledPatterns->dispose();
        delete _cachedCompiledPatterns;
        _cachedCompiledPatterns = nullptr;
    }
}

std::string MatchRule::getDebugMatchRegExp() const {
    return _match.source;
}

void MatchRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    out->push(&_match);
}

CompiledRule* MatchRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                const std::string* endRegexSource) {
    return _getCachedCompiledPatterns(grammar)->compile(onigLib);
}

CompiledRule* MatchRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                  const std::string* endRegexSource,
                                  bool allowA, bool allowG) {
    return _getCachedCompiledPatterns(grammar)->compileAG(onigLib, allowA, allowG);
}

RegExpSourceList* MatchRule::_getCachedCompiledPatterns(IRuleRegistry* grammar) {
    if (!_cachedCompiledPatterns) {
        _cachedCompiledPatterns = new RegExpSourceList();
        collectPatterns(grammar, _cachedCompiledPatterns);
    }
    return _cachedCompiledPatterns;
}

// IncludeOnlyRule implementation

IncludeOnlyRule::IncludeOnlyRule(ILocation* location_, RuleId id_,
                                const std::string* name, const std::string* contentName,
                                const ICompilePatternsResult& patterns_)
    : Rule(location_, id_, name, contentName),
      patterns(patterns_.patterns),
      hasMissingPatterns(patterns_.hasMissingPatterns),
      _cachedCompiledPatterns(nullptr) {
}

void IncludeOnlyRule::dispose() {
    if (_cachedCompiledPatterns) {
        _cachedCompiledPatterns->dispose();
        delete _cachedCompiledPatterns;
        _cachedCompiledPatterns = nullptr;
    }
}

void IncludeOnlyRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    for (const auto& pattern : patterns) {
        Rule* rule = grammar->getRule(pattern);
        rule->collectPatterns(grammar, out);
    }
}

CompiledRule* IncludeOnlyRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                      const std::string* endRegexSource) {
    return _getCachedCompiledPatterns(grammar)->compile(onigLib);
}

CompiledRule* IncludeOnlyRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                        const std::string* endRegexSource,
                                        bool allowA, bool allowG) {
    return _getCachedCompiledPatterns(grammar)->compileAG(onigLib, allowA, allowG);
}

RegExpSourceList* IncludeOnlyRule::_getCachedCompiledPatterns(IRuleRegistry* grammar) {
    if (!_cachedCompiledPatterns) {
        _cachedCompiledPatterns = new RegExpSourceList();
        collectPatterns(grammar, _cachedCompiledPatterns);
    }
    return _cachedCompiledPatterns;
}

// BeginEndRule implementation

BeginEndRule::BeginEndRule(ILocation* location_, RuleId id_,
                          const std::string* name, const std::string* contentName,
                          const std::string& begin, const std::vector<CaptureRule*>& beginCaptures_,
                          const std::string& end, const std::vector<CaptureRule*>& endCaptures_,
                          bool applyEndPatternLast_, const ICompilePatternsResult& patterns_)
    : Rule(location_, id_, name, contentName),
      _begin(begin, id_),
      _end(end.empty() ? "\uFFFF" : end, END_RULE_ID),
      beginCaptures(beginCaptures_),
      endHasBackReferences(_end.hasBackReferences),
      endCaptures(endCaptures_),
      applyEndPatternLast(applyEndPatternLast_),
      patterns(patterns_.patterns),
      hasMissingPatterns(patterns_.hasMissingPatterns),
      _cachedCompiledPatterns(nullptr) {
}

BeginEndRule::~BeginEndRule() {
    for (auto* capture : beginCaptures) {
        delete capture;
    }
    for (auto* capture : endCaptures) {
        delete capture;
    }
}

void BeginEndRule::dispose() {
    if (_cachedCompiledPatterns) {
        _cachedCompiledPatterns->dispose();
        delete _cachedCompiledPatterns;
        _cachedCompiledPatterns = nullptr;
    }
}

std::string BeginEndRule::getDebugBeginRegExp() const {
    return _begin.source;
}

std::string BeginEndRule::getDebugEndRegExp() const {
    return _end.source;
}

std::string BeginEndRule::getEndWithResolvedBackReferences(const std::string& lineText,
                                                           const std::vector<IOnigCaptureIndex>& captureIndices) {
    return _end.resolveBackReferences(lineText, captureIndices);
}

void BeginEndRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    out->push(&_begin);
}

CompiledRule* BeginEndRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                   const std::string* endRegexSource) {
    std::string endSource = endRegexSource ? *endRegexSource : _end.source;
    return _getCachedCompiledPatterns(grammar, endSource)->compile(onigLib);
}

CompiledRule* BeginEndRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                     const std::string* endRegexSource,
                                     bool allowA, bool allowG) {
    std::string endSource = endRegexSource ? *endRegexSource : _end.source;
    return _getCachedCompiledPatterns(grammar, endSource)->compileAG(onigLib, allowA, allowG);
}

RegExpSourceList* BeginEndRule::_getCachedCompiledPatterns(IRuleRegistry* grammar,
                                                           const std::string& endRegexSource) {
    if (!_cachedCompiledPatterns) {
        _cachedCompiledPatterns = new RegExpSourceList();

        for (const auto& pattern : patterns) {
            Rule* rule = grammar->getRule(pattern);
            rule->collectPatterns(grammar, _cachedCompiledPatterns);
        }

        RegexSource* endPattern = new RegexSource(_end.source, _end.ruleId);
        if (applyEndPatternLast) {
            _cachedCompiledPatterns->push(endPattern);
        } else {
            _cachedCompiledPatterns->unshift(endPattern);
        }
    }

    if (_end.hasBackReferences) {
        int index = applyEndPatternLast ? (_cachedCompiledPatterns->length() - 1) : 0;
        _cachedCompiledPatterns->setSource(index, endRegexSource);
    }

    return _cachedCompiledPatterns;
}

// BeginWhileRule implementation

BeginWhileRule::BeginWhileRule(ILocation* location_, RuleId id_,
                              const std::string* name, const std::string* contentName,
                              const std::string& begin, const std::vector<CaptureRule*>& beginCaptures_,
                              const std::string& whilePattern, const std::vector<CaptureRule*>& whileCaptures_,
                              const ICompilePatternsResult& patterns_)
    : Rule(location_, id_, name, contentName),
      _begin(begin, id_),
      _while(whilePattern, WHILE_RULE_ID),
      beginCaptures(beginCaptures_),
      whileCaptures(whileCaptures_),
      whileHasBackReferences(_while.hasBackReferences),
      patterns(patterns_.patterns),
      hasMissingPatterns(patterns_.hasMissingPatterns),
      _cachedCompiledPatterns(nullptr),
      _cachedCompiledWhilePatterns(nullptr) {
}

BeginWhileRule::~BeginWhileRule() {
    for (auto* capture : beginCaptures) {
        delete capture;
    }
    for (auto* capture : whileCaptures) {
        delete capture;
    }
}

void BeginWhileRule::dispose() {
    if (_cachedCompiledPatterns) {
        _cachedCompiledPatterns->dispose();
        delete _cachedCompiledPatterns;
        _cachedCompiledPatterns = nullptr;
    }
    if (_cachedCompiledWhilePatterns) {
        _cachedCompiledWhilePatterns->dispose();
        delete _cachedCompiledWhilePatterns;
        _cachedCompiledWhilePatterns = nullptr;
    }
}

std::string BeginWhileRule::getDebugBeginRegExp() const {
    return _begin.source;
}

std::string BeginWhileRule::getDebugWhileRegExp() const {
    return _while.source;
}

std::string BeginWhileRule::getWhileWithResolvedBackReferences(const std::string& lineText,
                                                               const std::vector<IOnigCaptureIndex>& captureIndices) {
    return _while.resolveBackReferences(lineText, captureIndices);
}

void BeginWhileRule::collectPatterns(IRuleRegistry* grammar, RegExpSourceList* out) {
    out->push(&_begin);
}

CompiledRule* BeginWhileRule::compile(IRuleRegistry* grammar, IOnigLib* onigLib,
                                     const std::string* endRegexSource) {
    return _getCachedCompiledPatterns(grammar)->compile(onigLib);
}

CompiledRule* BeginWhileRule::compileAG(IRuleRegistry* grammar, IOnigLib* onigLib,
                                       const std::string* endRegexSource,
                                       bool allowA, bool allowG) {
    return _getCachedCompiledPatterns(grammar)->compileAG(onigLib, allowA, allowG);
}

CompiledRule* BeginWhileRule::compileWhile(IOnigLib* onigLib, const std::string* endRegexSource) {
    std::string whileSource = endRegexSource ? *endRegexSource : _while.source;
    return _getCachedCompiledWhilePatterns(onigLib, whileSource)->compile(onigLib);
}

CompiledRule* BeginWhileRule::compileWhileAG(IOnigLib* onigLib, const std::string* endRegexSource,
                                            bool allowA, bool allowG) {
    std::string whileSource = endRegexSource ? *endRegexSource : _while.source;
    return _getCachedCompiledWhilePatterns(onigLib, whileSource)->compileAG(onigLib, allowA, allowG);
}

RegExpSourceList* BeginWhileRule::_getCachedCompiledPatterns(IRuleRegistry* grammar) {
    if (!_cachedCompiledPatterns) {
        _cachedCompiledPatterns = new RegExpSourceList();

        for (const auto& pattern : patterns) {
            Rule* rule = grammar->getRule(pattern);
            rule->collectPatterns(grammar, _cachedCompiledPatterns);
        }
    }
    return _cachedCompiledPatterns;
}

RegExpSourceList* BeginWhileRule::_getCachedCompiledWhilePatterns(IOnigLib* onigLib,
                                                                  const std::string& whileRegexSource) {
    if (!_cachedCompiledWhilePatterns) {
        _cachedCompiledWhilePatterns = new RegExpSourceList();
        RegexSource* whilePattern = new RegexSource(_while.source, _while.ruleId);
        _cachedCompiledWhilePatterns->push(whilePattern);
    }

    if (_while.hasBackReferences) {
        _cachedCompiledWhilePatterns->setSource(0, whileRegexSource);
    }

    return _cachedCompiledWhilePatterns;
}

// RuleFactory implementation

Rule* RuleFactory::createCaptureRule(IRuleFactoryHelper* helper, ILocation* location,
                                     const std::string* name, const std::string* contentName,
                                     RuleId retokenizeCapturedWithRuleId) {
    CaptureRule* rule = new CaptureRule(location, ruleIdFromNumber(-1), name, contentName, retokenizeCapturedWithRuleId);
    helper->registerRule(rule);
    return rule;
}

RuleId RuleFactory::getCompiledRuleId(IRawRule* desc, IRuleFactoryHelper* helper, IRawRepository* repository) {
    if (!desc) {
        return ruleIdFromNumber(-1);
    }

    if (desc->id != nullptr) {
        return *desc->id;
    }

    RuleId ruleId = helper->registerRule(nullptr); // Will be set later
    desc->id = new RuleId(ruleId);

    Rule* rule = nullptr;

    if (desc->match) {
        rule = new MatchRule(
            desc->vscodeTextmateLocation,
            ruleId,
            desc->name,
            *desc->match,
            _compileCaptures(desc->captures, helper, repository)
        );
    } else if (desc->begin == nullptr) {
        if (desc->repository) {
            // Merge repositories
            // Simplified: just use the desc repository
        }
        std::vector<IRawRule*>* patterns = desc->patterns;
        if (!patterns && desc->include) {
            patterns = new std::vector<IRawRule*>();
            IRawRule* includeRule = new IRawRule();
            includeRule->include = new std::string(*desc->include);
            patterns->push_back(includeRule);
        }

        rule = new IncludeOnlyRule(
            desc->vscodeTextmateLocation,
            ruleId,
            desc->name,
            desc->contentName,
            _compilePatterns(patterns, helper, repository)
        );
    } else if (desc->whilePattern) {
        rule = new BeginWhileRule(
            desc->vscodeTextmateLocation,
            ruleId,
            desc->name,
            desc->contentName,
            *desc->begin,
            _compileCaptures(desc->beginCaptures ? desc->beginCaptures : desc->captures, helper, repository),
            *desc->whilePattern,
            _compileCaptures(desc->whileCaptures ? desc->whileCaptures : desc->captures, helper, repository),
            _compilePatterns(desc->patterns, helper, repository)
        );
    } else {
        rule = new BeginEndRule(
            desc->vscodeTextmateLocation,
            ruleId,
            desc->name,
            desc->contentName,
            *desc->begin,
            _compileCaptures(desc->beginCaptures ? desc->beginCaptures : desc->captures, helper, repository),
            desc->end ? *desc->end : "",
            _compileCaptures(desc->endCaptures ? desc->endCaptures : desc->captures, helper, repository),
            desc->applyEndPatternLast ? *desc->applyEndPatternLast : false,
            _compilePatterns(desc->patterns, helper, repository)
        );
    }

    // Register the rule (implementation detail - would need to store it)
    return ruleId;
}

std::vector<CaptureRule*> RuleFactory::_compileCaptures(IRawCaptures* captures,
                                                        IRuleFactoryHelper* helper,
                                                        IRawRepository* repository) {
    std::vector<CaptureRule*> result;

    if (!captures) {
        return result;
    }

    // Find maximum capture id
    int maximumCaptureId = 0;
    for (const auto& pair : captures->captures) {
        int numericCaptureId = std::stoi(pair.first);
        if (numericCaptureId > maximumCaptureId) {
            maximumCaptureId = numericCaptureId;
        }
    }

    // Initialize result
    result.resize(maximumCaptureId + 1, nullptr);

    // Fill out result
    for (const auto& pair : captures->captures) {
        int numericCaptureId = std::stoi(pair.first);
        RuleId retokenizeCapturedWithRuleId = ruleIdFromNumber(0);

        if (pair.second->patterns) {
            retokenizeCapturedWithRuleId = getCompiledRuleId(pair.second, helper, repository);
        }

        result[numericCaptureId] = new CaptureRule(
            pair.second->vscodeTextmateLocation,
            ruleIdFromNumber(-1),
            pair.second->name,
            pair.second->contentName,
            retokenizeCapturedWithRuleId
        );
    }

    return result;
}

ICompilePatternsResult RuleFactory::_compilePatterns(std::vector<IRawRule*>* patterns,
                                                     IRuleFactoryHelper* helper,
                                                     IRawRepository* repository) {
    ICompilePatternsResult result;

    if (!patterns) {
        return result;
    }

    for (auto* pattern : *patterns) {
        RuleId ruleId = ruleIdFromNumber(-1);

        if (pattern->include) {
            IncludeReference reference = parseInclude(*pattern->include);

            // Handle different include types
            // Simplified implementation
            ruleId = getCompiledRuleId(pattern, helper, repository);
        } else {
            ruleId = getCompiledRuleId(pattern, helper, repository);
        }

        if (ruleIdToNumber(ruleId) != -1) {
            result.patterns.push_back(ruleId);
        }
    }

    result.hasMissingPatterns = (patterns->size() != result.patterns.size());
    return result;
}

} // namespace vscode_textmate
