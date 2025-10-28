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

/// <summary>
/// High-level stateful tokenization session for incremental text editing
///
/// The Session API manages document state internally, handling incremental
/// retokenization with automatic state cascading and early stopping.
///
/// Usage pattern:
/// <code>
/// using (var session = new TextMateSession(grammar)) {
///     session.SetLines(allLines);
///
///     // Later, when user edits line 50
///     session.Edit(new[] { newLine }, 50, 1);
///
///     var tokens = session.GetLineTokens(50);
/// }
/// </code>
/// </summary>
public class TextMateSession : IDisposable
{
    private TextMateNative.TextMateSession _sessionHandle;
    private bool _disposed;
    private static int _operationCount;

    /// <summary>
    /// Create a new session for incremental tokenization
    /// </summary>
    public TextMateSession(Grammar grammar)
    {
        if (grammar == null)
        {
            throw new ArgumentNullException(nameof(grammar));
        }

        // Get the underlying native grammar handle
        // We need to get access to the grammar's handle - for now, we'll create
        // a temporary workaround by storing reference
        _sessionHandle = TextMateNative.textmate_session_create(
            new TextMateNative.TextMateGrammar { Handle = IntPtr.Zero }
        );

        if (_sessionHandle.Handle == 0)
        {
            throw new Exception("Failed to create TextMate session");
        }
    }

    /// <summary>
    /// Initialize session with complete document lines
    /// </summary>
    public void SetLines(string[] lines)
    {
        ThrowIfDisposed();
        if (lines == null) throw new ArgumentNullException(nameof(lines));

        var result = TextMateNative.textmate_session_set_lines(_sessionHandle, lines, lines.Length);
        if (result != 0)
        {
            throw new Exception($"Failed to set lines: error code {result}");
        }
    }

    /// <summary>
    /// Get the current number of lines in the session
    /// </summary>
    public int GetLineCount()
    {
        ThrowIfDisposed();
        return TextMateNative.textmate_session_get_line_count(_sessionHandle);
    }

    /// <summary>
    /// Edit (replace) lines and retokenize incrementally
    ///
    /// Example: User edits line 50
    ///   session.Edit(new[] { newLine }, 50, 1);
    ///
    /// The session automatically:
    /// - Replaces lines[50:51] with the new line
    /// - Retokenizes line 50 + cascades forward
    /// - Stops when state stabilizes (incremental optimization)
    /// </summary>
    public void Edit(string[] lines, int startIndex, int replaceCount)
    {
        ThrowIfDisposed();
        if (lines == null) throw new ArgumentNullException(nameof(lines));
        if (startIndex < 0) throw new ArgumentOutOfRangeException(nameof(startIndex));
        if (replaceCount < 0) throw new ArgumentOutOfRangeException(nameof(replaceCount));

        TriggerPeriodicCleanup();

        var result = TextMateNative.textmate_session_edit(
            _sessionHandle, lines, lines.Length, startIndex, replaceCount
        );
        if (result != 0)
        {
            throw new Exception($"Failed to edit: error code {result}");
        }
    }

    /// <summary>
    /// Add (insert) new lines and retokenize incrementally
    ///
    /// Example: User pastes 5 lines at position 100
    ///   session.Add(pastedLines, 5, 100);
    ///
    /// The session automatically:
    /// - Shifts lines 100+ down by 5 positions
    /// - Inserts new lines at position 100
    /// - Retokenizes and cascades forward
    /// </summary>
    public void Add(string[] lines, int insertIndex)
    {
        ThrowIfDisposed();
        if (lines == null) throw new ArgumentNullException(nameof(lines));
        if (insertIndex < 0) throw new ArgumentOutOfRangeException(nameof(insertIndex));

        TriggerPeriodicCleanup();

        var result = TextMateNative.textmate_session_add(
            _sessionHandle, lines, lines.Length, insertIndex
        );
        if (result != 0)
        {
            throw new Exception($"Failed to add lines: error code {result}");
        }
    }

    /// <summary>
    /// Remove (delete) lines and retokenize incrementally
    ///
    /// Example: User deletes 3 lines starting at line 50
    ///   session.Remove(50, 3);
    ///
    /// The session automatically:
    /// - Removes lines 50-52
    /// - Shifts remaining lines up
    /// - Retokenizes and cascades forward
    /// </summary>
    public void Remove(int startIndex, int removeCount)
    {
        ThrowIfDisposed();
        if (startIndex < 0) throw new ArgumentOutOfRangeException(nameof(startIndex));
        if (removeCount < 0) throw new ArgumentOutOfRangeException(nameof(removeCount));

        TriggerPeriodicCleanup();

        var result = TextMateNative.textmate_session_remove(_sessionHandle, startIndex, removeCount);
        if (result != 0)
        {
            throw new Exception($"Failed to remove lines: error code {result}");
        }
    }

    /// <summary>
    /// Get cached tokens for a single line
    ///
    /// Returns cached tokens without any retokenization.
    /// Fast O(1) operation - useful for rendering.
    /// </summary>
    public TokenizeLineResult? GetLineTokens(int lineIndex)
    {
        ThrowIfDisposed();
        if (lineIndex < 0) throw new ArgumentOutOfRangeException(nameof(lineIndex));

        var resultPtr = TextMateNative.textmate_session_get_line_tokens(_sessionHandle, lineIndex);
        if (resultPtr == IntPtr.Zero)
        {
            return null;
        }

        try
        {
            var nativeResult = Marshal.PtrToStructure<TextMateNative.TextMateTokenizeResult>(resultPtr);
            var tokens = new List<Token>(nativeResult.TokenCount);

            unsafe
            {
                var tokenPtr = (TextMateNative.TextMateToken*)nativeResult.Tokens;
                for (int i = 0; i < nativeResult.TokenCount; i++)
                {
                    var nativeToken = tokenPtr[i];
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

                    tokens.Add(new Token
                    {
                        StartIndex = nativeToken.StartIndex,
                        EndIndex = nativeToken.EndIndex,
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
            TextMateNative.textmate_session_free_tokens_result(resultPtr);
        }
    }

    /// <summary>
    /// Get the state at the end of a line
    /// </summary>
    public StateStack? GetLineState(int lineIndex)
    {
        ThrowIfDisposed();
        if (lineIndex < 0) throw new ArgumentOutOfRangeException(nameof(lineIndex));

        var state = TextMateNative.textmate_session_get_line_state(_sessionHandle, lineIndex);
        if (state.Handle == IntPtr.Zero)
        {
            return null;
        }

        return new StateStack(state);
    }

    /// <summary>
    /// Invalidate cached tokens for a range of lines
    ///
    /// Forces retokenization on next query. Useful when grammar changes
    /// or external state is modified.
    /// </summary>
    public void InvalidateRange(int startIndex, int endIndex)
    {
        ThrowIfDisposed();
        if (startIndex < 0) throw new ArgumentOutOfRangeException(nameof(startIndex));

        TextMateNative.textmate_session_invalidate_range(_sessionHandle, startIndex, endIndex);
    }

    /// <summary>
    /// Clear entire cache but keep document structure
    /// </summary>
    public void ClearCache()
    {
        ThrowIfDisposed();
        TextMateNative.textmate_session_clear_cache(_sessionHandle);
    }

    /// <summary>
    /// Get session metadata for debugging/monitoring
    /// </summary>
    public SessionMetadata GetMetadata()
    {
        ThrowIfDisposed();
        var native = TextMateNative.textmate_session_get_metadata(_sessionHandle);
        return new SessionMetadata
        {
            CreatedAtMs = native.CreatedAtMs,
            ReferenceCount = native.ReferenceCount,
            LineCount = native.LineCount,
            CachedLineCount = native.CachedLineCount,
            MemoryUsageBytes = native.MemoryUsageBytes
        };
    }

    /// <summary>
    /// Periodic cleanup of expired sessions (called automatically)
    /// </summary>
    private static void TriggerPeriodicCleanup()
    {
        if (++_operationCount % 100 == 0)
        {
            TextMateNative.textmate_session_cleanup_expired(60000); // 60 seconds
        }
    }

    private void ThrowIfDisposed()
    {
        if (_disposed)
        {
            throw new ObjectDisposedException(nameof(TextMateSession));
        }
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            if (_sessionHandle.Handle != 0)
            {
                TextMateNative.textmate_session_dispose(_sessionHandle);
            }
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }

    ~TextMateSession()
    {
        if (!_disposed)
        {
            TextMateNative.textmate_session_dispose(_sessionHandle);
        }
    }
}

/// <summary>
/// Session metadata for monitoring and debugging
/// </summary>
public class SessionMetadata
{
    public ulong CreatedAtMs { get; set; }
    public uint ReferenceCount { get; set; }
    public int LineCount { get; set; }
    public int CachedLineCount { get; set; }
    public ulong MemoryUsageBytes { get; set; }
}

/// <summary>
/// Represents a single highlighted token with complete styling information
/// </summary>
public class HighlightedToken
{
    public int StartIndex { get; set; }
    public int EndIndex { get; set; }
    public List<string> Scopes { get; set; } = new();
    public string ForegroundColor { get; set; } = "";
    public string BackgroundColor { get; set; } = "";
    public int FontStyle { get; set; }
    public int TokenType { get; set; }
    public string DebugInfo { get; set; } = "";

    public string GetText(string lineContent)
    {
        if (StartIndex >= 0 && EndIndex <= lineContent.Length)
        {
            return lineContent.Substring(StartIndex, EndIndex - StartIndex);
        }
        return "";
    }
}

/// <summary>
/// Represents a complete highlighted line with all tokens and styling
/// </summary>
public class HighlightedLine
{
    public int LineIndex { get; set; }
    public string Content { get; set; } = "";
    public List<HighlightedToken> Tokens { get; set; } = new();
    public bool IsComplete { get; set; }
    public ulong Version { get; set; }
}

/// <summary>
/// Syntax highlighting engine combining Session API with Theme system
/// Provides automatic styling resolution for complete syntax highlighting
/// </summary>
public class SyntaxHighlighter : IDisposable
{
    private TextMateSyntaxHighlighter _handle;
    private bool _disposed;
    private Theme _theme;

    /// <summary>
    /// Create a new syntax highlighter
    /// </summary>
    public SyntaxHighlighter(Grammar grammar, Theme theme, bool enableCache = true)
    {
        _theme = theme ?? throw new ArgumentNullException(nameof(theme));

        _handle = enableCache
            ? TextMateNative.textmate_syntax_highlighter_create_with_cache(
                grammar._handle, theme._handle, 1)
            : TextMateNative.textmate_syntax_highlighter_create(
                grammar._handle, theme._handle);

        if (_handle.Handle == IntPtr.Zero)
        {
            throw new Exception("Failed to create SyntaxHighlighter");
        }
    }

    /// <summary>
    /// Load a complete document
    /// </summary>
    public void SetDocument(IEnumerable<string> lines)
    {
        var lineArray = lines as string[] ?? lines.ToArray();
        TextMateNative.textmate_syntax_highlighter_set_document(_handle, lineArray, lineArray.Length);
    }

    /// <summary>
    /// Edit a single line
    /// </summary>
    public void EditLine(int lineIndex, string newContent)
    {
        TextMateNative.textmate_syntax_highlighter_edit_line(_handle, lineIndex, newContent);
    }

    /// <summary>
    /// Insert lines at specified position
    /// </summary>
    public void InsertLines(int startIndex, IEnumerable<string> lines)
    {
        var lineArray = lines as string[] ?? lines.ToArray();
        TextMateNative.textmate_syntax_highlighter_insert_lines(_handle, startIndex, lineArray, lineArray.Length);
    }

    /// <summary>
    /// Remove lines
    /// </summary>
    public void RemoveLines(int startIndex, int count)
    {
        TextMateNative.textmate_syntax_highlighter_remove_lines(_handle, startIndex, count);
    }

    /// <summary>
    /// Get current line count
    /// </summary>
    public int GetLineCount()
    {
        return TextMateNative.textmate_syntax_highlighter_get_line_count(_handle);
    }

    /// <summary>
    /// Get syntax-highlighted version of a single line
    /// </summary>
    public HighlightedLine GetHighlightedLine(int lineIndex)
    {
        var handle = TextMateNative.textmate_syntax_highlighter_get_highlighted_line(_handle, lineIndex);
        if (handle.Handle == IntPtr.Zero)
        {
            throw new Exception($"Failed to get highlighted line {lineIndex}");
        }

        try
        {
            var result = new HighlightedLine
            {
                LineIndex = TextMateNative.textmate_highlighted_line_get_index(handle),
                Content = Marshal.PtrToStringUTF8(
                    TextMateNative.textmate_highlighted_line_get_content(handle)) ?? "",
                IsComplete = TextMateNative.textmate_highlighted_line_is_complete(handle) != 0,
            };

            int tokenCount = TextMateNative.textmate_highlighted_line_get_token_count(handle);
            for (int i = 0; i < tokenCount; i++)
            {
                var tokenPtr = TextMateNative.textmate_highlighted_line_get_token(handle, i);
                if (tokenPtr != IntPtr.Zero)
                {
                    var token = new HighlightedToken
                    {
                        StartIndex = TextMateNative.textmate_highlighted_token_get_start_index(
                            new TextMateNative.TextMateHighlightedToken { Handle = tokenPtr }),
                        EndIndex = TextMateNative.textmate_highlighted_token_get_end_index(
                            new TextMateNative.TextMateHighlightedToken { Handle = tokenPtr }),
                        ForegroundColor = Marshal.PtrToStringUTF8(
                            TextMateNative.textmate_highlighted_token_get_foreground_color(
                                new TextMateNative.TextMateHighlightedToken { Handle = tokenPtr })) ?? "",
                        BackgroundColor = Marshal.PtrToStringUTF8(
                            TextMateNative.textmate_highlighted_token_get_background_color(
                                new TextMateNative.TextMateHighlightedToken { Handle = tokenPtr })) ?? "",
                        FontStyle = TextMateNative.textmate_highlighted_token_get_font_style(
                            new TextMateNative.TextMateHighlightedToken { Handle = tokenPtr }),
                    };
                    result.Tokens.Add(token);
                }
            }

            return result;
        }
        finally
        {
            TextMateNative.textmate_highlighted_line_dispose(handle);
        }
    }

    /// <summary>
    /// Get multiple highlighted lines (batch query is more efficient)
    /// </summary>
    public List<HighlightedLine> GetHighlightedRange(int startIndex, int endIndex)
    {
        var results = new List<HighlightedLine>();
        for (int i = startIndex; i <= endIndex; i++)
        {
            results.Add(GetHighlightedLine(i));
        }
        return results;
    }

    /// <summary>
    /// Switch to a different theme
    /// </summary>
    public void SetTheme(Theme newTheme)
    {
        _theme = newTheme ?? throw new ArgumentNullException(nameof(newTheme));
        TextMateNative.textmate_syntax_highlighter_set_theme(_handle, newTheme._handle);
    }

    /// <summary>
    /// Clear all cached highlighting
    /// </summary>
    public void ClearCache()
    {
        TextMateNative.textmate_syntax_highlighter_clear_cache(_handle);
    }

    /// <summary>
    /// Invalidate cache for a range of lines
    /// </summary>
    public void InvalidateCacheRange(int startIndex, int endIndex)
    {
        TextMateNative.textmate_syntax_highlighter_invalidate_cache_range(_handle, startIndex, endIndex);
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            if (_handle.Handle != IntPtr.Zero)
            {
                TextMateNative.textmate_syntax_highlighter_dispose(_handle);
            }
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }

    ~SyntaxHighlighter()
    {
        Dispose();
    }
}
