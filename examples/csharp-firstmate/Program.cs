using System;
using System.IO;
using System.Text.Json;
using System.Collections.Generic;
using System.Linq;
using TextMateSharp;

class Program
{
    class TestToken
    {
        public string value { get; set; } = "";
        public List<string> scopes { get; set; } = new();
    }

    class TestLine
    {
        public string line { get; set; } = "";
        public List<TestToken> tokens { get; set; } = new();
    }

    class TestCase
    {
        public string desc { get; set; } = "";
        public List<string> grammars { get; set; } = new();
        public string? grammarPath { get; set; }
        public string? grammarScopeName { get; set; }
        public List<string>? grammarInjections { get; set; }
        public List<TestLine> lines { get; set; } = new();
    }

    static int Main(string[] args)
    {
        // Check for test unicode flag
        if (args.Length > 0 && args[0] == "--test-unicode")
        {
            TestUnicode.TestMarshalingIssue();
            return 0;
        }

        Console.WriteLine("TextMate FirstMate Test Runner (C#)");
        Console.WriteLine("====================================\n");

        string testsPath = args.Length > 0 ? args[0] : "../../test-cases/first-mate/tests.json";
        string fixturesBasePath = Path.GetDirectoryName(Path.GetFullPath(testsPath))!;

        if (!File.Exists(testsPath))
        {
            Console.WriteLine($"Error: Test file not found: {testsPath}");
            return 1;
        }

        try
        {
            // Parse test cases
            var jsonText = File.ReadAllText(testsPath);
            var testCases = JsonSerializer.Deserialize<List<TestCase>>(jsonText, new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            });

            if (testCases == null || testCases.Count == 0)
            {
                Console.WriteLine("No test cases found!");
                return 1;
            }

            Console.WriteLine($"Running {testCases.Count} test case(s)...\n");

            int totalTests = 0;
            int passedTests = 0;
            int failedTests = 0;

            foreach (var testCase in testCases)
            {
                totalTests++;
                string testName = !string.IsNullOrEmpty(testCase.desc) ? testCase.desc : $"Test {totalTests}";
                Console.Write($"Test {totalTests}: {testName} ... ");

                try
                {
                    bool passed = RunTestCase(testCase, fixturesBasePath);
                    if (passed)
                    {
                        Console.WriteLine("✓ PASSED");
                        passedTests++;
                    }
                    else
                    {
                        Console.WriteLine("✗ FAILED");
                        failedTests++;
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"✗ ERROR: {ex.Message}");
                    failedTests++;
                }
            }

            Console.WriteLine($"\n{new string('=', 50)}");
            Console.WriteLine($"Total: {totalTests} | Passed: {passedTests} | Failed: {failedTests}");
            Console.WriteLine($"{new string('=', 50)}\n");

            return failedTests > 0 ? 1 : 0;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Fatal error: {ex.Message}");
            Console.WriteLine($"Stack trace: {ex.StackTrace}");
            return 1;
        }
    }

    static bool RunTestCase(TestCase testCase, string fixturesBasePath)
    {
        using var textMate = new TextMate();

        // Step 1: Add ALL grammars to the registry first (so dependencies can be resolved)
        string? mainGrammarScopeName = testCase.grammarScopeName;
        var grammarScopeByPath = new Dictionary<string, string>();

        foreach (var grammarPath in testCase.grammars)
        {
            string fullGrammarPath = Path.Combine(fixturesBasePath, grammarPath);
            if (!File.Exists(fullGrammarPath))
            {
                throw new Exception($"Grammar file not found: {fullGrammarPath}");
            }

            // Read the grammar JSON to get its scope name
            var grammarJson = File.ReadAllText(fullGrammarPath);
            var grammarDoc = JsonDocument.Parse(grammarJson);
            string scopeName = grammarDoc.RootElement.GetProperty("scopeName").GetString()!;

            // Add grammar to registry
            textMate.AddGrammarFromJson(grammarJson);

            grammarScopeByPath[grammarPath] = scopeName;

            // If this is the main grammar path, save its scope name
            if (testCase.grammarPath == grammarPath && string.IsNullOrEmpty(mainGrammarScopeName))
            {
                mainGrammarScopeName = scopeName;
            }
        }

        // Step 2: Determine the main grammar scope name
        if (string.IsNullOrEmpty(mainGrammarScopeName))
        {
            // Use the first grammar if no specific one was indicated
            if (testCase.grammars.Count > 0)
            {
                mainGrammarScopeName = grammarScopeByPath[testCase.grammars[0]];
            }
            else
            {
                throw new Exception("No grammars specified in test case");
            }
        }

        // Step 3: Set grammar injections if specified
        if (testCase.grammarInjections != null && testCase.grammarInjections.Count > 0)
        {
            textMate.SetInjections(mainGrammarScopeName, testCase.grammarInjections.ToArray());
        }

        // Step 4: Load the main grammar by scope name through the registry
        // This allows the registry to properly resolve includes and dependencies
        var grammar = textMate.LoadGrammar(mainGrammarScopeName);

        return RunTokenizationTest(grammar, testCase.lines);
    }

    static bool RunTokenizationTest(Grammar grammar, List<TestLine> testLines)
    {
        var state = TextMate.GetInitialState();

        for (int lineIndex = 0; lineIndex < testLines.Count; lineIndex++)
        {
            var testLine = testLines[lineIndex];
            var result = grammar.TokenizeLine(testLine.line, state);
            state = result.RuleStack!;

            // Build actual tokens
            var actualTokens = result.Tokens.Select(token => new TestToken
            {
                value = token.GetValue(testLine.line),
                scopes = token.Scopes
            }).ToList();

            // Filter expected tokens (skip empty tokens on non-empty lines like TypeScript does)
            var expectedTokens = testLine.tokens;
            if (testLine.line.Length > 0)
            {
                expectedTokens = testLine.tokens.Where(t => t.value.Length > 0).ToList();
            }

            // Compare token count
            if (actualTokens.Count != expectedTokens.Count)
            {
                Console.WriteLine();
                Console.WriteLine($"  Line {lineIndex + 1}: \"{testLine.line}\"");
                Console.WriteLine($"  Token count mismatch:");
                Console.WriteLine($"    Expected: {expectedTokens.Count} tokens");
                Console.WriteLine($"    Actual: {actualTokens.Count} tokens");
                Console.WriteLine($"  Expected tokens:");
                for (int i = 0; i < expectedTokens.Count; i++)
                {
                    var token = expectedTokens[i];
                    Console.WriteLine($"    [{i}] \"{token.value}\" scopes: {string.Join(", ", token.scopes)}");
                }
                Console.WriteLine($"  Actual tokens:");
                for (int i = 0; i < actualTokens.Count; i++)
                {
                    var token = actualTokens[i];
                    Console.WriteLine($"    [{i}] \"{token.value}\" scopes: {string.Join(", ", token.scopes)}");
                }
                return false;
            }

            // Compare each token
            for (int tokenIndex = 0; tokenIndex < actualTokens.Count; tokenIndex++)
            {
                var actualToken = actualTokens[tokenIndex];
                var expectedToken = expectedTokens[tokenIndex];

                // Check token value
                if (actualToken.value != expectedToken.value)
                {
                    Console.WriteLine();
                    Console.WriteLine($"  Line {lineIndex + 1}: \"{testLine.line}\"");
                    Console.WriteLine($"  Token {tokenIndex} value mismatch:");
                    Console.WriteLine($"    Expected: \"{expectedToken.value}\"");
                    Console.WriteLine($"    Actual: \"{actualToken.value}\"");
                    return false;
                }

                // Check token scopes
                if (!actualToken.scopes.SequenceEqual(expectedToken.scopes))
                {
                    Console.WriteLine();
                    Console.WriteLine($"  Line {lineIndex + 1}: \"{testLine.line}\"");
                    Console.WriteLine($"  Token {tokenIndex} (\"{actualToken.value}\") scope mismatch:");
                    Console.WriteLine($"    Expected: [{string.Join(", ", expectedToken.scopes)}]");
                    Console.WriteLine($"    Actual: [{string.Join(", ", actualToken.scopes)}]");
                    return false;
                }
            }
        }

        return true;
    }
}
