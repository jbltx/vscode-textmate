using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using TextMateSharp;

/// <summary>
/// Session API Example: Language Server Protocol (LSP) Integration Pattern
///
/// This example demonstrates how to integrate the Session API into a
/// Language Server Protocol implementation. It simulates:
/// - Opening a document (initialize session)
/// - Handling document change notifications (incremental edits)
/// - Providing semantic tokens for diagnostics
/// - Managing version tracking and performance
/// </summary>
class SessionLSPExample
{
    /// <summary>
    /// Simulates an LSP document with incremental tokenization
    /// </summary>
    class LSPDocument
    {
        public string Uri { get; set; }
        public int Version { get; set; }
        public TextMateSession Session { get; set; }
        public string[] Lines { get; set; }
        private readonly Stopwatch _totalTime = Stopwatch.StartNew();

        public LSPDocument(string uri, string[] initialLines, TextMateSession session)
        {
            Uri = uri;
            Version = 1;
            Lines = initialLines;
            Session = session;
            Session.SetLines(Lines);
        }

        /// <summary>
        /// Handle single-line text document change (e.g., user typing)
        /// </summary>
        public void OnDidChange(int lineNumber, int lineStart, int lineEnd, string newText)
        {
            Version++;
            Lines[lineNumber] = newText;
            Session.Edit(new[] { newText }, lineNumber, 1);

            Console.WriteLine($"  [{Version}] Line {lineNumber} changed: {newText.Substring(0, Math.Min(40, newText.Length))}...");
        }

        /// <summary>
        /// Handle multi-line document change (e.g., paste operation)
        /// </summary>
        public void OnDidChangeMultiLine(int startLine, int endLine, string[] newLines)
        {
            Version++;
            int replaceCount = endLine - startLine + 1;

            // Update array
            if (newLines.Length != replaceCount)
            {
                Array.Resize(ref Lines, Lines.Length - replaceCount + newLines.Length);
            }

            // Shift and insert
            for (int i = 0; i < newLines.Length; i++)
            {
                Lines[startLine + i] = newLines[i];
            }

            Session.Edit(newLines, startLine, replaceCount);
            Console.WriteLine($"  [{Version}] Lines {startLine}-{endLine} changed ({newLines.Length} new lines)");
        }

        /// <summary>
        /// Get semantic tokens for a range (for diagnostics/highlighting)
        /// </summary>
        public List<SemanticToken> GetSemanticTokens(int lineStart, int lineEnd)
        {
            var tokens = new List<SemanticToken>();

            for (int lineNum = lineStart; lineNum <= lineEnd && lineNum < Lines.Length; lineNum++)
            {
                var lineResult = Session.GetLineTokens(lineNum);
                if (lineResult == null) continue;

                foreach (var token in lineResult.Tokens)
                {
                    tokens.Add(new SemanticToken
                    {
                        Line = lineNum,
                        StartChar = token.StartIndex,
                        EndChar = token.EndIndex,
                        Text = token.GetValue(Lines[lineNum]),
                        Scopes = token.Scopes
                    });
                }
            }

            return tokens;
        }

        public SessionMetadata GetMetadata()
        {
            return Session.GetMetadata();
        }

        public string GetStats()
        {
            var meta = GetMetadata();
            return $"[v{Version}] {meta.LineCount} lines, {meta.CachedLineCount} cached, {meta.MemoryUsageBytes} bytes";
        }
    }

    class SemanticToken
    {
        public int Line { get; set; }
        public int StartChar { get; set; }
        public int EndChar { get; set; }
        public string Text { get; set; }
        public List<string> Scopes { get; set; }
    }

    static void Main()
    {
        Console.WriteLine("TextMate Session API - Language Server Protocol Integration");
        Console.WriteLine("========================================================\n");

        try
        {
            string grammarPath = FindGrammarFile("json.json");
            if (!File.Exists(grammarPath))
            {
                Console.WriteLine($"Grammar not found at: {grammarPath}");
                return;
            }

            using (var textmate = new TextMate())
            {
                textmate.AddGrammarFromFile(grammarPath);
                var grammar = textmate.LoadGrammar("source.json");

                // Simulate LSP document open
                Console.WriteLine("1. Client: textDocument/didOpen");
                var initialLines = new[]
                {
                    "{",
                    "  \"name\": \"TextMate\",",
                    "  \"version\": \"1.0\",",
                    "  \"enabled\": true",
                    "}"
                };

                using (var session = new TextMateSession(grammar))
                {
                    var doc = new LSPDocument("file:///example.json", initialLines, session);
                    Console.WriteLine($"   Opened document: {doc.Uri}");
                    Console.WriteLine($"   {doc.GetStats()}\n");

                    // Simulate incremental changes
                    Console.WriteLine("2. Client: textDocument/didChange (incremental updates)");

                    // Change 1: Edit version number
                    Console.WriteLine("\n   Change 1: Update version");
                    var sw = Stopwatch.StartNew();
                    doc.OnDidChange(2, 0, 28, "  \"version\": \"2.0\",");
                    sw.Stop();
                    Console.WriteLine($"   Time: {sw.Elapsed.TotalMilliseconds:F2}ms");
                    Console.WriteLine($"   {doc.GetStats()}");

                    // Change 2: Edit enabled flag
                    Console.WriteLine("\n   Change 2: Toggle enabled");
                    sw.Restart();
                    doc.OnDidChange(3, 0, 24, "  \"enabled\": false");
                    sw.Stop();
                    Console.WriteLine($"   Time: {sw.Elapsed.TotalMilliseconds:F2}ms");
                    Console.WriteLine($"   {doc.GetStats()}");

                    // Change 3: Add new property
                    Console.WriteLine("\n   Change 3: Add property");
                    sw.Restart();
                    doc.OnDidChangeMultiLine(3, 3, new[]
                    {
                        "  \"enabled\": false,",
                        "  \"author\": \"TextMate\""
                    });
                    sw.Stop();
                    Console.WriteLine($"   Time: {sw.Elapsed.TotalMilliseconds:F2}ms");
                    Console.WriteLine($"   {doc.GetStats()}");

                    // Request semantic tokens for diagnostics
                    Console.WriteLine("\n3. Client: textDocument/semanticTokens/range");
                    Console.WriteLine("   Requesting tokens for display/analysis...\n");

                    var tokens = doc.GetSemanticTokens(0, doc.Lines.Length - 1);
                    Console.WriteLine($"   Retrieved {tokens.Count} semantic tokens:");
                    foreach (var token in tokens.Take(10))
                    {
                        string scopeChain = string.Join(" > ", token.Scopes);
                        Console.WriteLine($"     [{token.Text}] @ line {token.Line}: {scopeChain}");
                    }
                    if (tokens.Count > 10)
                    {
                        Console.WriteLine($"     ... and {tokens.Count - 10} more");
                    }

                    // Show how this differs from manual state management
                    Console.WriteLine("\n4. Key Benefits for LSP Implementation:");
                    Console.WriteLine("   ✓ Automatic incremental tokenization");
                    Console.WriteLine("   ✓ No manual state tracking between edits");
                    Console.WriteLine("   ✓ Memory-safe with automatic cleanup");
                    Console.WriteLine("   ✓ Version tracking built-in");
                    Console.WriteLine("   ✓ O(1) token queries for diagnostics");
                    Console.WriteLine("   ✓ Scales to large documents (10K+ lines)");

                    Console.WriteLine("\n5. Integration Pattern:");
                    Console.WriteLine("   LSP Client");
                    Console.WriteLine("      |");
                    Console.WriteLine("      v didOpen/didChange");
                    Console.WriteLine("   LSPDocument (wrapper)");
                    Console.WriteLine("      |");
                    Console.WriteLine("      v");
                    Console.WriteLine("   TextMateSession (stateful)");
                    Console.WriteLine("      |");
                    Console.WriteLine("      v");
                    Console.WriteLine("   TextMate Grammar (tokenization)");
                }
            }

            Console.WriteLine("\nExample completed successfully!");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error: {ex.Message}");
        }
    }

    static string FindGrammarFile(string grammarName)
    {
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
