using System;
using System.Linq;
using System.Text;
using TextMateSharp;

class TestUnicode
{
    public static void TestMarshalingIssue()
    {
        string testLine = "'𝞗'";

        Console.WriteLine($"Original string: {testLine}");
        Console.WriteLine($"String length (C# chars): {testLine.Length}");
        Console.WriteLine($"Char codes: {string.Join(", ", testLine.ToCharArray().Select(c => ((int)c).ToString("X4")))}");

        // Convert to UTF-8
        byte[] utf8Bytes = Encoding.UTF8.GetBytes(testLine);
        Console.WriteLine($"UTF-8 byte length: {utf8Bytes.Length}");
        Console.WriteLine($"UTF-8 bytes: {BitConverter.ToString(utf8Bytes)}");

        // Test tokenization
        using var textMate = new TextMate();

        // Load JavaScript grammar
        string grammarPath = "../../test-cases/first-mate/fixtures/javascript.json";
        var grammarJson = System.IO.File.ReadAllText(grammarPath);
        textMate.AddGrammarFromJson(grammarJson);
        var grammar = textMate.LoadGrammar("source.js");

        var state = TextMate.GetInitialState();
        var result = grammar.TokenizeLine(testLine, state);

        Console.WriteLine($"\nTokenization result:");
        Console.WriteLine($"Token count: {result.Tokens.Count}");

        for (int i = 0; i < result.Tokens.Count; i++)
        {
            var token = result.Tokens[i];
            string value = token.GetValue(testLine);
            Console.WriteLine($"Token {i}: StartIndex={token.StartIndex}, EndIndex={token.EndIndex}");
            Console.WriteLine($"  Value: '{value}'");
            Console.WriteLine($"  Char codes: {string.Join(", ", value.ToCharArray().Select(c => ((int)c).ToString("X4")))}");
            Console.WriteLine($"  Scopes: {string.Join(", ", token.Scopes)}");
        }
    }
}
