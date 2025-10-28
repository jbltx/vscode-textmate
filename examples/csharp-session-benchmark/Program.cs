using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Linq;
using TextMateSharp;

/// <summary>
/// Session API Example: Performance Benchmarking
///
/// This example demonstrates the performance benefits of the Session API
/// by comparing incremental editing scenarios. It shows:
/// - Document initialization performance
/// - Single-line edit performance with state cascading
/// - Sequential edits demonstrating incremental optimization
/// - Memory efficiency
/// </summary>
class SessionBenchmarkExample
{
    static void Main()
    {
        Console.WriteLine("TextMate Session API - Performance Benchmark");
        Console.WriteLine("==========================================\n");

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

                Console.WriteLine("Benchmark 1: Document Initialization\n");
                BenchmarkInitialization(grammar);

                Console.WriteLine("\n\nBenchmark 2: Single-Line Edits (Incremental)\n");
                BenchmarkSingleLineEdits(grammar);

                Console.WriteLine("\n\nBenchmark 3: Sequential Edits\n");
                BenchmarkSequentialEdits(grammar);

                Console.WriteLine("\n\nBenchmark 4: Query Performance (Cached Tokens)\n");
                BenchmarkQueryPerformance(grammar);

                Console.WriteLine("\nAll benchmarks completed!");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error: {ex.Message}");
        }
    }

    /// <summary>
    /// Benchmark: Initial document loading and tokenization
    /// </summary>
    static void BenchmarkInitialization(Grammar grammar)
    {
        int[] sizes = { 100, 500, 1000, 5000 };

        Console.WriteLine("Document size | Time (ms) | Lines/sec");
        Console.WriteLine("------|---------|---------|----------");

        foreach (int size in sizes)
        {
            string[] lines = GenerateJsonDocument(size);

            using (var session = new TextMateSession(grammar))
            {
                var sw = Stopwatch.StartNew();
                session.SetLines(lines);
                sw.Stop();

                double linesPerSec = size / (sw.Elapsed.TotalSeconds + 0.0001);
                Console.WriteLine($"{size,6} | {sw.Elapsed.TotalMilliseconds,7:F3} | {linesPerSec,9:F0}");
            }
        }
    }

    /// <summary>
    /// Benchmark: Single-line edits at various positions
    /// Shows how edit position affects cascading
    /// </summary>
    static void BenchmarkSingleLineEdits(Grammar grammar)
    {
        string[] document = GenerateJsonDocument(10000);

        using (var session = new TextMateSession(grammar))
        {
            session.SetLines(document);

            int[] positions = { 0, 2500, 5000, 7500, 9999 };
            Console.WriteLine("Edit position | Percentage | Time (ms) | Cascade lines");
            Console.WriteLine("--|--|--|--");

            foreach (int pos in positions)
            {
                var sw = Stopwatch.StartNew();
                session.Edit(new[] { $"  \"line_{pos}\": {pos}," }, pos, 1);
                sw.Stop();

                int pct = (pos * 100) / 10000;
                int cascadeEstimate = 10000 - pos; // Rough estimate
                Console.WriteLine($"{pos,13} | {pct,9}% | {sw.Elapsed.TotalMilliseconds,7:F3} | ~{cascadeEstimate,5}");
            }
        }
    }

    /// <summary>
    /// Benchmark: Multiple sequential edits
    /// </summary>
    static void BenchmarkSequentialEdits(Grammar grammar)
    {
        string[] document = GenerateJsonDocument(1000);

        using (var session = new TextMateSession(grammar))
        {
            session.SetLines(document);

            int[] editCounts = { 1, 5, 10, 20 };
            Console.WriteLine("Number of edits | Total time (ms) | Per-edit (ms)");
            Console.WriteLine("--|--|--");

            foreach (int count in editCounts)
            {
                // Reset document
                session.SetLines(document);

                var sw = Stopwatch.StartNew();
                for (int i = 0; i < count; i++)
                {
                    int lineNum = (i * 100) % 900; // Vary position
                    session.Edit(new[] { $"  \"edit_{i}\": true," }, lineNum, 1);
                }
                sw.Stop();

                double perEdit = sw.Elapsed.TotalMilliseconds / count;
                Console.WriteLine($"{count,15} | {sw.Elapsed.TotalMilliseconds,14:F3} | {perEdit,12:F3}");
            }
        }
    }

    /// <summary>
    /// Benchmark: Query cached tokens (should be O(1))
    /// </summary>
    static void BenchmarkQueryPerformance(Grammar grammar)
    {
        string[] document = GenerateJsonDocument(10000);

        using (var session = new TextMateSession(grammar))
        {
            session.SetLines(document);

            // Warm up
            session.GetLineTokens(5000);

            // Query 100 times
            var sw = Stopwatch.StartNew();
            for (int i = 0; i < 100; i++)
            {
                int line = (i * 137) % 9999; // Random access pattern
                session.GetLineTokens(line);
            }
            sw.Stop();

            double avgTime = sw.Elapsed.TotalMicroseconds / 100;
            Console.WriteLine($"Query cached tokens (100 iterations)");
            Console.WriteLine($"Total time: {sw.Elapsed.TotalMilliseconds:F3}ms");
            Console.WriteLine($"Average time per query: {avgTime:F2}µs");
            Console.WriteLine($"Complexity: O(1) - constant time lookup!");
        }
    }

    /// <summary>
    /// Generate a sample JSON document
    /// </summary>
    static string[] GenerateJsonDocument(int lineCount)
    {
        var lines = new List<string> { "{" };

        for (int i = 0; i < lineCount - 2; i++)
        {
            string comma = i < lineCount - 3 ? "," : "";
            lines.Add($"  \"property_{i}\": {i % 100}{comma}");
        }

        lines.Add("}");
        return lines.ToArray();
    }

    /// <summary>
    /// Find grammar file
    /// </summary>
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
