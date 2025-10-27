using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using static TextMateSharp.TextMateNative;

namespace TextMateSharp;

/// <summary>
/// Internal helper for scope string interning
/// </summary>
internal static class ScopeCache
{
    // Global scope string interning cache for memory efficiency
    private static readonly System.Collections.Concurrent.ConcurrentDictionary<string, string> _cache = new();

    public static string Intern(string scope)
    {
        return _cache.GetOrAdd(scope, scope);
    }
}

/// <summary>
/// Managed wrapper for TextMate tokenization
/// </summary>
public class TextMate : IDisposable
{
    private TextMateOnigLib _onigLib;
    private TextMateRegistry _registry;
    private bool _disposed;

    public TextMate()
    {
        _onigLib = textmate_oniglib_create();
        if (_onigLib.Handle == IntPtr.Zero)
        {
            throw new Exception("Failed to create Oniguruma library");
        }

        _registry = textmate_registry_create(_onigLib);
        if (_registry.Handle == IntPtr.Zero)
        {
            textmate_oniglib_dispose(_onigLib);
            throw new Exception("Failed to create TextMate registry");
        }
    }

    /// <summary>
    /// Add a grammar from a JSON file to the registry (does not return Grammar)
    /// </summary>
    public void AddGrammarFromFile(string grammarPath)
    {
        var result = textmate_registry_add_grammar_from_file(_registry, grammarPath);
        if (result == 0)
        {
            throw new Exception($"Failed to add grammar from file: {grammarPath}");
        }
    }

    /// <summary>
    /// Add a grammar from JSON content to the registry (does not return Grammar)
    /// </summary>
    public void AddGrammarFromJson(string jsonContent)
    {
        var result = textmate_registry_add_grammar_from_json(_registry, jsonContent);
        if (result == 0)
        {
            throw new Exception("Failed to add grammar from JSON content");
        }
    }

    /// <summary>
    /// Set grammar injections for a scope (call before loading the grammar)
    /// </summary>
    public void SetInjections(string scopeName, string[] injections)
    {
        if (injections != null && injections.Length > 0)
        {
            textmate_registry_set_injections(_registry, scopeName, injections, injections.Length);
        }
    }

    /// <summary>
    /// Load a grammar by scope name (after adding grammars to the registry)
    /// This properly resolves dependencies and includes
    /// </summary>
    public Grammar LoadGrammar(string scopeName)
    {
        var grammarHandle = textmate_registry_load_grammar(_registry, scopeName);
        if (grammarHandle.Handle == IntPtr.Zero)
        {
            throw new Exception($"Failed to load grammar with scope: {scopeName}");
        }
        return new Grammar(grammarHandle);
    }

    /// <summary>
    /// Get the initial state for tokenization
    /// </summary>
    public static StateStack GetInitialState()
    {
        return new StateStack(textmate_get_initial_state());
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            if (_registry.Handle != IntPtr.Zero)
            {
                textmate_registry_dispose(_registry);
            }
            if (_onigLib.Handle != IntPtr.Zero)
            {
                textmate_oniglib_dispose(_onigLib);
            }
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }

    ~TextMate()
    {
        Dispose();
    }
}

/// <summary>
/// Represents a TextMate grammar
/// </summary>
public class Grammar
{
    private readonly TextMateGrammar _handle;

    internal Grammar(TextMateGrammar handle)
    {
        _handle = handle;
    }

    /// <summary>
    /// Tokenize a line of text
    /// </summary>
    public TokenizeLineResult TokenizeLine(string lineText, StateStack prevState)
    {
        var resultPtr = textmate_tokenize_line(_handle, lineText, prevState.Handle);
        if (resultPtr == IntPtr.Zero)
        {
            throw new Exception("Failed to tokenize line");
        }

        try
        {
            var nativeResult = Marshal.PtrToStructure<TextMateTokenizeResult>(resultPtr);
            var tokens = new List<Token>(nativeResult.TokenCount);

            // ASCII fast path: if the line is pure ASCII, UTF-8 byte indices == UTF-16 char indices
            bool isAscii = IsAscii(lineText);

            // Cached position for index conversion (tokens are ordered)
            int lastByteIndex = 0;
            int lastCharIndex = 0;
            int lastStringPos = 0;

            unsafe
            {
                var tokenPtr = (TextMateToken*)nativeResult.Tokens;
                for (int i = 0; i < nativeResult.TokenCount; i++)
                {
                    var nativeToken = tokenPtr[i];

                    // Pre-allocate list with exact capacity to reduce allocations
                    var scopes = new List<string>(nativeToken.ScopeDepth);
                    var scopesPtr = (IntPtr*)nativeToken.Scopes;

                    for (int j = 0; j < nativeToken.ScopeDepth; j++)
                    {
                        var scopeStr = Marshal.PtrToStringUTF8(scopesPtr[j]);
                        if (scopeStr != null)
                        {
                            // Intern scope strings to reduce memory allocations
                            scopes.Add(ScopeCache.Intern(scopeStr));
                        }
                    }

                    // Convert UTF-8 byte indices to UTF-16 char indices
                    int startCharIndex, endCharIndex;
                    if (isAscii)
                    {
                        // Fast path: no conversion needed (clamp to string length for safety)
                        startCharIndex = Math.Min(nativeToken.StartIndex, lineText.Length);
                        endCharIndex = Math.Min(nativeToken.EndIndex, lineText.Length);
                    }
                    else
                    {
                        // Use cached position for faster conversion
                        startCharIndex = Utf8ByteIndexToCharIndexCached(lineText, nativeToken.StartIndex,
                            ref lastByteIndex, ref lastCharIndex, ref lastStringPos);
                        endCharIndex = Utf8ByteIndexToCharIndexCached(lineText, nativeToken.EndIndex,
                            ref lastByteIndex, ref lastCharIndex, ref lastStringPos);
                    }

                    tokens.Add(new Token
                    {
                        StartIndex = startCharIndex,
                        EndIndex = endCharIndex,
                        Scopes = scopes
                    });
                }
            }

            return new TokenizeLineResult
            {
                Tokens = tokens,
                RuleStack = new StateStack(nativeResult.RuleStack),
                StoppedEarly = nativeResult.StoppedEarly != 0
            };
        }
        finally
        {
            textmate_free_tokenize_result(resultPtr);
        }
    }

    /// <summary>
    /// Check if a string contains only ASCII characters (fast path detection)
    /// Optimized with ReadOnlySpan for better performance
    /// </summary>
    private static bool IsAscii(string str)
    {
        ReadOnlySpan<char> span = str.AsSpan();
        for (int i = 0; i < span.Length; i++)
        {
            if (span[i] > 0x7F) return false;
        }
        return true;
    }

    /// <summary>
    /// Convert UTF-8 byte index to UTF-16 char index with cached position (exploits token ordering)
    /// </summary>
    private static int Utf8ByteIndexToCharIndexCached(string str, int targetByteIndex,
        ref int lastByteIndex, ref int lastCharIndex, ref int lastStringPos)
    {
        if (targetByteIndex == 0)
        {
            lastByteIndex = 0;
            lastCharIndex = 0;
            lastStringPos = 0;
            return 0;
        }

        // Start from cached position if target is ahead
        int charIndex = targetByteIndex >= lastByteIndex ? lastCharIndex : 0;
        int currentByteIndex = targetByteIndex >= lastByteIndex ? lastByteIndex : 0;
        int startPos = targetByteIndex >= lastByteIndex ? lastStringPos : 0;

        for (int i = startPos; i < str.Length; i++)
        {
            if (currentByteIndex >= targetByteIndex)
            {
                // Cache position for next call
                lastByteIndex = currentByteIndex;
                lastCharIndex = charIndex;
                lastStringPos = i;
                return charIndex;
            }

            char c = str[i];

            // Calculate UTF-8 byte length for this character
            if (c < 0x80)
            {
                currentByteIndex += 1; // ASCII
                charIndex++;
            }
            else if (c < 0x800)
            {
                currentByteIndex += 2; // 2-byte UTF-8
                charIndex++;
            }
            else if (c >= 0xD800 && c <= 0xDBFF)
            {
                // High surrogate - this is part of a 4-byte UTF-8 sequence
                currentByteIndex += 4;
                charIndex += 2; // Count both high and low surrogate
                i++; // Skip the low surrogate in next iteration
            }
            else if (c >= 0xDC00 && c <= 0xDFFF)
            {
                // Low surrogate appearing alone (shouldn't happen in valid strings)
                currentByteIndex += 3;
                charIndex++;
            }
            else
            {
                currentByteIndex += 3; // 3-byte UTF-8
                charIndex++;
            }
        }

        // Cache final position
        lastByteIndex = currentByteIndex;
        lastCharIndex = charIndex;
        lastStringPos = str.Length;
        return charIndex;
    }

    /// <summary>
    /// Tokenize a line of text with encoded tokens (more efficient)
    /// </summary>
    public TokenizeLineResult2 TokenizeLine2(string lineText, StateStack prevState)
    {
        var resultPtr = textmate_tokenize_line2(_handle, lineText, prevState.Handle);
        if (resultPtr == IntPtr.Zero)
        {
            throw new Exception("Failed to tokenize line");
        }

        try
        {
            var nativeResult = Marshal.PtrToStructure<TextMateTokenizeResult2>(resultPtr);
            var tokens = new uint[nativeResult.TokenCount];

            Marshal.Copy(nativeResult.Tokens, (int[])(object)tokens, 0, nativeResult.TokenCount);

            return new TokenizeLineResult2
            {
                Tokens = tokens,
                RuleStack = new StateStack(nativeResult.RuleStack),
                StoppedEarly = nativeResult.StoppedEarly != 0
            };
        }
        finally
        {
            textmate_free_tokenize_result2(resultPtr);
        }
    }

    /// <summary>
    /// Batch tokenize multiple lines in a single native call (Phase 2 optimization)
    /// This dramatically reduces PInvoke overhead by processing all lines at once
    /// </summary>
    public List<TokenizeLineResult> TokenizeLines(string[] lines, StateStack prevState)
    {
        // Convert strings to UTF-8 and pin them
        var utf8Lines = new IntPtr[lines.Length];
        var handles = new System.Runtime.InteropServices.GCHandle[lines.Length];

        try
        {
            // Pin all strings
            for (int i = 0; i < lines.Length; i++)
            {
                var utf8Bytes = System.Text.Encoding.UTF8.GetBytes(lines[i] + "\0");
                handles[i] = System.Runtime.InteropServices.GCHandle.Alloc(utf8Bytes, System.Runtime.InteropServices.GCHandleType.Pinned);
                utf8Lines[i] = handles[i].AddrOfPinnedObject();
            }

            // Call native batch function
            var resultPtr = textmate_tokenize_lines(_handle, utf8Lines, lines.Length, prevState.Handle);
            if (resultPtr == IntPtr.Zero)
            {
                throw new Exception("Failed to tokenize lines");
            }

            try
            {
                var batchResult = Marshal.PtrToStructure<TextMateTokenizeMultiLinesResult>(resultPtr);
                var results = new List<TokenizeLineResult>(batchResult.LineCount);

                unsafe
                {
                    var lineResultsPtr = (IntPtr*)batchResult.LineResults;

                    for (int lineIdx = 0; lineIdx < batchResult.LineCount; lineIdx++)
                    {
                        var lineResultPtr = lineResultsPtr[lineIdx];
                        var nativeResult = Marshal.PtrToStructure<TextMateTokenizeResult>(lineResultPtr);
                        var tokens = new List<Token>(nativeResult.TokenCount);

                        // ASCII fast path
                        bool isAscii = IsAscii(lines[lineIdx]);

                        // Cached position for index conversion
                        int lastByteIndex = 0;
                        int lastCharIndex = 0;
                        int lastStringPos = 0;

                        var tokenPtr = (TextMateToken*)nativeResult.Tokens;
                        for (int i = 0; i < nativeResult.TokenCount; i++)
                        {
                            var nativeToken = tokenPtr[i];

                            // Pre-allocate list with exact capacity
                            var scopes = new List<string>(nativeToken.ScopeDepth);
                            var scopesPtr = (IntPtr*)nativeToken.Scopes;

                            for (int j = 0; j < nativeToken.ScopeDepth; j++)
                            {
                                var scopeStr = Marshal.PtrToStringUTF8(scopesPtr[j]);
                                if (scopeStr != null)
                                {
                                    scopes.Add(ScopeCache.Intern(scopeStr));
                                }
                            }

                            // Convert UTF-8 byte indices to UTF-16 char indices
                            int startCharIndex, endCharIndex;
                            if (isAscii)
                            {
                                startCharIndex = Math.Min(nativeToken.StartIndex, lines[lineIdx].Length);
                                endCharIndex = Math.Min(nativeToken.EndIndex, lines[lineIdx].Length);
                            }
                            else
                            {
                                startCharIndex = Utf8ByteIndexToCharIndexCached(lines[lineIdx], nativeToken.StartIndex,
                                    ref lastByteIndex, ref lastCharIndex, ref lastStringPos);
                                endCharIndex = Utf8ByteIndexToCharIndexCached(lines[lineIdx], nativeToken.EndIndex,
                                    ref lastByteIndex, ref lastCharIndex, ref lastStringPos);
                            }

                            tokens.Add(new Token
                            {
                                StartIndex = startCharIndex,
                                EndIndex = endCharIndex,
                                Scopes = scopes
                            });
                        }

                        results.Add(new TokenizeLineResult
                        {
                            Tokens = tokens,
                            RuleStack = new StateStack(nativeResult.RuleStack),
                            StoppedEarly = nativeResult.StoppedEarly != 0
                        });
                    }
                }

                return results;
            }
            finally
            {
                textmate_free_tokenize_lines_result(resultPtr);
            }
        }
        finally
        {
            // Unpin all strings
            for (int i = 0; i < handles.Length; i++)
            {
                if (handles[i].IsAllocated)
                {
                    handles[i].Free();
                }
            }
        }
    }
}

/// <summary>
/// Represents a tokenization state stack (opaque handle)
/// </summary>
public class StateStack
{
    internal TextMateStateStack Handle { get; }

    internal StateStack(TextMateStateStack handle)
    {
        Handle = handle;
    }
}

/// <summary>
/// Represents a token with scopes
/// </summary>
public class Token
{
    public int StartIndex { get; init; }
    public int EndIndex { get; init; }
    public List<string> Scopes { get; init; } = new();

    /// <summary>
    /// Get the token value from the line text
    /// </summary>
    public string GetValue(string lineText)
    {
        return lineText.Substring(StartIndex, EndIndex - StartIndex);
    }
}

/// <summary>
/// Result of tokenizing a line
/// </summary>
public class TokenizeLineResult
{
    public List<Token> Tokens { get; init; } = new();
    public StateStack? RuleStack { get; init; }
    public bool StoppedEarly { get; init; }
}

/// <summary>
/// Result of tokenizing a line with encoded tokens
/// </summary>
public class TokenizeLineResult2
{
    public uint[] Tokens { get; init; } = Array.Empty<uint>();
    public StateStack? RuleStack { get; init; }
    public bool StoppedEarly { get; init; }
}
