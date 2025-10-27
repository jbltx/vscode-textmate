using System;
using System.IO;
using System.Text.Json;
using System.Collections.Generic;
using System.Diagnostics;
using TextMateSharp;

class BenchmarkLarge
{
    class BenchmarkCase
    {
        public string Name { get; set; } = "";
        public string FilePath { get; set; } = "";
        public string GrammarPath { get; set; } = "";
        public string ScopeName { get; set; } = "";
    }

    class BenchmarkResult
    {
        public string TestName { get; set; } = "";
        public int LineCount { get; set; }
        public int TokenCount { get; set; }
        public int CharCount { get; set; }
        public long ElapsedMs { get; set; }
        public double LinesPerSecond => LineCount / (ElapsedMs / 1000.0);
        public double TokensPerSecond => TokenCount / (ElapsedMs / 1000.0);
        public double MBPerSecond => (CharCount / 1024.0 / 1024.0) / (ElapsedMs / 1000.0);
    }

    static int Main(string[] args)
    {
        Console.WriteLine("TextMate Large File Benchmark (C#)");
        Console.WriteLine("===================================\n");

        string benchmarkPath = args.Length > 0 ? args[0] : "../../benchmark";

        if (!Directory.Exists(benchmarkPath))
        {
            Console.WriteLine($"Error: Benchmark directory not found: {benchmarkPath}");
            return 1;
        }

        // Define benchmark cases
        var benchmarkCases = new List<BenchmarkCase>
        {
            new BenchmarkCase
            {
                Name = "jQuery v2.0.3",
                FilePath = Path.Combine(benchmarkPath, "large.js.txt"),
                GrammarPath = Path.Combine(benchmarkPath, "JavaScript.tmLanguage.json"),
                ScopeName = "source.js"
            },
            new BenchmarkCase
            {
                Name = "vscode.d.ts",
                FilePath = Path.Combine(benchmarkPath, "vscode.d.ts.txt"),
                GrammarPath = "../../test-cases/themes/syntaxes/TypeScript.tmLanguage.json",
                ScopeName = "source.ts"
            },
            new BenchmarkCase
            {
                Name = "Bootstrap CSS v3.1.1",
                FilePath = Path.Combine(benchmarkPath, "bootstrap.css.txt"),
                GrammarPath = "../../test-cases/first-mate/fixtures/css.json",
                ScopeName = "source.css"
            },
            new BenchmarkCase
            {
                Name = "JavaScript Grammar (JSON)",
                FilePath = Path.Combine(benchmarkPath, "JavaScript.tmLanguage.json.txt"),
                GrammarPath = "../../test-cases/themes/syntaxes/JSON.json",
                ScopeName = "source.json"
            },
            new BenchmarkCase
            {
                Name = "Bootstrap CSS minified",
                FilePath = Path.Combine(benchmarkPath, "bootstrap.min.css.txt"),
                GrammarPath = "../../test-cases/first-mate/fixtures/css.json",
                ScopeName = "source.css"
            },
            new BenchmarkCase
            {
                Name = "jQuery minified",
                FilePath = Path.Combine(benchmarkPath, "large.min.js.txt"),
                GrammarPath = Path.Combine(benchmarkPath, "JavaScript.tmLanguage.json"),
                ScopeName = "source.js"
            },
            new BenchmarkCase
            {
                Name = "Bootstrap multi-byte minified",
                FilePath = Path.Combine(benchmarkPath, "main.08642f99.css.txt"),
                GrammarPath = "../../test-cases/first-mate/fixtures/css.json",
                ScopeName = "source.css"
            },
            new BenchmarkCase
            {
                Name = "Simple minified JS",
                FilePath = Path.Combine(benchmarkPath, "minified.js.txt"),
                GrammarPath = Path.Combine(benchmarkPath, "JavaScript.tmLanguage.json"),
                ScopeName = "source.js"
            }
        };

        // Number of iterations
        int warmupRuns = 1;
        int benchmarkRuns = 3;

        Console.WriteLine($"Warmup runs: {warmupRuns}, Benchmark runs: {benchmarkRuns}\n");

        var results = new List<BenchmarkResult>();

        foreach (var testCase in benchmarkCases)
        {
            if (!File.Exists(testCase.FilePath))
            {
                Console.WriteLine($"Skipping {testCase.Name}: File not found");
                continue;
            }

            if (!File.Exists(testCase.GrammarPath))
            {
                Console.WriteLine($"Skipping {testCase.Name}: Grammar not found");
                continue;
            }

            Console.Write($"Benchmarking {testCase.Name}... ");

            try
            {
                // Read file once
                var content = File.ReadAllText(testCase.FilePath);
                var lines = content.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.None);
                int charCount = content.Length;

                // Load grammar once (outside benchmark timing)
                using var textMate = new TextMate();
                var grammarJson = File.ReadAllText(testCase.GrammarPath);
                textMate.AddGrammarFromJson(grammarJson);
                var grammar = textMate.LoadGrammar(testCase.ScopeName);

                // Warmup runs
                for (int i = 0; i < warmupRuns; i++)
                {
                    RunTokenization(grammar, lines, out _, out _);
                }

                // Benchmark runs
                var times = new List<long>();
                int lineCount = 0;
                int tokenCount = 0;

                for (int i = 0; i < benchmarkRuns; i++)
                {
                    var elapsed = RunTokenization(grammar, lines, out lineCount, out tokenCount);
                    times.Add(elapsed);
                }

                // Use median time
                times.Sort();
                long medianTime = times[times.Count / 2];

                results.Add(new BenchmarkResult
                {
                    TestName = testCase.Name,
                    LineCount = lineCount,
                    TokenCount = tokenCount,
                    CharCount = charCount,
                    ElapsedMs = medianTime
                });

                Console.WriteLine($"✓ ({medianTime} ms)");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"✗ ERROR: {ex.Message}");
            }
        }

        // Display results
        Console.WriteLine("\n" + new string('=', 110));
        Console.WriteLine($"{"Test Name",-35} {"Lines",-10} {"Tokens",-12} {"MB",-8} {"Time(ms)",-10} {"Lines/s",-10} {"Tokens/s",-12} {"MB/s",-8}");
        Console.WriteLine(new string('=', 110));

        long totalTime = 0;
        int totalLines = 0;
        int totalTokens = 0;
        int totalChars = 0;

        foreach (var result in results)
        {
            double mb = result.CharCount / 1024.0 / 1024.0;
            Console.WriteLine(
                $"{result.TestName,-35} {result.LineCount,-10} {result.TokenCount,-12} {mb,-8:F2} " +
                $"{result.ElapsedMs,-10} {result.LinesPerSecond,-10:F0} {result.TokensPerSecond,-12:F0} {result.MBPerSecond,-8:F2}"
            );
            totalTime += result.ElapsedMs;
            totalLines += result.LineCount;
            totalTokens += result.TokenCount;
            totalChars += result.CharCount;
        }

        Console.WriteLine(new string('=', 110));
        double totalMB = totalChars / 1024.0 / 1024.0;
        Console.WriteLine($"{"TOTAL",-35} {totalLines,-10} {totalTokens,-12} {totalMB,-8:F2} {totalTime,-10}");
        Console.WriteLine(new string('=', 110));

        double overallLinesPerSec = totalLines / (totalTime / 1000.0);
        double overallTokensPerSec = totalTokens / (totalTime / 1000.0);
        double overallMBPerSec = totalMB / (totalTime / 1000.0);

        Console.WriteLine($"\nOverall Performance:");
        Console.WriteLine($"  Lines/sec:  {overallLinesPerSec:F0}");
        Console.WriteLine($"  Tokens/sec: {overallTokensPerSec:F0}");
        Console.WriteLine($"  MB/sec:     {overallMBPerSec:F2}");
        Console.WriteLine();

        return 0;
    }

    static long RunTokenization(Grammar grammar, string[] lines, out int lineCount, out int tokenCount)
    {
        var sw = Stopwatch.StartNew();

        // Use batch API (Phase 2 optimization)
        var state = TextMate.GetInitialState();
        var results = grammar.TokenizeLines(lines, state);

        sw.Stop();

        // Count total tokens
        int tokens = 0;
        foreach (var result in results)
        {
            tokens += result.Tokens.Count;
        }

        lineCount = lines.Length;
        tokenCount = tokens;
        return sw.ElapsedMilliseconds;
    }
}
