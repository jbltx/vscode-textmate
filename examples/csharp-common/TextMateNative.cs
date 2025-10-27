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

    // Get scope name from grammar
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr textmate_grammar_get_scope_name(TextMateGrammar grammar);

    // Dispose grammar
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_grammar_dispose(TextMateGrammar grammar);

    // Dispose Oniguruma library
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void textmate_oniglib_dispose(TextMateOnigLib onigLib);
}
