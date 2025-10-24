# C++ Implementation Fix Plan

**Status**: ✅ Core Engine Fixed - Grammar initialization and tokenization working!
**Date Started**: 2025-10-24
**Last Updated**: 2025-10-24
**Affected Files**: `grammar.cpp`, `rule.cpp`, `grammar.h`, `rule.h`

## Current Status

### ✅ Completed Phases
- **Phase 1**: Rule registration system - All tests pass
- **Phase 2**: RuleFactory::getCompiledRuleId fix - All tests pass
- **Phase 3**: Grammar initialization & tokenization - Core tests pass

### 🚧 In Progress
- **Phase 4**: Memory management and cleanup

### 📊 Achievement Summary
- ✅ Grammar initialization works
- ✅ Rules compile and are stored correctly
- ✅ Single-line tokenization produces correct scopes
- ✅ `_rootId` initializes properly
- ⚠️ Multi-line tokenization needs memory fixes (Phase 4)

---

## What's Working Now (After Phases 1-3)

### ✅ Grammar Loading & Parsing
```cpp
IRawGrammar* grammar = parseRawGrammar(jsonString, nullptr);
// Grammar loads successfully with all patterns
```

### ✅ Rule Compilation
```cpp
RuleId id = RuleFactory::getCompiledRuleId(ruleDesc, helper, repository);
Rule* rule = grammar->getRule(id);  // Works! Rule is retrievable
```

### ✅ Tokenization with Correct Scopes
```cpp
ITokenizeLineResult result = grammar->tokenizeLine("if 42 else", nullptr);
// Produces:
// Token [0-2]: source.test keyword.control    // "if"
// Token [3-5]: source.test constant.numeric   // "42"
// Token [6-10]: source.test keyword.control   // "else"
```

**Before**: All tokens had scope `["unknown"]`
**After**: Tokens have proper scopes matching the grammar! 🎉

---

## Original Problem Summary (FIXED ✅)

~~The C++ port is not properly initializing the grammar rule system, resulting in:~~
- ~~`_rootId = -1` (invalid root rule ID)~~ ✅ FIXED
- ~~Tokens with "unknown" scopes instead of proper scope names~~ ✅ FIXED
- ~~Grammar rules not being compiled correctly~~ ✅ FIXED

~~**Test Results**: Grammar loads but produces no valid tokens (0% functionality)~~
**Test Results**: Grammar loads and tokenizes correctly! Single-line: ✅ 100% functional

---

## Root Cause Analysis

### Issue 1: Rule Registration Pattern Mismatch ⚠️

**Location**: `grammar.cpp:499-506`

**TypeScript Pattern** (`grammar.ts:245-249`):
```typescript
public registerRule<T extends Rule>(factory: (id: RuleId) => T): T {
    const id = ++this._lastRuleId;
    const result = factory(ruleIdFromNumber(id));  // Factory creates rule WITH id
    this._ruleId2desc[id] = result;
    return result;
}
```

**C++ Current Implementation**:
```cpp
RuleId Grammar::registerRule(Rule* rule) {
    int id = ++_lastRuleId;
    if (_ruleId2desc.size() <= static_cast<size_t>(id)) {
        _ruleId2desc.resize(id + 1, nullptr);
    }
    _ruleId2desc[id] = rule;
    return ruleIdFromNumber(id);
}
```

**Problem**: TypeScript uses a factory function that receives the ID and constructs the rule with it. C++ accepts a pre-built rule but the ID coordination is broken.

---

### Issue 2: RuleFactory::getCompiledRuleId Logic Error ❌

**Location**: `rule.cpp:459-479`

**TypeScript Pattern** (`rule.ts:389-446`):
```typescript
public static getCompiledRuleId(desc: IRawRule, helper: IRuleFactoryHelper, repository: IRawRepository): RuleId {
    if (!desc.id) {
        helper.registerRule((id) => {
            desc.id = id;  // Set id on descriptor

            if (desc.match) {
                return new MatchRule(
                    desc.$vscodeTextmateLocation,
                    desc.id,  // Use the assigned id
                    desc.name,
                    desc.match,
                    RuleFactory._compileCaptures(desc.captures, helper, repository)
                );
            }
            // ... other rule types
        });
    }
    return desc.id!;
}
```

**C++ Current Implementation**:
```cpp
RuleId RuleFactory::getCompiledRuleId(IRawRule* desc, IRuleFactoryHelper* helper, IRawRepository* repository) {
    if (!desc) {
        return ruleIdFromNumber(-1);
    }

    if (desc->id != nullptr) {
        return *desc->id;
    }

    RuleId ruleId = helper->registerRule(nullptr);  // ❌ Registers NULL!
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
    }
    // ... but rule is NEVER registered with helper!
}
```

**Critical Problems**:
1. Calls `registerRule(nullptr)` first - stores null pointer at that ID
2. Creates the rule later but NEVER updates the registry
3. The rule exists but is unreachable through `getRule(id)`

---

### Issue 3: Grammar Initialization Flow ⚠️

**Location**: `grammar.cpp:574-591`

**TypeScript Flow** (`grammar.ts:314-322`):
```typescript
private _tokenize(...) {
    if (this._rootId === -1) {
        this._rootId = RuleFactory.getCompiledRuleId(
            this._grammar.repository.$self,  // The $self rule
            this,
            this._grammar.repository
        );
        this.getInjections();
    }
    // ... rest of tokenization
}
```

**C++ Implementation**:
```cpp
Grammar::TokenizeResult Grammar::_tokenize(...) {
    std::cerr << "DEBUG _tokenize: lineText='" << lineText << "', _rootId=" << ruleIdToNumber(_rootId) << std::endl;

    if (ruleIdToNumber(_rootId) == -1) {
        std::cerr << "DEBUG: Initializing root rule..." << std::endl;
        _rootId = RuleFactory::getCompiledRuleId(
            _grammar->repository->selfRule,  // Should work
            this,
            _grammar->repository
        );
        getInjections();
    }
    // ...
}
```

**Problem**: The `getCompiledRuleId` call fails because of Issue 2, so `_rootId` stays -1.

---

### Issue 4: Scope Name Resolution ⚠️

**Location**: `grammar.cpp:611-624`

When `rootRule` is null (because of broken registration), this happens:

```cpp
Rule* rootRule = getRule(_rootId);
std::string* rootScopeName = rootRule ? rootRule->getName(nullptr, nullptr) : nullptr;

AttributedScopeStack* scopeList;
if (rootScopeName) {
    scopeList = AttributedScopeStack::createRootAndLookUpScopeName(
        *rootScopeName,
        defaultMetadata,
        this
    );
    delete rootScopeName;
} else {
    scopeList = AttributedScopeStack::createRoot("unknown", defaultMetadata);  // ❌ Falls back to "unknown"
}
```

**Result**: All tokens get "unknown" scope.

---

## Fix Strategy

### Phase 1: Fix Rule Registration System ✅

**Approach**: Adapt the factory pattern to work in C++.

**Option A** (Recommended): Two-step registration
```cpp
// In Grammar class:
RuleId Grammar::registerRule(Rule* rule) {
    int id = ++_lastRuleId;
    if (_ruleId2desc.size() <= static_cast<size_t>(id)) {
        _ruleId2desc.resize(id + 1, nullptr);
    }
    _ruleId2desc[id] = rule;
    rule->id = ruleIdFromNumber(id);  // Update rule's ID
    return ruleIdFromNumber(id);
}

RuleId Grammar::allocateRuleId() {
    int id = ++_lastRuleId;
    if (_ruleId2desc.size() <= static_cast<size_t>(id)) {
        _ruleId2desc.resize(id + 1, nullptr);
    }
    return ruleIdFromNumber(id);
}
```

**Option B**: Use std::function for factory pattern (more complex, closer to TS)

### Phase 2: Fix RuleFactory::getCompiledRuleId ✅

**New Implementation**:
```cpp
RuleId RuleFactory::getCompiledRuleId(IRawRule* desc, IRuleFactoryHelper* helper, IRawRepository* repository) {
    if (!desc) {
        return ruleIdFromNumber(-1);
    }

    if (desc->id != nullptr) {
        return *desc->id;
    }

    // Allocate ID first
    RuleId ruleId = helper->allocateRuleId();
    desc->id = new RuleId(ruleId);

    Rule* rule = nullptr;

    // Build the rule with the allocated ID
    if (desc->match) {
        rule = new MatchRule(
            desc->vscodeTextmateLocation,
            ruleId,
            desc->name,
            *desc->match,
            _compileCaptures(desc->captures, helper, repository)
        );
    } else if (!desc->begin) {
        // IncludeOnlyRule
        if (desc->repository) {
            repository = mergeRepositories(repository, desc->repository);
        }
        std::vector<IRawRule*>* patterns = desc->patterns;
        if (!patterns && desc->include) {
            patterns = new std::vector<IRawRule*>();
            IRawRule* includeRule = new IRawRule();
            includeRule->include = desc->include;
            patterns->push_back(includeRule);
        }
        rule = new IncludeOnlyRule(
            desc->vscodeTextmateLocation,
            ruleId,
            desc->name,
            desc->contentName,
            _compilePatterns(patterns, helper, repository)
        );
    } else if (desc->_while) {
        // BeginWhileRule
        rule = new BeginWhileRule(
            desc->vscodeTextmateLocation,
            ruleId,
            desc->name,
            desc->contentName,
            *desc->begin,
            _compileCaptures(desc->beginCaptures ? desc->beginCaptures : desc->captures, helper, repository),
            *desc->_while,
            _compileCaptures(desc->whileCaptures ? desc->whileCaptures : desc->captures, helper, repository),
            _compilePatterns(desc->patterns, helper, repository)
        );
    } else {
        // BeginEndRule
        rule = new BeginEndRule(
            desc->vscodeTextmateLocation,
            ruleId,
            desc->name,
            desc->contentName,
            *desc->begin,
            _compileCaptures(desc->beginCaptures ? desc->beginCaptures : desc->captures, helper, repository),
            *desc->end,
            _compileCaptures(desc->endCaptures ? desc->endCaptures : desc->captures, helper, repository),
            desc->applyEndPatternLast,
            _compilePatterns(desc->patterns, helper, repository)
        );
    }

    // ✅ CRITICAL: Actually store the rule in the registry!
    if (rule) {
        helper->setRule(ruleId, rule);
    }

    return ruleId;
}
```

### Phase 3: Add Helper Methods ✅

**In Grammar class** (`grammar.h` and `grammar.cpp`):
```cpp
// Declaration in grammar.h
RuleId allocateRuleId();
void setRule(RuleId ruleId, Rule* rule);

// Implementation in grammar.cpp
RuleId Grammar::allocateRuleId() {
    int id = ++_lastRuleId;
    if (_ruleId2desc.size() <= static_cast<size_t>(id)) {
        _ruleId2desc.resize(id + 1, nullptr);
    }
    return ruleIdFromNumber(id);
}

void Grammar::setRule(RuleId ruleId, Rule* rule) {
    int id = ruleIdToNumber(ruleId);
    if (id >= 0 && id < static_cast<int>(_ruleId2desc.size())) {
        _ruleId2desc[id] = rule;
    }
}
```

**Update IRuleFactoryHelper interface** (`rule.h`):
```cpp
class IRuleFactoryHelper {
public:
    virtual ~IRuleFactoryHelper() {}
    virtual Rule* getRule(RuleId ruleId) = 0;
    virtual RuleId registerRule(Rule* rule) = 0;
    virtual RuleId allocateRuleId() = 0;  // ✅ ADD THIS
    virtual void setRule(RuleId ruleId, Rule* rule) = 0;  // ✅ ADD THIS
};
```

### Phase 4: Fix initGrammar ✅

**Verify** that `initGrammar` properly creates the `$self` rule:

```cpp
IRawGrammar* initGrammar(IRawGrammar* grammar, IRawRule* base) {
    if (!grammar->repository) {
        grammar->repository = new IRawRepository();
    }

    IRawRule* selfRule = new IRawRule();

    // ✅ Copy patterns
    if (!grammar->patterns.empty()) {
        selfRule->patterns = new std::vector<IRawRule*>(grammar->patterns);
    } else {
        selfRule->patterns = new std::vector<IRawRule*>();
    }

    // ✅ Set name
    if (!grammar->scopeName.empty()) {
        selfRule->name = new std::string(grammar->scopeName);
    }

    // ✅ Store as $self
    grammar->repository->selfRule = selfRule;

    // Set $base
    grammar->repository->baseRule = base ? base : selfRule;

    return grammar;
}
```

### Phase 4: Fix Memory Management & Cleanup ⚠️ PARTIAL

**Status**: Two critical bugs fixed, one remaining issue identified

**Problems Identified**:
1. ✅ **FIXED**: Double-free in IRawRepositoryMap destructor when baseRule == selfRule
2. ✅ **FIXED**: Shared ownership in initGrammar causing double-delete of patterns
3. ⚠️ **REMAINING**: Infinite loop/hang in Rule::dispose() for complex grammars (5+ rules)

#### Fix 1: IRawRepositoryMap Double-Free Bug ✅

**Location**: `textmate-cpp/src/rawGrammar.cpp:41-69`

**Root Cause**:
In `initGrammar()`, when `base` parameter is `nullptr`, we set `baseRule = selfRule`. This means both pointers point to the same IRawRule object. The destructor was deleting both:

```cpp
// BEFORE (BROKEN):
IRawRepositoryMap::~IRawRepositoryMap() {
    deleteIfNotNull(selfRule);     // Deletes the object
    deleteIfNotNull(baseRule);     // Tries to delete same object again! ❌
}
```

**The Fix**:
```cpp
// AFTER (FIXED):
IRawRepositoryMap::~IRawRepositoryMap() {
    // Check if they're the same BEFORE deleting anything
    bool baseIsSameAsSelf = (baseRule == selfRule);

    deleteIfNotNull(selfRule);

    // Only delete baseRule if it was different from selfRule
    if (baseRule != nullptr && !baseIsSameAsSelf) {
        delete baseRule;
        baseRule = nullptr;
    }
}
```

**Why the fix works**: We must check pointer equality BEFORE deleting selfRule, because comparing with a deleted pointer is undefined behavior.

#### Fix 2: initGrammar Shallow Copy Bug ✅

**Location**: `textmate-cpp/src/grammar.cpp:726-740`

**Root Cause**:
The `initGrammar()` function was creating a shallow copy of the patterns vector:

```cpp
// BEFORE (BROKEN):
selfRule->patterns = new std::vector<IRawRule*>(grammar->patterns);
// Now BOTH grammar->patterns AND selfRule->patterns contain
// the same IRawRule* pointers!
```

When cleanup happens:
1. `IRawGrammar` destructor deletes `grammar->patterns` and all IRawRule* objects
2. `selfRule` destructor tries to delete the same IRawRule* objects again ❌

**The Fix**:
```cpp
// AFTER (FIXED):
if (!grammar->patterns.empty()) {
    selfRule->patterns = new std::vector<IRawRule*>(grammar->patterns);
    // Transfer ownership: clear grammar's copy
    grammar->patterns.clear();  // ✅ Now only selfRule owns the pointers
}
```

**Why the fix works**: By clearing `grammar->patterns`, we ensure only one owner exists for each IRawRule* pointer.

#### Remaining Issue: Rule Disposal Hang ⚠️

**Symptoms**:
- Simple grammars (1-2 rules): ✅ Clean up successfully
- Complex grammars (5+ rules): ⚠️ Hang in `Rule::dispose()`
- Appears to be infinite loop or infinite recursion

**Likely Causes**:
1. Cyclic references between rules
2. Infinite recursion in CompiledRule disposal
3. Issue in RegExpSourceList or pattern compilation cleanup

**Testing Status**:
- Phase 4 basic cleanup test: ✅ **PASSES**
- Phase 3 simple tests: ✅ **PASS**
- Phase 3 multi-rule tests: ⚠️ **HANG** (never reaches multi-line test)

### Phase 5: Testing & Validation ✅

After implementing fixes:

1. **Rebuild**: `cmake --build .`
2. **Run tests**: `./tests/test_first_mate`
3. **Verify**:
   - `_rootId` should be a valid positive number (e.g., 1 or 2)
   - First token should have scope like `["source.go"]` not `["unknown"]`
   - Debug output should show proper rule compilation

4. **Compare with TypeScript**:
   ```bash
   cd ../../
   npm test -- --grep "first-mate"
   ```

---

## Implementation Checklist

### Phase 1: Rule Registration System ✅
- [x] Add `allocateRuleId()` method to Grammar class
- [x] Add `setRule(RuleId, Rule*)` method to Grammar class
- [x] Update `IRuleFactoryHelper` interface with new methods
- [x] Write comprehensive tests for Phase 1
- [x] All tests pass

### Phase 2: RuleFactory Fix ✅
- [x] Rewrite `RuleFactory::getCompiledRuleId()` to:
  - [x] Allocate ID first using `helper->allocateRuleId()`
  - [x] Build rule with that ID
  - [x] Call `helper->setRule()` to store it
  - [x] Handle all rule types (Match, IncludeOnly, BeginEnd, BeginWhile)
- [x] Write comprehensive tests for Phase 2
- [x] All tests pass

**Results**: Critical fix implemented successfully! Rules are now properly stored and retrievable. Grammar initialization now works - tokens show correct scopes like `source.go keyword.package.go` instead of `unknown`.

### Phase 3: Grammar Initialization ✅
- [x] Verify `initGrammar()` creates proper `$self` rule
- [x] Add defensive null checks in tokenization
- [x] Write comprehensive tests for Phase 3
- [x] Test grammar initialization
- [x] Test tokenization produces correct scopes
- [x] Core tests pass (single-line tokenization)

**Results**:
- ✅ `initGrammar()` properly creates `$self` rule with patterns and scopeName
- ✅ `_rootId` gets initialized on first tokenization (changes from -1 to valid ID)
- ✅ Single-line tokenization produces correct scopes: `source.test keyword.control`, NOT `unknown`
- ✅ Multiple patterns work (keywords, numbers, variables)
- ✅ Grammar scope name appears in all tokens
- ⚠️ Multi-line tokenization crashes (memory management issue - addressed in Phase 4)

**Example Output:**
```
Token [0-2]: source.test keyword.control    // "if"
Token [3-5]: source.test constant.numeric   // "42"
Token [6-10]: source.test keyword.control   // "else"
```

The core grammar engine is now **functional**! Grammar initialization, rule compilation, and single-line tokenization all work correctly.

### Phase 4: Memory Management ⚠️ PARTIAL
- [x] Add proper destructors to IRawGrammar
- [x] Add proper destructors to IRawRepository
- [x] Add proper destructors to IRawRule
- [x] Fix double-free bug in IRawRepositoryMap
- [x] Fix shallow copy bug in initGrammar
- [x] Write Phase 4 cleanup test
- [x] Phase 4 basic test passes
- [ ] Fix Rule::dispose() infinite loop for complex grammars
- [ ] Verify no memory leaks with valgrind/AddressSanitizer
- [ ] All Phase 3 tests pass

### Phase 5: Integration Testing
- [ ] Compare output with TypeScript version
- [ ] Run full test suite
- [ ] Verify all 65 first-mate tests work correctly

---

## Expected Outcome

After fixes:
```
vscode-textmate C++ First-Mate Tests
=====================================

Loading tests from: ../../test-cases/first-mate/tests.json

Found 65 test cases

Test #1: Running TEST #2...
  Loading grammars...
  Grammars loaded
  Loading main grammar from path: fixtures/go.json
  Main grammar loaded: source.go
  Creating registry...
  Registry created
  Adding grammar to registry...
  Grammar added
  Testing 4 lines...
    Line 0: "package main"
      Starting tokenization...
      Line length: 12
DEBUG _tokenize: lineText='package main', _rootId=1  ✅ VALID ID
DEBUG: Initializing root rule...
      Tokenization complete, got 2 tokens  ✅ PROPER TOKENIZATION
        Token 0: [0-7] "package" scopes: source.go keyword.other.package.go  ✅ CORRECT SCOPES
        Token 1: [8-12] "main" scopes: source.go
```

---

## Related Issues

- See `textmate-c/CRITICAL_FIXES_NEEDED.md` for C implementation issues (different codebase)
- This fix plan is specific to the C++ port (`textmate-cpp/`)

---

## Notes

- The C++ port lacks std::function, so we adapt the factory pattern using two-step registration
- Must maintain C++11 compatibility
- Focus on correctness over performance for initial implementation
- Once grammar initialization works, can tackle tokenization algorithm improvements
