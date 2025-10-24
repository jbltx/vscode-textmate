#ifndef VSCODE_TEXTMATE_TOKENIZE_STRING_H
#define VSCODE_TEXTMATE_TOKENIZE_STRING_H

#include "types.h"
#include "onigLib.h"

namespace vscode_textmate {

// Forward declarations
class Grammar;
class StateStackImpl;
class LineTokens;

// StackElement result structure
struct StackElement {
    StateStackImpl* stack;
    int linePos;
    int anchorPosition;
    bool stoppedEarly;

    StackElement()
        : stack(nullptr), linePos(0), anchorPosition(0), stoppedEarly(false) {}
};

// Main tokenization function
StackElement tokenizeString(
    Grammar* grammar,
    OnigString* lineText,
    bool isFirstLine,
    int linePos,
    StateStackImpl* stack,
    LineTokens* lineTokens,
    bool checkWhileConditions,
    int timeLimit
);

} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_TOKENIZE_STRING_H
