using System;
using System.IO;
using TextMateSharp;

class Program
{
    static void Main(string[] args)
    {
        Console.WriteLine("TextMate C# Playground");
        Console.WriteLine("=====================\n");

        try
        {
            // Initialize TextMate
            using var textMate = new TextMate();

            // Determine grammar path
            string grammarPath;
            if (args.Length > 0)
            {
                grammarPath = args[0];
            }
            else
            {
                // Default to JavaScript grammar from test-cases
                grammarPath = Path.GetFullPath("../../test-cases/first-mate/fixtures/javascript.json");
            }

            if (!File.Exists(grammarPath))
            {
                Console.WriteLine($"Error: Grammar file not found: {grammarPath}");
                Console.WriteLine("\nUsage: dotnet run [path-to-grammar.json]");
                Console.WriteLine("Example: dotnet run ../../test-cases/first-mate/fixtures/javascript.json");
                return;
            }

            Console.WriteLine($"Adding grammar: {grammarPath}");
            textMate.AddGrammarFromFile(grammarPath);

            // Parse the grammar to get its scope name
            var grammarJson = File.ReadAllText(grammarPath);
            var grammarDoc = System.Text.Json.JsonDocument.Parse(grammarJson);
            var scopeName = grammarDoc.RootElement.GetProperty("scopeName").GetString();

            Console.WriteLine($"Loading grammar by scope: {scopeName}");
            var grammar = textMate.LoadGrammar(scopeName!);
            Console.WriteLine("Grammar loaded successfully!\n");

            // Sample JavaScript code to tokenize
            string[] lines = {
                "function hello(name) {",
                "  console.log('Hello, ' + name);",
                "  return true;",
                "}"
            };

            Console.WriteLine("Tokenizing JavaScript code:");
            Console.WriteLine("---------------------------\n");

            // Tokenize line by line
            var state = TextMate.GetInitialState();

            foreach (var line in lines)
            {
                Console.WriteLine($"Line: {line}");

                var result = grammar.TokenizeLine(line, state);
                state = result.RuleStack!;

                foreach (var token in result.Tokens)
                {
                    var text = line.Substring(token.StartIndex, token.EndIndex - token.StartIndex);
                    Console.WriteLine($"  [{token.StartIndex,3}:{token.EndIndex,3}] '{text}'");
                    foreach (var scope in token.Scopes)
                    {
                        Console.WriteLine($"            - {scope}");
                    }
                }

                Console.WriteLine();
            }

            // Demonstrate tokenizeLine2 (encoded tokens)
            Console.WriteLine("\nTokenizing with encoded tokens (tokenizeLine2):");
            Console.WriteLine("-----------------------------------------------\n");

            state = TextMate.GetInitialState();
            foreach (var line in lines)
            {
                Console.WriteLine($"Line: {line}");

                var result = grammar.TokenizeLine2(line, state);
                state = result.RuleStack!;

                Console.WriteLine($"  Token count: {result.Tokens.Length / 2}");
                Console.WriteLine($"  Encoded tokens: [{string.Join(", ", result.Tokens)}]");
                Console.WriteLine();
            }

            Console.WriteLine("\n✓ Tokenization completed successfully!");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error: {ex.Message}");
            Console.WriteLine($"Stack trace: {ex.StackTrace}");
            Environment.Exit(1);
        }
    }
}
