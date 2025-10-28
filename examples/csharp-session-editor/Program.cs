using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using TextMateSharp;

/// <summary>
/// Session API Example: Basic Editor Simulation
///
/// This example demonstrates the high-level Session API for incremental text editing.
/// It simulates a simple text editor with:
/// - Loading a document
/// - Making edits to individual lines
/// - Observing how state cascades efficiently
/// - Displaying tokens for rendering
/// </summary>
class SessionEditorExample
{
    static void Main()
    {
        Console.WriteLine("TextMate Session API - Editor Simulation Example");
        Console.WriteLine("================================================\n");

        try
        {
            // Find grammar file
            string grammarPath = FindGrammarFile("json.json");
            if (!File.Exists(grammarPath))
            {
                Console.WriteLine($"Grammar not found at: {grammarPath}");
                Console.WriteLine("Please run this from the examples/csharp-session-editor directory");
                return;
            }

            // Create and load grammar
            Console.WriteLine("1. Loading JSON grammar...");
            using (var textmate = new TextMate())
            {
                textmate.AddGrammarFromFile(grammarPath);
                var grammar = textmate.LoadGrammar("source.json");

                // Create a simple JSON document
                string[] document = new[]
                {
                    "{",
                    "  \"name\": \"example\",",
                    "  \"value\": 42,",
                    "  \"active\": true",
                    "}"
                };

                Console.WriteLine("2. Creating session and loading document...");
                using (var session = new TextMateSession(grammar))
                {
                    session.SetLines(document);
                    Console.WriteLine($"   Loaded {session.GetLineCount()} lines\n");

                    // Display initial tokens
                    Console.WriteLine("3. Initial tokenization (before edits):");
                    DisplayTokens(session, document);

                    // Simulate user edits
                    Console.WriteLine("\n4. Simulating user edits...");

                    // Edit 1: Change a value
                    Console.WriteLine("\n   Edit 1: Change line 2 value from 42 to 100");
                    var sw = Stopwatch.StartNew();
                    session.Edit(new[] { "  \"value\": 100," }, 2, 1);
                    sw.Stop();
                    Console.WriteLine($"   Time: {sw.Elapsed.TotalMilliseconds:F2}ms (incremental!)");
                    document[2] = "  \"value\": 100,";

                    // Display tokens after edit
                    Console.WriteLine("   Tokens after edit:");
                    DisplayLineTokens(session, document, 2);

                    // Edit 2: Add a new property
                    Console.WriteLine("\n   Edit 2: Insert new property at line 4");
                    sw.Restart();
                    session.Add(new[] { "  \"updated\": \"2024-10-27\"," }, 4);
                    sw.Stop();
                    Console.WriteLine($"   Time: {sw.Elapsed.TotalMilliseconds:F2}ms");

                    // Update document array
                    Array.Resize(ref document, document.Length + 1);
                    for (int i = document.Length - 1; i > 4; i--)
                    {
                        document[i] = document[i - 1];
                    }
                    document[4] = "  \"updated\": \"2024-10-27\",";

                    Console.WriteLine($"   Now {session.GetLineCount()} lines total");
                    DisplayLineTokens(session, document, 4);

                    // Edit 3: Remove a property
                    Console.WriteLine("\n   Edit 3: Remove the updated property");
                    sw.Restart();
                    session.Remove(4, 1);
                    sw.Stop();
                    Console.WriteLine($"   Time: {sw.Elapsed.TotalMilliseconds:F2}ms");

                    // Show metadata
                    Console.WriteLine("\n5. Session Metadata:");
                    var metadata = session.GetMetadata();
                    Console.WriteLine($"   Lines in session: {metadata.LineCount}");
                    Console.WriteLine($"   Cached lines: {metadata.CachedLineCount}");
                    Console.WriteLine($"   Memory usage: {metadata.MemoryUsageBytes} bytes");
                    Console.WriteLine($"   Reference count: {metadata.ReferenceCount}");

                    Console.WriteLine("\n6. Key Benefits of Session API:");
                    Console.WriteLine("   ✓ Automatic state cascading");
                    Console.WriteLine("   ✓ Early stopping optimization");
                    Console.WriteLine("   ✓ Memory-safe with IDisposable");
                    Console.WriteLine("   ✓ Simple high-level API");
                    Console.WriteLine("   ✓ No manual state management");
                }
            }

            Console.WriteLine("\nExample completed successfully!");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error: {ex.Message}");
            if (ex.InnerException != null)
            {
                Console.WriteLine($"Inner: {ex.InnerException.Message}");
            }
        }
    }

    /// <summary>
    /// Display tokens for all lines in document
    /// </summary>
    static void DisplayTokens(TextMateSession session, string[] document)
    {
        for (int i = 0; i < document.Length; i++)
        {
            DisplayLineTokens(session, document, i);
        }
    }

    /// <summary>
    /// Display tokens for a single line
    /// </summary>
    static void DisplayLineTokens(TextMateSession session, string[] document, int lineIndex)
    {
        var result = session.GetLineTokens(lineIndex);
        if (result == null || result.Tokens.Count == 0)
        {
            Console.WriteLine($"   Line {lineIndex}: (no tokens)");
            return;
        }

        Console.WriteLine($"   Line {lineIndex}: {document[lineIndex]}");
        foreach (var token in result.Tokens)
        {
            string value = token.GetValue(document[lineIndex]);
            string scopes = string.Join(" > ", token.Scopes);
            Console.WriteLine($"      [{value}] -> {scopes}");
        }
    }

    /// <summary>
    /// Find grammar file in test-cases directory
    /// </summary>
    static string FindGrammarFile(string grammarName)
    {
        // Try relative to current directory
        string[] candidates = new[]
        {
            $"../../test-cases/first-mate/fixtures/{grammarName}",
            $"../../../test-cases/first-mate/fixtures/{grammarName}",
            Path.Combine(AppContext.BaseDirectory, $"../../test-cases/first-mate/fixtures/{grammarName}"),
        };

        foreach (var path in candidates)
        {
            string fullPath = Path.GetFullPath(path);
            if (File.Exists(fullPath))
            {
                return fullPath;
            }
        }

        return Path.GetFullPath(candidates[0]);
    }
}
