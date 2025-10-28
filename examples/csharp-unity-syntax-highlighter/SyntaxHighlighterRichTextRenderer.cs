using TextMateSharp;
using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;

/// <summary>
/// Renders syntax-highlighted code as Unity UI Toolkit rich text
///
/// Example output:
/// <mspace=0.6em><color=#FF0000><b><noparse>const</noparse></b></color> x = 5;</mspace>
/// </summary>
public class SyntaxHighlighterRichTextRenderer
{
    private SyntaxHighlighter _highlighter;

    /// <summary>
    /// Font style flags matching TextMate
    /// </summary>
    private const int FONT_STYLE_ITALIC = 1;
    private const int FONT_STYLE_BOLD = 2;
    private const int FONT_STYLE_UNDERLINE = 4;
    private const int FONT_STYLE_STRIKETHROUGH = 8;

    public SyntaxHighlighterRichTextRenderer(Grammar grammar, Theme theme, bool enableCache = true)
    {
        _highlighter = new SyntaxHighlighter(grammar, theme, enableCache);
    }

    /// <summary>
    /// Render a single line of code with syntax highlighting as rich text
    /// </summary>
    public string RenderLine(int lineIndex, string monospaceEmSize = "0.6em", bool wrapInMspace = true)
    {
        var line = _highlighter.GetHighlightedLine(lineIndex);
        return RenderLine(line, monospaceEmSize, wrapInMspace);
    }

    /// <summary>
    /// Render a HighlightedLine with syntax highlighting as rich text
    /// </summary>
    public string RenderLine(HighlightedLine line, string monospaceEmSize = "0.6em", bool wrapInMspace = true)
    {
        var sb = new StringBuilder();

        if (wrapInMspace)
        {
            sb.Append($"<mspace={monospaceEmSize}>");
        }

        for (int i = 0; i < line.Tokens.Count; i++)
        {
            var token = line.Tokens[i];
            var text = token.GetText(line.Content);

            // Escape special characters for noparse
            var escapedText = EscapeText(text);

            // Build opening tags
            var openTags = BuildOpeningTags(token);
            var closeTags = BuildClosingTags(token);

            sb.Append(openTags);
            sb.Append("<noparse>");
            sb.Append(escapedText);
            sb.Append("</noparse>");
            sb.Append(closeTags);
        }

        if (wrapInMspace)
        {
            sb.Append("</mspace>");
        }

        return sb.ToString();
    }

    /// <summary>
    /// Render multiple lines with syntax highlighting
    /// </summary>
    public string RenderLines(int startIndex, int endIndex, string monospaceEmSize = "0.6em")
    {
        var lines = _highlighter.GetHighlightedRange(startIndex, endIndex);
        var sb = new StringBuilder();

        foreach (var line in lines)
        {
            sb.Append("<line>");
            sb.Append(RenderLine(line, monospaceEmSize, wrapInMspace: true));
            sb.Append("</line>");

            if (line.LineIndex < endIndex)
            {
                sb.AppendLine();
            }
        }

        return sb.ToString();
    }

    /// <summary>
    /// Update the syntax highlighter with new code
    /// </summary>
    public void SetDocument(IEnumerable<string> lines)
    {
        _highlighter.SetDocument(lines);
    }

    /// <summary>
    /// Edit a line and get the updated rich text
    /// </summary>
    public string EditLineAndRender(int lineIndex, string newContent, string monospaceEmSize = "0.6em")
    {
        _highlighter.EditLine(lineIndex, newContent);
        return RenderLine(lineIndex, monospaceEmSize);
    }

    /// <summary>
    /// Switch theme and re-render
    /// </summary>
    public void SetTheme(Theme newTheme)
    {
        _highlighter.SetTheme(newTheme);
    }

    public void Dispose()
    {
        _highlighter?.Dispose();
    }

    // ============================================================================
    // Private Helpers
    // ============================================================================

    private string BuildOpeningTags(HighlightedToken token)
    {
        var sb = new StringBuilder();

        // Color tag (always add if color is specified)
        if (!string.IsNullOrEmpty(token.ForegroundColor))
        {
            sb.Append($"<color={token.ForegroundColor}>");
        }

        // Font style tags
        if ((token.FontStyle & FONT_STYLE_BOLD) != 0)
        {
            sb.Append("<b>");
        }

        if ((token.FontStyle & FONT_STYLE_ITALIC) != 0)
        {
            sb.Append("<i>");
        }

        if ((token.FontStyle & FONT_STYLE_UNDERLINE) != 0)
        {
            sb.Append("<u>");
        }

        if ((token.FontStyle & FONT_STYLE_STRIKETHROUGH) != 0)
        {
            sb.Append("<s>");
        }

        return sb.ToString();
    }

    private string BuildClosingTags(HighlightedToken token)
    {
        var sb = new StringBuilder();

        // Close tags in reverse order
        if ((token.FontStyle & FONT_STYLE_STRIKETHROUGH) != 0)
        {
            sb.Append("</s>");
        }

        if ((token.FontStyle & FONT_STYLE_UNDERLINE) != 0)
        {
            sb.Append("</u>");
        }

        if ((token.FontStyle & FONT_STYLE_ITALIC) != 0)
        {
            sb.Append("</i>");
        }

        if ((token.FontStyle & FONT_STYLE_BOLD) != 0)
        {
            sb.Append("</b>");
        }

        // Color tag
        if (!string.IsNullOrEmpty(token.ForegroundColor))
        {
            sb.Append("</color>");
        }

        return sb.ToString();
    }

    private string EscapeText(string text)
    {
        if (string.IsNullOrEmpty(text))
            return text;

        // Escape special XML characters
        return text
            .Replace("&", "&amp;")
            .Replace("<", "&lt;")
            .Replace(">", "&gt;")
            .Replace("\"", "&quot;")
            .Replace("'", "&apos;");
    }
}

/// <summary>
/// Unity UI Toolkit integration example
/// </summary>
public class UnityCodeEditorHighlighter
{
    private SyntaxHighlighterRichTextRenderer _renderer;
    private Grammar _grammar;
    private Theme _theme;

    public UnityCodeEditorHighlighter(Grammar grammar, Theme theme)
    {
        _grammar = grammar;
        _theme = theme;
        _renderer = new SyntaxHighlighterRichTextRenderer(grammar, theme);
    }

    /// <summary>
    /// Highlight code for display in a Label with rich text
    /// </summary>
    public string HighlightCode(string code, string monospaceEmSize = "0.6em")
    {
        var lines = code.Split('\n');
        _renderer.SetDocument(lines);

        return _renderer.RenderLines(0, lines.Length - 1, monospaceEmSize);
    }

    /// <summary>
    /// Highlight a single line
    /// </summary>
    public string HighlightLine(string line, string monospaceEmSize = "0.6em")
    {
        _renderer.SetDocument(new[] { line });
        return _renderer.RenderLine(0, monospaceEmSize, wrapInMspace: true);
    }

    /// <summary>
    /// Handle real-time edits and return updated rich text
    /// </summary>
    public string HandleLineEdit(int lineIndex, string newContent, string monospaceEmSize = "0.6em")
    {
        return _renderer.EditLineAndRender(lineIndex, newContent, monospaceEmSize);
    }

    /// <summary>
    /// Switch theme
    /// </summary>
    public void SetTheme(Theme newTheme)
    {
        _renderer.SetTheme(newTheme);
    }

    public void Dispose()
    {
        _renderer?.Dispose();
    }
}

/// <summary>
/// Example usage in a Unity MonoBehaviour-like context
/// </summary>
public class CodeViewerExample
{
    public static void DemoUnityIntegration()
    {
        Console.WriteLine("=== Unity UI Toolkit Syntax Highlighting Example ===\n");

        try
        {
            // Setup
            using (var tm = new TextMate())
            {
                tm.AddGrammarFromJson(GetMinimalJavaScriptGrammar());
                var grammar = tm.LoadGrammar("source.javascript");
                var theme = new Theme();

                // Create highlighter
                var highlighter = new UnityCodeEditorHighlighter(grammar, theme);

                // Example 1: Simple single line
                Console.WriteLine("Example 1: Single line highlight");
                Console.WriteLine("-".PadRight(70, '-'));
                var singleLine = "const x = 5;";
                var richText = highlighter.HighlightLine(singleLine);
                Console.WriteLine($"Input:  {singleLine}");
                Console.WriteLine($"Output: {richText}\n");

                // Example 2: Multiple lines
                Console.WriteLine("Example 2: Multiple lines");
                Console.WriteLine("-".PadRight(70, '-'));
                var code = @"function fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}";
                var multiLine = highlighter.HighlightCode(code);
                Console.WriteLine($"Input code:\n{code}\n");
                Console.WriteLine($"Rich text output:\n{multiLine}\n");

                // Example 3: Real-time edit
                Console.WriteLine("Example 3: Real-time edit");
                Console.WriteLine("-".PadRight(70, '-'));
                var editorCode = new[] { "const x = 5;", "console.log(x);" };
                highlighter.HighlightCode(string.Join("\n", editorCode));
                var edited = highlighter.HandleLineEdit(0, "const y = 10;");
                Console.WriteLine($"After editing line 0 to 'const y = 10;':");
                Console.WriteLine($"Rich text: {edited}\n");

                // Example 4: Show complex styling
                Console.WriteLine("Example 4: Complex styling (with colors)");
                Console.WriteLine("-".PadRight(70, '-'));
                var complexCode = new[] {
                    "// This is a comment",
                    "const message = \"Hello, World!\";",
                    "function greet(name) {",
                    "    return `Hello, ${name}!`;",
                    "}"
                };
                var complexRich = highlighter.HighlightCode(string.Join("\n", complexCode));
                Console.WriteLine($"Input:\n{string.Join("\n", complexCode)}\n");
                Console.WriteLine($"Output (rich text):\n{complexRich}\n");

                Console.WriteLine("✓ Examples completed!");
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"✗ Error: {ex.Message}");
            Console.Error.WriteLine(ex.StackTrace);
        }
    }

    static string GetMinimalJavaScriptGrammar()
    {
        return @"{
            ""name"": ""JavaScript"",
            ""scopeName"": ""source.javascript"",
            ""patterns"": [
                { ""name"": ""comment.line.double-slash.js"", ""match"": ""//.*$"" },
                { ""name"": ""string.quoted.double.js"", ""match"": ""\\""[^\\""]*\\""\" },
                { ""name"": ""string.template.js"", ""match"": ""\\`[^\\`]*\\`\"" },
                { ""name"": ""keyword.control.js"", ""match"": ""\b(function|const|let|var|return|if|else|for|while)\b"" }
            ]
        }";
    }
}

/// <summary>
/// Advanced: Custom renderer with theme color mapping
/// </summary>
public class UnityRichTextRendererAdvanced : SyntaxHighlighterRichTextRenderer
{
    private Dictionary<string, string> _colorMap;

    public UnityRichTextRendererAdvanced(Grammar grammar, Theme theme, bool enableCache = true)
        : base(grammar, theme, enableCache)
    {
        _colorMap = new Dictionary<string, string>();
    }

    /// <summary>
    /// Add custom color mappings (useful for Unity theme-specific colors)
    /// </summary>
    public void AddColorMapping(string textMateColor, string unityColor)
    {
        _colorMap[textMateColor] = unityColor;
    }

    /// <summary>
    /// Add multiple color mappings at once
    /// </summary>
    public void AddColorMappings(Dictionary<string, string> colorMappings)
    {
        foreach (var kvp in colorMappings)
        {
            _colorMap[kvp.Key] = kvp.Value;
        }
    }

    /// <summary>
    /// Map TextMate colors to Unity colors
    /// Example: #FF0000 -> #FF0000FF (with alpha)
    /// </summary>
    public string MapColor(string textMateColor)
    {
        if (string.IsNullOrEmpty(textMateColor))
            return textMateColor;

        if (_colorMap.TryGetValue(textMateColor, out var mappedColor))
        {
            return mappedColor;
        }

        // If no mapping, ensure color has alpha channel for Unity
        if (textMateColor.Length == 7) // #RRGGBB
        {
            return textMateColor + "FF"; // Add full alpha
        }

        return textMateColor;
    }
}

// ============================================================================
// Main Entry Point
// ============================================================================

class Program
{
    static void Main()
    {
        CodeViewerExample.DemoUnityIntegration();
    }
}
