#include "theme.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <cstdlib>
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../stb/stb_image.h"
#include "../stb/stb_image_resize.h"

// ---- Color math ----

uint32_t Color::to_u32() const {
    auto c = [](float v){ return (uint8_t)(v * 255.f); };
    return (c(a) << 24) | (c(b) << 16) | (c(g) << 8) | c(r);
}

float Color::luminance() const {
    auto lin = [](float c) {
        return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
    };
    return 0.2126f * lin(r) + 0.7152f * lin(g) + 0.0722f * lin(b);
}

float Color::contrast_ratio(const Color& other) const {
    float l1 = luminance(), l2 = other.luminance();
    if (l1 < l2) std::swap(l1, l2);
    return (l1 + 0.05f) / (l2 + 0.05f);
}

Color Color::enforce_contrast(const Color& bg, float min_ratio) const {
    if (contrast_ratio(bg) >= min_ratio) return *this;
    // Shift lightness until contrast is met
    Color c = *this;
    float step = bg.luminance() > 0.5f ? -0.05f : 0.05f;
    for (int i = 0; i < 20; ++i) {
        c.r = std::clamp(c.r + step, 0.f, 1.f);
        c.g = std::clamp(c.g + step, 0.f, 1.f);
        c.b = std::clamp(c.b + step, 0.f, 1.f);
        if (c.contrast_ratio(bg) >= min_ratio) break;
    }
    return c;
}

Color Color::from_hsv(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.f - fabsf(fmodf(h / 60.f, 2.f) - 1.f));
    float m = v - c;
    float r, g, b;
    if      (h < 60)  { r=c; g=x; b=0; }
    else if (h < 120) { r=x; g=c; b=0; }
    else if (h < 180) { r=0; g=c; b=x; }
    else if (h < 240) { r=0; g=x; b=c; }
    else if (h < 300) { r=x; g=0; b=c; }
    else              { r=c; g=0; b=x; }
    return {r+m, g+m, b+m, 1.f};
}

// ---- K-means ----

static float color_dist(const Color& a, const Color& b) {
    float dr = a.r-b.r, dg = a.g-b.g, db = a.b-b.b;
    return dr*dr + dg*dg + db*db;
}

static std::vector<Color> kmeans(const std::vector<Color>& pixels, int k, int iters = 20) {
    std::mt19937 rng(42);
    std::vector<Color> centers(k);
    std::vector<int> assign(pixels.size());

    // Init: pick random pixels as starting centers
    for (int i = 0; i < k; ++i)
        centers[i] = pixels[rng() % pixels.size()];

    for (int iter = 0; iter < iters; ++iter) {
        // Assign
        for (size_t i = 0; i < pixels.size(); ++i) {
            float best = 1e9f; int bi = 0;
            for (int j = 0; j < k; ++j) {
                float d = color_dist(pixels[i], centers[j]);
                if (d < best) { best = d; bi = j; }
            }
            assign[i] = bi;
        }
        // Recompute centers
        std::vector<Color> sums(k, {0,0,0,0});
        std::vector<int> counts(k, 0);
        for (size_t i = 0; i < pixels.size(); ++i) {
            sums[assign[i]].r += pixels[i].r;
            sums[assign[i]].g += pixels[i].g;
            sums[assign[i]].b += pixels[i].b;
            counts[assign[i]]++;
        }
        for (int j = 0; j < k; ++j) {
            if (counts[j] > 0) {
                centers[j].r = sums[j].r / counts[j];
                centers[j].g = sums[j].g / counts[j];
                centers[j].b = sums[j].b / counts[j];
            }
        }
    }
    return centers;
}

// ---- Wallpaper path ----

std::string Theme::get_wallpaper_path() {
#if defined(_WIN32)
    char buf[MAX_PATH] = {};
    SystemParametersInfoA(SPI_GETDESKWALLPAPER, MAX_PATH, buf, 0);
    return std::string(buf);
#else
    // Try GNOME
    FILE* f = popen("gsettings get org.gnome.desktop.background picture-uri 2>/dev/null", "r");
    if (f) {
        char buf[1024] = {};
        fgets(buf, sizeof(buf), f);
        pclose(f);
        std::string s(buf);
        // Strip 'file://' and quotes
        auto pos = s.find("file://");
        if (pos != std::string::npos) {
            s = s.substr(pos + 7);
            s.erase(std::remove(s.begin(), s.end(), '\''), s.end());
            s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
            return s;
        }
    }
    return "";
#endif
}

// ---- Build theme from palette ----

static Theme build_theme(std::vector<Color> palette) {
    // Sort by luminance
    std::sort(palette.begin(), palette.end(), [](const Color& a, const Color& b){
        return a.luminance() < b.luminance();
    });

    Color bg      = palette.front();
    Color fg      = palette.back();
    bool dark_bg  = bg.luminance() < 0.3f;

    // Find most vivid color for accent
    Color accent = palette[palette.size()/2];
    float best_sat = 0;
    for (auto& c : palette) {
        float mx = std::max({c.r, c.g, c.b});
        float mn = std::min({c.r, c.g, c.b});
        float sat = mx > 0 ? (mx - mn) / mx : 0;
        if (sat > best_sat && c.luminance() > 0.1f && c.luminance() < 0.9f) {
            best_sat = sat;
            accent = c;
        }
    }

    Color bg_panel = {bg.r * 1.1f, bg.g * 1.1f, bg.b * 1.1f, 1.f};
    Color bg_input = {bg.r * 1.2f, bg.g * 1.2f, bg.b * 1.2f, 1.f};
    Color fg_dim   = {fg.r * 0.6f, fg.g * 0.6f, fg.b * 0.6f, 1.f};
    Color accent2  = Color::from_hsv(fmodf(
        [&]{ float h=0,s,v,mn=std::min({accent.r,accent.g,accent.b}),mx=std::max({accent.r,accent.g,accent.b});
             float d=mx-mn; s=mx>0?d/mx:0; v=mx;
             if(d>0){if(mx==accent.r)h=60*(accent.g-accent.b)/d;
             else if(mx==accent.g)h=120+60*(accent.b-accent.r)/d;
             else h=240+60*(accent.r-accent.g)/d;} if(h<0)h+=360; return h; }()
        + 60.f, 360.f), 0.7f, 0.8f);

    Theme t;
    t.bg       = bg;
    t.bg_panel = bg_panel;
    t.bg_input = bg_input;
    t.fg       = fg.enforce_contrast(bg);
    t.fg_dim   = fg_dim.enforce_contrast(bg, 3.0f);
    t.accent   = accent.enforce_contrast(bg);
    t.accent2  = accent2.enforce_contrast(bg);
    t.error    = Color{0.9f, 0.3f, 0.3f, 1.f}.enforce_contrast(bg);
    t.warning  = Color{0.9f, 0.7f, 0.2f, 1.f}.enforce_contrast(bg);
    t.success  = Color{0.3f, 0.8f, 0.4f, 1.f}.enforce_contrast(bg);

    // Derive syntax colors from palette + hue shifts
    float base_hue = 0; // compute from accent
    t.syntax.keyword     = Color::from_hsv(fmodf(base_hue + 270, 360), 0.7f, dark_bg ? 0.9f : 0.4f);
    t.syntax.type        = Color::from_hsv(fmodf(base_hue + 200, 360), 0.6f, dark_bg ? 0.85f : 0.45f);
    t.syntax.string      = Color::from_hsv(fmodf(base_hue + 120, 360), 0.6f, dark_bg ? 0.8f : 0.4f);
    t.syntax.comment     = fg_dim;
    t.syntax.number      = Color::from_hsv(fmodf(base_hue + 30,  360), 0.7f, dark_bg ? 0.85f : 0.45f);
    t.syntax.function    = accent;
    t.syntax.variable    = t.fg;
    t.syntax.preprocessor = Color::from_hsv(fmodf(base_hue + 180, 360), 0.5f, dark_bg ? 0.8f : 0.5f);

    // Enforce contrast on all syntax colors
    auto& s = t.syntax;
    s.keyword      = s.keyword.enforce_contrast(bg);
    s.type         = s.type.enforce_contrast(bg);
    s.string       = s.string.enforce_contrast(bg);
    s.number       = s.number.enforce_contrast(bg);
    s.function     = s.function.enforce_contrast(bg);
    s.preprocessor = s.preprocessor.enforce_contrast(bg);

    return t;
}

// ---- Public API ----

Theme Theme::from_image_path(const std::string& path) {
    if (path.empty()) return fallback_dark();

    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 3);
    if (!data) return fallback_dark();

    // Downsample to 64x64 for fast k-means
    const int S = 64;
    std::vector<uint8_t> small(S * S * 3);
    stbir_resize_uint8(data, w, h, 0, small.data(), S, S, 0, 3);
    stbi_image_free(data);

    std::vector<Color> pixels;
    pixels.reserve(S * S);
    for (int i = 0; i < S*S; ++i) {
        pixels.push_back({
            small[i*3+0] / 255.f,
            small[i*3+1] / 255.f,
            small[i*3+2] / 255.f,
            1.f
        });
    }

    auto palette = kmeans(pixels, 8);
    return build_theme(palette);
}

Theme Theme::from_wallpaper() {
    return from_image_path(get_wallpaper_path());
}

Theme Theme::fallback_dark() {
    Theme t;
    t.bg        = {0.08f, 0.08f, 0.10f, 1.f};
    t.bg_panel  = {0.11f, 0.11f, 0.14f, 1.f};
    t.bg_input  = {0.14f, 0.14f, 0.18f, 1.f};
    t.fg        = {0.88f, 0.90f, 0.92f, 1.f};
    t.fg_dim    = {0.42f, 0.45f, 0.50f, 1.f};
    t.accent    = {0.45f, 0.78f, 1.00f, 1.f};
    t.accent2   = {0.30f, 0.60f, 0.90f, 1.f};
    t.error     = {0.95f, 0.38f, 0.38f, 1.f};
    t.warning   = {0.95f, 0.78f, 0.25f, 1.f};
    t.success   = {0.35f, 0.88f, 0.50f, 1.f};
    t.syntax = {
        {0.55f, 0.75f, 1.00f, 1.f}, // keyword  - light blue
        {0.40f, 0.88f, 0.85f, 1.f}, // type     - cyan
        {0.75f, 0.88f, 0.45f, 1.f}, // string   - soft green
        {0.38f, 0.42f, 0.40f, 1.f}, // comment  - muted
        {0.98f, 0.72f, 0.42f, 1.f}, // number   - warm orange
        {0.45f, 0.78f, 1.00f, 1.f}, // function - light blue
        {0.88f, 0.90f, 0.92f, 1.f}, // variable - fg
        {0.85f, 0.55f, 0.42f, 1.f}, // preprocessor
    };
    return t;
}