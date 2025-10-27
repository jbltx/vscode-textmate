#include "../src/grammar.h"
#include "../src/rule.h"
#include "../src/onigLib.h"
#include "../src/theme.h"
#include "../src/rawGrammar.h"
#include <iostream>
#include <cassert>

using namespace vscode_textmate;

/**
 * Phase 1 Test: Rule Registration System
 *
 * This test verifies that:
 * 1. allocateRuleId() properly allocates unique IDs
 * 2. setRule() properly stores rules at allocated IDs
 * 3. getRule() retrieves the correct rule
 * 4. The old registerRule() still works
 * 5. IDs are sequential and don't conflict
 */

void test_allocate_rule_id() {
    std::cout << "\n=== Test: allocateRuleId() ===" << std::endl;

    // Create a minimal grammar for testing
    IRawGrammar* rawGrammar = new IRawGrammar();
    rawGrammar->scopeName = "test.scope";
    rawGrammar->patterns = new std::vector<IRawRule*>();

    IOnigLib* onigLib = new DefaultOnigLib();

    // Create a minimal theme provider
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

    Grammar* grammar = new Grammar(
        "test.scope",
        rawGrammar,
        0, // initialLanguage
        nullptr, // embeddedLanguages
        nullptr, // tokenTypes
        nullptr, // balancedBracketSelectors
        nullptr, // grammarRepository
        themeProvider,
        onigLib
    );

    // Test 1: Allocate first ID (should be 1, since 0 is reserved for null)
    RuleId id1 = grammar->allocateRuleId();
    int idNum1 = ruleIdToNumber(id1);
    std::cout << "  First allocated ID: " << idNum1 << std::endl;
    assert(idNum1 == 1 && "First ID should be 1");

    // Test 2: Allocate second ID (should be 2)
    RuleId id2 = grammar->allocateRuleId();
    int idNum2 = ruleIdToNumber(id2);
    std::cout << "  Second allocated ID: " << idNum2 << std::endl;
    assert(idNum2 == 2 && "Second ID should be 2");

    // Test 3: Allocate third ID (should be 3)
    RuleId id3 = grammar->allocateRuleId();
    int idNum3 = ruleIdToNumber(id3);
    std::cout << "  Third allocated ID: " << idNum3 << std::endl;
    assert(idNum3 == 3 && "Third ID should be 3");

    // Test 4: IDs should be unique
    assert(idNum1 != idNum2 && "IDs must be unique");
    assert(idNum2 != idNum3 && "IDs must be unique");
    assert(idNum1 != idNum3 && "IDs must be unique");

    std::cout << "  ✅ All allocateRuleId() tests passed!" << std::endl;

    std::cout << "  Cleaning up..." << std::endl;
    // Note: Grammar destructor will handle rule cleanup
    delete grammar;
    std::cout << "  Grammar deleted" << std::endl;
    // Note: Skip rawGrammar cleanup - it's complex and not part of Phase 1 test
    std::cout << "  Cleanup complete" << std::endl;
}

void test_set_and_get_rule() {
    std::cout << "\n=== Test: setRule() and getRule() ===" << std::endl;

    // Create a minimal grammar for testing
    IRawGrammar* rawGrammar = new IRawGrammar();
    rawGrammar->scopeName = "test.scope";
    rawGrammar->patterns = new std::vector<IRawRule*>();

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

    Grammar* grammar = new Grammar(
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

    // Test 1: Allocate ID and create a MatchRule
    RuleId id1 = grammar->allocateRuleId();
    std::string matchPattern = "test.*";
    std::string ruleName = "test.name";

    MatchRule* rule1 = new MatchRule(
        nullptr, // location
        id1,
        &ruleName,
        matchPattern,
        std::vector<CaptureRule*>() // empty captures
    );

    std::cout << "  Created MatchRule with ID: " << ruleIdToNumber(id1) << std::endl;

    // Test 2: Store the rule
    grammar->setRule(id1, rule1);
    std::cout << "  Stored rule at ID: " << ruleIdToNumber(id1) << std::endl;

    // Test 3: Retrieve the rule
    Rule* retrievedRule = grammar->getRule(id1);
    std::cout << "  Retrieved rule at ID: " << ruleIdToNumber(id1) << std::endl;

    assert(retrievedRule != nullptr && "Retrieved rule should not be null");
    assert(retrievedRule == rule1 && "Retrieved rule should be the same as stored");

    // Test 4: Verify rule type
    MatchRule* matchRule = dynamic_cast<MatchRule*>(retrievedRule);
    assert(matchRule != nullptr && "Rule should be a MatchRule");
    std::cout << "  Rule type verified as MatchRule" << std::endl;

    // Test 5: Verify rule ID matches
    assert(ruleIdToNumber(matchRule->id) == ruleIdToNumber(id1) && "Rule ID should match");
    std::cout << "  Rule ID matches: " << ruleIdToNumber(matchRule->id) << std::endl;

    // Test 6: Test multiple rules
    RuleId id2 = grammar->allocateRuleId();
    MatchRule* rule2 = new MatchRule(
        nullptr,
        id2,
        &ruleName,
        "another.*",
        std::vector<CaptureRule*>()
    );
    grammar->setRule(id2, rule2);

    Rule* retrieved2 = grammar->getRule(id2);
    assert(retrieved2 == rule2 && "Second rule should be retrievable");
    assert(retrieved2 != rule1 && "Rules should be different");
    std::cout << "  Multiple rules work correctly" << std::endl;

    // Test 7: Verify first rule still accessible
    Rule* retrieved1Again = grammar->getRule(id1);
    assert(retrieved1Again == rule1 && "First rule should still be accessible");
    std::cout << "  First rule still accessible after second rule added" << std::endl;

    std::cout << "  ✅ All setRule() and getRule() tests passed!" << std::endl;

    delete grammar;
    // Skip rawGrammar cleanup
}

void test_register_rule_still_works() {
    std::cout << "\n=== Test: registerRule() (legacy) still works ===" << std::endl;

    IRawGrammar* rawGrammar = new IRawGrammar();
    rawGrammar->scopeName = "test.scope";
    rawGrammar->patterns = new std::vector<IRawRule*>();

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

    Grammar* grammar = new Grammar(
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

    // Test: Use old registerRule method
    std::string ruleName = "legacy.test";
    MatchRule* rule = new MatchRule(
        nullptr,
        ruleIdFromNumber(-1), // Doesn't matter, registerRule will assign
        &ruleName,
        "legacy.*",
        std::vector<CaptureRule*>()
    );

    RuleId assignedId = grammar->registerRule(rule);
    int idNum = ruleIdToNumber(assignedId);
    std::cout << "  registerRule() assigned ID: " << idNum << std::endl;

    assert(idNum > 0 && "Assigned ID should be positive");

    // Verify we can retrieve it
    Rule* retrieved = grammar->getRule(assignedId);
    assert(retrieved == rule && "Should retrieve the registered rule");
    std::cout << "  Legacy registerRule() still works correctly" << std::endl;

    std::cout << "  ✅ Legacy registerRule() test passed!" << std::endl;

    delete grammar;
    // Skip rawGrammar cleanup
}

void test_mixed_allocation_methods() {
    std::cout << "\n=== Test: Mixed allocation methods ===" << std::endl;

    IRawGrammar* rawGrammar = new IRawGrammar();
    rawGrammar->scopeName = "test.scope";
    rawGrammar->patterns = new std::vector<IRawRule*>();

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

    Grammar* grammar = new Grammar(
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

    std::string ruleName = "mixed.test";

    // Method 1: allocateRuleId + setRule
    RuleId id1 = grammar->allocateRuleId();
    MatchRule* rule1 = new MatchRule(nullptr, id1, &ruleName, "pattern1.*", std::vector<CaptureRule*>());
    grammar->setRule(id1, rule1);
    std::cout << "  Method 1 (allocate+set) - ID: " << ruleIdToNumber(id1) << std::endl;

    // Method 2: registerRule (legacy)
    MatchRule* rule2 = new MatchRule(nullptr, ruleIdFromNumber(-1), &ruleName, "pattern2.*", std::vector<CaptureRule*>());
    RuleId id2 = grammar->registerRule(rule2);
    std::cout << "  Method 2 (registerRule) - ID: " << ruleIdToNumber(id2) << std::endl;

    // Method 3: allocateRuleId + setRule again
    RuleId id3 = grammar->allocateRuleId();
    MatchRule* rule3 = new MatchRule(nullptr, id3, &ruleName, "pattern3.*", std::vector<CaptureRule*>());
    grammar->setRule(id3, rule3);
    std::cout << "  Method 3 (allocate+set) - ID: " << ruleIdToNumber(id3) << std::endl;

    // Verify all IDs are unique
    assert(ruleIdToNumber(id1) != ruleIdToNumber(id2) && "IDs must be unique");
    assert(ruleIdToNumber(id2) != ruleIdToNumber(id3) && "IDs must be unique");
    assert(ruleIdToNumber(id1) != ruleIdToNumber(id3) && "IDs must be unique");

    // Verify all rules are retrievable
    assert(grammar->getRule(id1) == rule1 && "Rule 1 should be retrievable");
    assert(grammar->getRule(id2) == rule2 && "Rule 2 should be retrievable");
    assert(grammar->getRule(id3) == rule3 && "Rule 3 should be retrievable");

    std::cout << "  All three methods work correctly together" << std::endl;
    std::cout << "  ✅ Mixed allocation test passed!" << std::endl;

    delete grammar;
    // Skip rawGrammar cleanup
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Phase 1 Tests: Rule Registration System" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        test_allocate_rule_id();
        test_set_and_get_rule();
        test_register_rule_still_works();
        test_mixed_allocation_methods();

        std::cout << "\n========================================" << std::endl;
        std::cout << "✅ ALL PHASE 1 TESTS PASSED!" << std::endl;
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
