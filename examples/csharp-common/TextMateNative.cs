using System;
using System.Runtime.InteropServices;

namespace TextMateSharp;

/// <summary>
/// PInvoke declarations for the TextMate C++ library
/// </summary>
public static class TextMateNative
{
    // Library name (cross-platform)
    private const string LibraryName = "vscode-textmate-cpp";

    // Opaque handle types
    public struct TextMateRegistry { public IntPtr Handle; }
    public struct TextMateGrammar { public IntPtr Handle; }
    public struct TextMateStateStack { public IntPtr Handle; }
    public struct TextMateOnigLib { public IntPtr Handle; }

    // Token structure
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateToken
    {
        public int StartIndex;
        public int EndIndex;
        public int ScopeDepth;
        public IntPtr Scopes; // char**
    }

    // Tokenize result structure
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateTokenizeResult
    {
        public IntPtr Tokens; // TextMateToken*
        public int TokenCount;
        public TextMateStateStack RuleStack;
        public int StoppedEarly;
    }

    // Tokenize result2 structure (encoded tokens)
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateTokenizeResult2
    {
        public IntPtr Tokens; // uint32_t*
        public int TokenCount;
        public TextMateStateStack RuleStack;
        public int StoppedEarly;
    }

    // Initialize Oniguruma library
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateOnigLib textmate_oniglib_create();

    // Create registry with Oniguruma library
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateRegistry textmate_registry_create(TextMateOnigLib onigLib);

    // Dispose registry
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_registry_dispose(TextMateRegistry registry);

    // Add grammar to registry from JSON file (returns 1 on success, 0 on failure)
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_registry_add_grammar_from_file(
        TextMateRegistry registry,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string grammarPath
    );

    // Add grammar to registry from JSON string (returns 1 on success, 0 on failure)
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_registry_add_grammar_from_json(
        TextMateRegistry registry,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string jsonContent
    );

    // Set grammar injections for a scope (call before loading the grammar)
    // Note: LPStr uses UTF-8 on Unix/macOS, ANSI on Windows
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_registry_set_injections(
        TextMateRegistry registry,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string scopeName,
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] injections,
        int injectionCount
    );

    // Load grammar by scope name (after grammars have been added to registry)
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateGrammar textmate_registry_load_grammar(
        TextMateRegistry registry,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string scopeName
    );

    // Get INITIAL state
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateStateStack textmate_get_initial_state();

    // Tokenize a line of text
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_tokenize_line(
        TextMateGrammar grammar,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string lineText,
        TextMateStateStack prevState
    );

    // Tokenize a line of text with encoded tokens
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_tokenize_line2(
        TextMateGrammar grammar,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string lineText,
        TextMateStateStack prevState
    );

    // Free tokenize result
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_free_tokenize_result(IntPtr result);

    // Free tokenize result2
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_free_tokenize_result2(IntPtr result);

    // Batch tokenize result structure (Phase 2 optimization)
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateTokenizeMultiLinesResult
    {
        public IntPtr LineResults; // TextMateTokenizeResult**
        public int LineCount;
    }

    // Batch tokenize multiple lines (Phase 2 optimization)
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_tokenize_lines(
        TextMateGrammar grammar,
        IntPtr[] lines,  // const char**
        int lineCount,
        TextMateStateStack initialState
    );

    // Free batch tokenize result
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_free_tokenize_lines_result(IntPtr result);

    // Get scope name from grammar
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_grammar_get_scope_name(TextMateGrammar grammar);

    // Dispose grammar
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_grammar_dispose(TextMateGrammar grammar);

    // Dispose Oniguruma library
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_oniglib_dispose(TextMateOnigLib onigLib);

    // ========================================================================
    // Session API - High-Level Stateful Tokenization
    // ========================================================================

    // Opaque session handle
    public struct TextMateSession { public ulong Handle; }

    // Represents a cached line with tokens and state
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateSessionLine
    {
        public IntPtr Tokens;           // TextMateToken*
        public int TokenCount;          // int32_t
        public TextMateStateStack State; // State at end of line
        public ulong Version;           // uint64_t - version for change tracking
    }

    // Result structure for batch queries
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateSessionLinesResult
    {
        public IntPtr Lines;            // TextMateSessionLine*
        public int LineCount;           // int32_t
    }

    // Session metadata
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateSessionMetadata
    {
        public ulong CreatedAtMs;       // uint64_t
        public uint ReferenceCount;     // uint32_t
        public int LineCount;           // int32_t
        public int CachedLineCount;     // int32_t
        public ulong MemoryUsageBytes;  // uint64_t
    }

    // ========================================================================
    // Session Lifecycle
    // ========================================================================

    // Create a new session for incremental tokenization
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateSession textmate_session_create(TextMateGrammar grammar);

    // Increment reference count for a session
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_session_retain(TextMateSession session);

    // Decrement reference count for a session
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_session_release(TextMateSession session);

    // Dispose a session explicitly
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_session_dispose(TextMateSession session);

    // ========================================================================
    // Session State Management
    // ========================================================================

    // Set the complete document lines (initializes session)
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_session_set_lines(
        TextMateSession session,
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPUTF8Str)] string[] lines,
        int lineCount
    );

    // Get current number of lines in session
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_session_get_line_count(TextMateSession session);

    // ========================================================================
    // Incremental Tokenization Operations
    // ========================================================================

    // Edit (replace) a range of lines and retokenize
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_session_edit(
        TextMateSession session,
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPUTF8Str)] string[] lines,
        int lineCount,
        int startIndex,
        int replaceCount
    );

    // Add (insert) new lines at specified position
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_session_add(
        TextMateSession session,
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPUTF8Str)] string[] lines,
        int lineCount,
        int insertIndex
    );

    // Remove (delete) a range of lines
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_session_remove(
        TextMateSession session,
        int startIndex,
        int removeCount
    );

    // ========================================================================
    // Query Operations
    // ========================================================================

    // Get cached tokens for a single line
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_session_get_line_tokens(
        TextMateSession session,
        int lineIndex
    );

    // Get cached state for a single line (state at end of line)
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateStateStack textmate_session_get_line_state(
        TextMateSession session,
        int lineIndex
    );

    // Get tokens for a range of lines in a single call
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_session_get_tokens_range(
        TextMateSession session,
        int startIndex,
        int endIndex
    );

    // Free query result
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_session_free_tokens_result(IntPtr result);

    // Free batch query result
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_session_free_lines_result(IntPtr result);

    // ========================================================================
    // Maintenance Operations
    // ========================================================================

    // Invalidate cached tokens for a range of lines
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_session_invalidate_range(
        TextMateSession session,
        int startIndex,
        int endIndex
    );

    // Clear entire session cache (but keep lines)
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_session_clear_cache(TextMateSession session);

    // Cleanup expired sessions (automatic memory management)
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_session_cleanup_expired(int maxAgeMs);

    // Get session metadata
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateSessionMetadata textmate_session_get_metadata(
        TextMateSession session
    );

    // ============================================================================
    // SyntaxHighlighter API
    // ============================================================================

    // Opaque handle types
    public struct TextMateSyntaxHighlighter { public IntPtr Handle; }
    public struct TextMateHighlightedLine { public IntPtr Handle; }
    public struct TextMateHighlightedToken { public IntPtr Handle; }

    // HighlightedToken C structure
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateHighlightedTokenC
    {
        public int StartIndex;
        public int EndIndex;
        public IntPtr Scopes; // const char**
        public int ScopeCount;
        public IntPtr ForegroundColor; // const char*
        public IntPtr BackgroundColor; // const char*
        public int FontStyle;
        public int TokenType;
        public IntPtr DebugInfo; // const char*
    }

    // HighlightedLine C structure
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateHighlightedLineC
    {
        public int LineIndex;
        public IntPtr Content; // const char*
        public IntPtr Tokens; // const TextMateHighlightedTokenC*
        public int TokenCount;
        public int IsComplete;
        public ulong Version;
    }

    // Metadata C structure
    [StructLayout(LayoutKind.Sequential)]
    public struct TextMateSyntaxHighlightingMetadataC
    {
        public ulong SessionId;
        public int LineCount;
        public int CachedLineCount;
        public double AverageLineTokenizationMs;
        public long LastUpdateMs;
        public IntPtr ThemeName; // const char*
        public int ThemeColorCount;
    }

    // Create syntax highlighter
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateSyntaxHighlighter textmate_syntax_highlighter_create(
        TextMateGrammar grammar,
        IntPtr theme // TextMateTheme
    );

    // Create syntax highlighter with cache option
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateSyntaxHighlighter textmate_syntax_highlighter_create_with_cache(
        TextMateGrammar grammar,
        IntPtr theme,
        int enableCache
    );

    // Dispose syntax highlighter
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_syntax_highlighter_dispose(
        TextMateSyntaxHighlighter highlighter
    );

    // Set document
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_syntax_highlighter_set_document(
        TextMateSyntaxHighlighter highlighter,
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] lines,
        int lineCount
    );

    // Edit line
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_syntax_highlighter_edit_line(
        TextMateSyntaxHighlighter highlighter,
        int lineIndex,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string newContent
    );

    // Insert lines
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_syntax_highlighter_insert_lines(
        TextMateSyntaxHighlighter highlighter,
        int startIndex,
        [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)] string[] lines,
        int lineCount
    );

    // Remove lines
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_syntax_highlighter_remove_lines(
        TextMateSyntaxHighlighter highlighter,
        int startIndex,
        int count
    );

    // Get line count
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_syntax_highlighter_get_line_count(
        TextMateSyntaxHighlighter highlighter
    );

    // Get highlighted line
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateHighlightedLine textmate_syntax_highlighter_get_highlighted_line(
        TextMateSyntaxHighlighter highlighter,
        int lineIndex
    );

    // Set theme
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_syntax_highlighter_set_theme(
        TextMateSyntaxHighlighter highlighter,
        IntPtr theme
    );

    // Clear cache
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_syntax_highlighter_clear_cache(
        TextMateSyntaxHighlighter highlighter
    );

    // Invalidate cache range
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_syntax_highlighter_invalidate_cache_range(
        TextMateSyntaxHighlighter highlighter,
        int startIndex,
        int endIndex
    );

    // Get metadata
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TextMateSyntaxHighlightingMetadataC textmate_syntax_highlighter_get_metadata(
        TextMateSyntaxHighlighter highlighter
    );

    // HighlightedLine accessors
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_highlighted_line_get_index(TextMateHighlightedLine line);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_highlighted_line_get_content(TextMateHighlightedLine line);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_highlighted_line_get_token_count(TextMateHighlightedLine line);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_highlighted_line_get_token(
        TextMateHighlightedLine line,
        int tokenIndex
    );

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_highlighted_line_is_complete(TextMateHighlightedLine line);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_highlighted_line_dispose(TextMateHighlightedLine line);

    // HighlightedToken accessors
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_highlighted_token_get_start_index(TextMateHighlightedToken token);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_highlighted_token_get_end_index(TextMateHighlightedToken token);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_highlighted_token_get_foreground_color(TextMateHighlightedToken token);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_highlighted_token_get_background_color(TextMateHighlightedToken token);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int textmate_highlighted_token_get_font_style(TextMateHighlightedToken token);

    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_highlighted_token_dispose(TextMateHighlightedToken token);
}
