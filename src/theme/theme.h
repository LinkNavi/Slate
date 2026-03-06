#pragma once
#include <cstdint>
#include <string>

struct Color {
    float r, g, b, a = 1.0f;
    uint32_t to_u32() const;
    static Color from_hsv(float h, float s, float v);
    float luminance() const;
    float contrast_ratio(const Color& other) const;
    Color enforce_contrast(const Color& bg, float min_ratio = 4.5f) const;
};

struct SyntaxColors {
    Color keyword;
    Color type;
    Color string;
    Color comment;
    Color number;
    Color function;
    Color variable;
    Color preprocessor;
};

struct Theme {
    Color bg;
    Color bg_panel;
    Color bg_input;
    Color fg;
    Color fg_dim;
    Color accent;
    Color accent2;
    Color error;
    Color warning;
    Color success;
    SyntaxColors syntax;

    static Theme from_wallpaper();
    static Theme from_image_path(const std::string& path);
    static Theme fallback_dark();

    void save(const std::string& path) const;
    static Theme load(const std::string& path);

private:
    static std::string get_wallpaper_path();
};
