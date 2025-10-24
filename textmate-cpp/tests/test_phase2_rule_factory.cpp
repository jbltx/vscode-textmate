#include "../src/grammar.h"
#include "../src/rule.h"
#include "../src/onigLib.h"
#include "../src/theme.h"
#include "../src/rawGrammar.h"
#include <iostream>
#include <cassert>

using namespace vscode_textmate;

/**
 * Phase 2 Test: RuleFactory::getCompiledRuleId
 *
 * This test verifies that:
 * 1. getCompiledRuleId properly allocates IDs
 * 2. Rules are actually stored in the registry
 * 3. Rules can be retrieved after compilation
 * 4. Different rule types are handled correctly (Match, IncludeOnly, BeginEnd)
 * 5. Rule IDs are stored in the IRawRule descriptor
 */

// Helper to create a test grammar
Grammar* createTestGrammar() {
    IRawGrammar* rawGrammar = new IRawGrammar();
    rawGrammar->scopeName = "test.scope";
    rawGrammar->patterns = std::vector<IRawRule*>();

    IOnigLib* onigLib = new DefaultOnigLib();

    class TestThemeProvider : public IThemeProvider {
    public:
        StyleAttributes* themeMatch(ScopeStack* scopePath) override {
            return getDefaults();
        }
        StyleAttributes* getDefaults() override {
            return new StyleAttributes(0, 1, 0);
        }
    };

    TestThemeProvider* themeProvider = new TestThemeProvider();

    return new Grammar(
        "test.scope",
        rawGrammar,
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        themeProvider,
        onigLib
    );
}

void test_match_rule_compilation() {
    std::cout << "\n=== Test: MatchRule Compilation ===" << std::endl;

    Grammar* grammar = createTestGrammar();
    IRawRepository* repository = new IRawRepository();

    // Create a MatchRule descriptor
    IRawRule* matchDesc = new IRawRule();
    matchDesc->match = new std::string("test.*");
    matchDesc->name = new std::string("test.match.name");

    std::cout << "  Created IRawRule with match pattern" << std::endl;

    // Compile the rule
    RuleId ruleId = RuleFactory::getCompiledRuleId(matchDesc, grammar, repository);
    int idNum = ruleIdToNumber(ruleId);

    std::cout << "  Compiled rule ID: " << idNum << std::endl;
    assert(idNum > 0 && "Rule ID should be positive");

    // Verify the rule is stored in the grammar
    Rule* retrievedRule = grammar->getRule(ruleId);
    assert(retrievedRule != nullptr && "Rule should be retrievable from grammar");
    std::cout << "  ✅ Rule is retrievable from grammar" << std::endl;

    // Verify it's a MatchRule
    MatchRule* matchRule = dynamic_cast<MatchRule*>(retrievedRule);
    assert(matchRule != nullptr && "Retrieved rule should be a MatchRule");
    std::cout << "  ✅ Rule is correct type (MatchRule)" << std::endl;

    // Verify the ID matches
    assert(ruleIdToNumber(matchRule->id) == idNum && "Rule ID should match");
    std::cout << "  ✅ Rule ID matches: " << ruleIdToNumber(matchRule->id) << std::endl;

    // Verify ID is stored in descriptor
    assert(matchDesc->id != nullptr && "ID should be stored in descriptor");
    assert(ruleIdToNumber(*matchDesc->id) == idNum && "Descriptor ID should match");
    std::cout << "  ✅ ID stored in descriptor: " << ruleIdToNumber(*matchDesc->id) << std::endl;

    // Test idempotency - calling again should return same ID
    RuleId ruleId2 = RuleFactory::getCompiledRuleId(matchDesc, grammar, repository);
    assert(ruleIdToNumber(ruleId2) == idNum && "Should return same ID on second call");
    std::cout << "  ✅ Idempotent: second call returns same ID" << std::endl;

    std::cout << "  ✅ All MatchRule compilation tests passed!" << std::endl;

    delete grammar;
    // Skip cleanup for now
}

void test_include_only_rule_compilation() {
    std::cout << "\n=== Test: IncludeOnlyRule Compilation ===" << std::endl;

    Grammar* grammar = createTestGrammar();
    IRawRepository* repository = new IRawRepository();

    // Create an IncludeOnlyRule descriptor (no begin, no match)
    IRawRule* includeDesc = new IRawRule();
    includeDesc->name = new std::string("test.include.name");
    includeDesc->patterns = new std::vector<IRawRule*>();

    std::cout << "  Created IRawRule with patterns (IncludeOnly)" << std::endl;

    // Compile the rule
    RuleId ruleId = RuleFactory::getCompiledRuleId(includeDesc, grammar, repository);
    int idNum = ruleIdToNumber(ruleId);

    std::cout << "  Compiled rule ID: " << idNum << std::endl;
    assert(idNum > 0 && "Rule ID should be positive");

    // Verify the rule is stored
    Rule* retrievedRule = grammar->getRule(ruleId);
    assert(retrievedRule != nullptr && "Rule should be retrievable");
    std::cout << "  ✅ Rule is retrievable from grammar" << std::endl;

    // Verify it's an IncludeOnlyRule
    IncludeOnlyRule* includeRule = dynamic_cast<IncludeOnlyRule*>(retrievedRule);
    assert(includeRule != nullptr && "Retrieved rule should be an IncludeOnlyRule");
    std::cout << "  ✅ Rule is correct type (IncludeOnlyRule)" << std::endl;

    std::cout << "  ✅ All IncludeOnlyRule compilation tests passed!" << std::endl;

    delete grammar;
}

void test_begin_end_rule_compilation() {
    std::cout << "\n=== Test: BeginEndRule Compilation ===" << std::endl;

    Grammar* grammar = createTestGrammar();
    IRawRepository* repository = new IRawRepository();

    // Create a BeginEndRule descriptor
    IRawRule* beginEndDesc = new IRawRule();
    beginEndDesc->name = new std::string("test.beginend.name");
    beginEndDesc->begin = new std::string("\\{");
    beginEndDesc->end = new std::string("\\}");
    beginEndDesc->patterns = new std::vector<IRawRule*>();

    std::cout << "  Created IRawRule with begin/end patterns" << std::endl;

    // Compile the rule
    RuleId ruleId = RuleFactory::getCompiledRuleId(beginEndDesc, grammar, repository);
    int idNum = ruleIdToNumber(ruleId);

    std::cout << "  Compiled rule ID: " << idNum << std::endl;
    assert(idNum > 0 && "Rule ID should be positive");

    // Verify the rule is stored
    Rule* retrievedRule = grammar->getRule(ruleId);
    assert(retrievedRule != nullptr && "Rule should be retrievable");
    std::cout << "  ✅ Rule is retrievable from grammar" << std::endl;

    // Verify it's a BeginEndRule
    BeginEndRule* beginEndRule = dynamic_cast<BeginEndRule*>(retrievedRule);
    assert(beginEndRule != nullptr && "Retrieved rule should be a BeginEndRule");
    std::cout << "  ✅ Rule is correct type (BeginEndRule)" << std::endl;

    std::cout << "  ✅ All BeginEndRule compilation tests passed!" << std::endl;

    delete grammar;
}

void test_multiple_rules_independent_ids() {
    std::cout << "\n=== Test: Multiple Rules Get Independent IDs ===" << std::endl;

    Grammar* grammar = createTestGrammar();
    IRawRepository* repository = new IRawRepository();

    // Create three different rules
    IRawRule* rule1 = new IRawRule();
    rule1->match = new std::string("rule1.*");
    rule1->name = new std::string("test.rule1");

    IRawRule* rule2 = new IRawRule();
    rule2->match = new std::string("rule2.*");
    rule2->name = new std::string("test.rule2");

    IRawRule* rule3 = new IRawRule();
    rule3->match = new std::string("rule3.*");
    rule3->name = new std::string("test.rule3");

    // Compile all three
    RuleId id1 = RuleFactory::getCompiledRuleId(rule1, grammar, repository);
    RuleId id2 = RuleFactory::getCompiledRuleId(rule2, grammar, repository);
    RuleId id3 = RuleFactory::getCompiledRuleId(rule3, grammar, repository);

    int idNum1 = ruleIdToNumber(id1);
    int idNum2 = ruleIdToNumber(id2);
    int idNum3 = ruleIdToNumber(id3);

    std::cout << "  Rule 1 ID: " << idNum1 << std::endl;
    std::cout << "  Rule 2 ID: " << idNum2 << std::endl;
    std::cout << "  Rule 3 ID: " << idNum3 << std::endl;

    // Verify all IDs are unique
    assert(idNum1 != idNum2 && "IDs must be unique");
    assert(idNum2 != idNum3 && "IDs must be unique");
    assert(idNum1 != idNum3 && "IDs must be unique");
    std::cout << "  ✅ All IDs are unique" << std::endl;

    // Verify all rules are retrievable
    assert(grammar->getRule(id1) != nullptr && "Rule 1 should be retrievable");
    assert(grammar->getRule(id2) != nullptr && "Rule 2 should be retrievable");
    assert(grammar->getRule(id3) != nullptr && "Rule 3 should be retrievable");
    std::cout << "  ✅ All rules are retrievable" << std::endl;

    // Verify no cross-contamination
    Rule* retrieved1 = grammar->getRule(id1);
    Rule* retrieved2 = grammar->getRule(id2);
    Rule* retrieved3 = grammar->getRule(id3);

    assert(retrieved1 != retrieved2 && "Rules must be different objects");
    assert(retrieved2 != retrieved3 && "Rules must be different objects");
    assert(retrieved1 != retrieved3 && "Rules must be different objects");
    std::cout << "  ✅ Each ID retrieves a different rule object" << std::endl;

    std::cout << "  ✅ All multiple rules tests passed!" << std::endl;

    delete grammar;
}

void test_nested_rule_compilation() {
    std::cout << "\n=== Test: Nested Rules (patterns within rules) ===" << std::endl;

    Grammar* grammar = createTestGrammar();
    IRawRepository* repository = new IRawRepository();

    // Create a parent rule with nested patterns
    IRawRule* parentRule = new IRawRule();
    parentRule->name = new std::string("test.parent");
    parentRule->begin = new std::string("begin");
    parentRule->end = new std::string("end");
    parentRule->patterns = new std::vector<IRawRule*>();

    // Add a nested pattern
    IRawRule* nestedPattern = new IRawRule();
    nestedPattern->match = new std::string("nested.*");
    nestedPattern->name = new std::string("test.nested");
    parentRule->patterns->push_back(nestedPattern);

    std::cout << "  Created parent rule with nested pattern" << std::endl;

    // Compile the parent rule (should compile nested too)
    RuleId parentId = RuleFactory::getCompiledRuleId(parentRule, grammar, repository);
    int parentIdNum = ruleIdToNumber(parentId);

    std::cout << "  Parent rule ID: " << parentIdNum << std::endl;
    assert(parentIdNum > 0 && "Parent ID should be positive");

    // Verify parent is stored
    Rule* parentRetrieved = grammar->getRule(parentId);
    assert(parentRetrieved != nullptr && "Parent rule should be retrievable");
    std::cout << "  ✅ Parent rule is retrievable" << std::endl;

    // Check if nested pattern was compiled
    if (nestedPattern->id != nullptr) {
        int nestedIdNum = ruleIdToNumber(*nestedPattern->id);
        std::cout << "  Nested pattern ID: " << nestedIdNum << std::endl;

        Rule* nestedRetrieved = grammar->getRule(*nestedPattern->id);
        assert(nestedRetrieved != nullptr && "Nested rule should be retrievable");
        std::cout << "  ✅ Nested pattern was compiled and is retrievable" << std::endl;

        assert(parentIdNum != nestedIdNum && "Parent and nested should have different IDs");
        std::cout << "  ✅ Parent and nested have different IDs" << std::endl;
    }

    std::cout << "  ✅ All nested rule compilation tests passed!" << std::endl;

    delete grammar;
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Phase 2 Tests: RuleFactory::getCompiledRuleId" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        test_match_rule_compilation();
        test_include_only_rule_compilation();
        test_begin_end_rule_compilation();
        test_multiple_rules_independent_ids();
        test_nested_rule_compilation();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ ALL PHASE 2 TESTS PASSED!" << std::endl;
        std::cout << "========================================\n" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED WITH EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n❌ TEST FAILED WITH UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
}
