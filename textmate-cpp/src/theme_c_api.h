#ifndef VSCODE_TEXTMATE_THEME_C_API_H
#define VSCODE_TEXTMATE_THEME_C_API_H

#include "theme.h"
#include <cstdint>
#include <string>

namespace vscode_textmate {

/**
 * Helper class to manage theme resources and provide C API implementation
 */
class ManagedTheme {
public:
    Theme* theme;
    StyleAttributes* defaults;

    ManagedTheme(Theme* theme_, StyleAttributes* defaults_)
        : theme(theme_), defaults(defaults_) {}

    ~ManagedTheme() {
        if (theme) {
            delete theme;
        }
        if (defaults) {
            delete defaults;
        }
    }
};

// Note: hexColorToUint32 and parseJsonTheme are defined as static in c_api.cpp


} // namespace vscode_textmate

#endif // VSCODE_TEXTMATE_THEME_C_API_H
