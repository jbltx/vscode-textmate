#include <gtest/gtest.h>
#include <iostream>
#include <chrono>
#include "../src/c_api.h"

class ThemeFileTest : public ::testing::Test {
};

TEST_F(ThemeFileTest, LoadDarkPlusFile) {
    std::cout << "\n=== Test: LoadDarkPlusFile ===" << std::endl;

    const char* themePath = "test-cases/themes/dark_plus.json";

    std::cout << "Loading theme from: " << themePath << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    TextMateTheme theme = textmate_theme_load_from_file(themePath);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "File loading + theme creation took: " << duration.count() << " ms" << std::endl;

    if (theme != nullptr) {
        std::cout << "✓ Theme loaded successfully" << std::endl;

        uint32_t fg = textmate_theme_get_default_foreground(theme);
        std::cout << "Default foreground: 0x" << std::hex << fg << std::dec << std::endl;

        textmate_theme_dispose(theme);
    } else {
        std::cout << "✗ Failed to load theme" << std::endl;
    }

    ASSERT_NE(theme, nullptr);
}
