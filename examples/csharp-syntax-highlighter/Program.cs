using TextMateSharp;
using System;
using System.Collections.Generic;
using System.Linq;

/// <summary>
/// Example: Using SyntaxHighlighter for complete syntax highlighting
/// This demonstrates the high-level API that combines tokenization with theme styling
/// </summary>
class SyntaxHighlighterExample
{
    static void Main()
    {
        Console.WriteLine("=== SyntaxHighlighter Example ===\n");

        try
        {
            // Step 1: Initialize TextMate
            Console.WriteLine("1. Initializing TextMate...");
            using (var tm = new TextMate())
            {
                // Step 2: Load grammar and theme
                // (Assumes you have grammar and theme files available)
                Console.WriteLine("2. Loading JavaScript grammar and theme...");

                // Add JavaScript grammar
                tm.AddGrammarFromJson(GetJavaScriptGrammarJson());

                // Load the grammar
                var grammar = tm.LoadGrammar("source.javascript");

                // Create a basic theme (monochrome for this example)
                var theme = new Theme();
                Console.WriteLine("3. Theme created\n");

                // Step 3: Create syntax highlighter
                Console.WriteLine("4. Creating SyntaxHighlighter...");
                using (var highlighter = new SyntaxHighlighter(grammar, theme))
                {
                    // Step 4: Load sample code
                    var sampleCode = new[]
                    {
                        "// Fibonacci function",
                        "function fibonacci(n) {",
                        "    if (n <= 1) return n;",
                        "    return fibonacci(n-1) + fibonacci(n-2);",
                        "}",
                        "",
                        "const result = fibonacci(10);",
                        "console.log(result);  // Output: 55"
                    };

                    Console.WriteLine("5. Loading sample code...");
                    highlighter.SetDocument(sampleCode);
                    Console.WriteLine($"   Loaded {highlighter.GetLineCount()} lines\n");

                    // Step 5: Get highlighted lines
                    Console.WriteLine("6. Highlighting lines:");
                    Console.WriteLine(new string('=', 70));

                    for (int i = 0; i < 3; i++)
                    {
                        var line = highlighter.GetHighlightedLine(i);
                        PrintHighlightedLine(line);
                    }

                    // Step 6: Demo editing
                    Console.WriteLine("\n7. Editing line 6...");
                    Console.WriteLine(new string('=', 70));
                    highlighter.EditLine(6, "const result = fibonacci(20);  // Updated");
                    var editedLine = highlighter.GetHighlightedLine(6);
                    PrintHighlightedLine(editedLine);

                    // Step 7: Demo theme switching
                    Console.WriteLine("\n8. Switching theme...");
                    var darkTheme = new Theme();
                    highlighter.SetTheme(darkTheme);
                    var rethemedLine = highlighter.GetHighlightedLine(0);
                    Console.WriteLine($"   Line 0 after theme switch: {rethemedLine.Tokens.Count} tokens");

                    // Step 8: Demo cache
                    Console.WriteLine("\n9. Cache management...");
                    Console.WriteLine("   Clearing cache...");
                    highlighter.ClearCache();
                    Console.WriteLine("   Cache cleared!");
                }

                Console.WriteLine("\n✓ Example completed successfully!");
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"✗ Error: {ex.Message}");
            Console.Error.WriteLine(ex.StackTrace);
        }
    }

    static void PrintHighlightedLine(HighlightedLine line)
    {
        Console.WriteLine($"\nLine {line.LineIndex}: {line.Content}");
        Console.WriteLine($"Complete: {line.IsComplete}, Tokens: {line.Tokens.Count}");

        foreach (var token in line.Tokens)
        {
            var text = token.GetText(line.Content);
            Console.WriteLine($"  [{token.StartIndex:D2}-{token.EndIndex:D2}] '{text}'");
            if (!string.IsNullOrEmpty(token.ForegroundColor))
            {
                Console.WriteLine($"    Color: {token.ForegroundColor}");
            }
            if (token.FontStyle != 0)
            {
                Console.WriteLine($"    Style: {token.FontStyle}");
            }
            if (token.Scopes.Any())
            {
                Console.WriteLine($"    Scope: {string.Join(" > ", token.Scopes)}");
            }
        }
    }

    static string GetJavaScriptGrammarJson()
    {
        // Minimal JavaScript grammar for example
        // In real usage, load from proper grammar files
        return @"{
            ""name"": ""JavaScript"",
            ""scopeName"": ""source.javascript"",
            ""patterns"": [
                { ""name"": ""comment.line.js"", ""match"": ""//.*"" },
                { ""name"": ""keyword.js"", ""match"": ""\b(function|const|let|var|return|if|else)\b"" },
                { ""name"": ""string.quoted.js"", ""match"": ""['\""](.*?)['\""]\|""  }
            ]
        }";
    }
}

/// <summary>
/// Extension: Advanced usage with real editor integration
/// </summary>
namespace SyntaxHighlighterAdvanced
{
    /// <summary>
    /// Example text editor using SyntaxHighlighter
    /// </summary>
    public class TextEditor
    {
        private SyntaxHighlighter _highlighter;
        private List<string> _lines;

        public TextEditor(Grammar grammar, Theme theme)
        {
            _highlighter = new SyntaxHighlighter(grammar, theme);
            _lines = new List<string>();
        }

        public void LoadFile(string[] content)
        {
            _lines = new List<string>(content);
            _highlighter.SetDocument(_lines);
        }

        public void InsertLine(int index, string content)
        {
            _lines.Insert(index, content);
            _highlighter.InsertLines(index, new[] { content });
        }

        public void UpdateLine(int index, string newContent)
        {
            if (index >= 0 && index < _lines.Count)
            {
                _lines[index] = newContent;
                _highlighter.EditLine(index, newContent);
            }
        }

        public void RemoveLine(int index)
        {
            if (index >= 0 && index < _lines.Count)
            {
                _lines.RemoveAt(index);
                _highlighter.RemoveLines(index, 1);
            }
        }

        public HighlightedLine RenderLine(int index)
        {
            return _highlighter.GetHighlightedLine(index);
        }

        public List<HighlightedLine> RenderViewport(int startLine, int endLine)
        {
            return _highlighter.GetHighlightedRange(startLine, Math.Min(endLine, _lines.Count - 1));
        }

        public void SwitchTheme(Theme newTheme)
        {
            _highlighter.SetTheme(newTheme);
        }

        public void Dispose()
        {
            _highlighter?.Dispose();
        }
    }

    /// <summary>
    /// Example: Using TextEditor with real-time editing
    /// </summary>
    public class EditorExample
    {
        public static void RunExample(Grammar grammar, Theme theme)
        {
            using (var editor = new TextEditor(grammar, theme))
            {
                var code = new[]
                {
                    "class Calculator {",
                    "    add(a, b) { return a + b; }",
                    "    subtract(a, b) { return a - b; }",
                    "}"
                };

                editor.LoadFile(code);

                // Display initial content
                Console.WriteLine("Initial code:");
                for (int i = 0; i < editor._lines.Count; i++)
                {
                    var line = editor.RenderLine(i);
                    Console.WriteLine($"{i}: {line.Content} ({line.Tokens.Count} tokens)");
                }

                // Edit a line
                Console.WriteLine("\nEditing line 1...");
                editor.UpdateLine(1, "    multiply(a, b) { return a * b; }");
                var edited = editor.RenderLine(1);
                Console.WriteLine($"{1}: {edited.Content} ({edited.Tokens.Count} tokens)");

                // Insert a line
                Console.WriteLine("\nInserting new method...");
                editor.InsertLine(3, "    divide(a, b) { return b != 0 ? a / b : null; }");
                var viewport = editor.RenderViewport(0, 100);
                foreach (var l in viewport)
                {
                    Console.WriteLine($"{l.LineIndex}: {l.Content}");
                }
            }
        }
    }
}
