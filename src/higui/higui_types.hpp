#pragma once

#ifdef _WIN32
extern "C" const int _fltused;
#endif // !_WIN32

#ifndef HI_RESTRICT
#ifdef __GNUC__
#define HI_RESTRICT __restrict__
#elif defined(_MSC_VER)
#define HI_RESTRICT __restrict
#else
#define HI_RESTRICT
#endif
#endif // !HI_RESTRICT

#include "../external/glad.hpp"

namespace hi {
// ===== Window stuff =====
namespace window {
// ===== Contains all info related to window backend =====
enum class Backend {
    Unknown,
    X11,
    WindowsAPI,
    Cocoa,
    AndroidNDK,
};

consteval Backend get_window_backend() {
#if defined(__linux__) // Linux
    return Backend::X11;
#elif defined(_WIN32)      // Windows
    return Backend::WindowsAPI;
#elif defined(__APPLE__)   // Apple
    return Backend::Cocoa;
#elif defined(__ANDROID__) // Android
    return Backend::AndroidNDK;
#else                      // Unknown
    return Backend::Unknown;
#endif
}

template <Backend backend> struct Selector;

template <> struct Selector<Backend::X11> {
    using Handler = unsigned int; // Window
    using DeviceContext = void *; // Display*
};

template <> struct Selector<Backend::WindowsAPI> {
    using Handler = void *;       // hWnd
    using DeviceContext = void *; // hDc
};

using Handler = Selector<get_window_backend()>::Handler;
using GraphicsContext = Selector<get_window_backend()>::DeviceContext;
} // namespace window

enum class Stage : unsigned {
    CreateWindow,
    Opengl,
    Engine,
    Game,

    __Count__,
    __Max__ = 100
}; // !Stage

enum class Error : unsigned {
    None,
    InternalMemoryAlloc,
    FontMemoryAlloc,
    ChunkMemoryAlloc,

    // Opengl Context
    CreateDummyWindowClassname,
    CreateDummyWindow,
    SetDummyPixelFormat,
    NotSupportedRequiredWglExtensions,
    EnableVSync,
    ModernOpenglContext,
    CreateContextAttribsArb,
    ChooseFbConfig,
    GetVisualFromFbConfig,

    // Window
    OpenDisplay,
    CreateWindowClassname,
    CreateWindow,
    LoadOpenglFunctions,

    // Opengl
    CompileShader,
    CreateShaderProgram,
    GenerateVertexArray,
    GenerateBuffer,
    GenerateTexture,

    __Count__,
    __Max__ = 100
}; // !Error

struct Result {
    Stage stage;
    Error error;
};

// Key states (pressed = true, otherwise false)
extern unsigned char key_array[];

namespace key {
#if defined(__linux__)
enum class KeyCode : unsigned char {
    // Mouse
    mouse_left = 1,   // Button1
    mouse_middle = 2, // Button2
    mouse_right = 3,  // Button3
    scroll_up = 4,    // Button4
    scroll_down = 5,  // Button5
    scroll_left = 6,  // Button6 (rarely used)
    scroll_right = 7, // Button7 (rarely used)
    mouse_x1 = 8,     // Button8 first additional mouse button
    mouse_x2 = 9,     // Button9 second additional mouse button

    // ASCII
    space = 0x20,
    exclam = 0x21,
    quotedbl = 0x22,
    numbersign = 0x23,
    dollar = 0x24,
    percent = 0x25,
    ampersand = 0x26,
    apostrophe = 0x27,
    quoteright = 0x27,
    parenleft = 0x28,
    parenright = 0x29,
    asterisk = 0x2a,
    plus = 0x2b,
    comma = 0x2c,
    minus = 0x2d,
    period = 0x2e,
    slash = 0x2f,
    digit_0 = 0x30,
    digit_1 = 0x31,
    digit_2 = 0x32,
    digit_3 = 0x33,
    digit_4 = 0x34,
    digit_5 = 0x35,
    digit_6 = 0x36,
    digit_7 = 0x37,
    digit_8 = 0x38,
    digit_9 = 0x39,
    colon = 0x3a,
    semicolon = 0x3b,
    less = 0x3c,
    equal = 0x3d,
    greater = 0x3e,
    question = 0x3f,
    at = 0x40,
    A = 0x41,
    B = 0x42,
    C = 0x43,
    D = 0x44,
    E = 0x45,
    F = 0x46,
    G = 0x47,
    H = 0x48,
    I = 0x49,
    J = 0x4a,
    K = 0x4b,
    L = 0x4c,
    M = 0x4d,
    N = 0x4e,
    O = 0x4f,
    P = 0x50,
    Q = 0x51,
    R = 0x52,
    S = 0x53,
    T = 0x54,
    U = 0x55,
    V = 0x56,
    W = 0x57,
    X = 0x58,
    Y = 0x59,
    Z = 0x5a,
    bracketleft = 0x5b,
    backslash = 0x5c,
    bracketright = 0x5d,
    asciicircum = 0x5e,
    underscore = 0x5f,
    grave = 0x60,
    quoteleft = 0x60,
    a = 0x61,
    b = 0x62,
    c = 0x63,
    d = 0x64,
    e = 0x65,
    f = 0x66,
    g = 0x67,
    h = 0x68,
    i = 0x69,
    j = 0x6a,
    k = 0x6b,
    l = 0x6c,
    m = 0x6d,
    n = 0x6e,
    o = 0x6f,
    p = 0x70,
    q = 0x71,
    r = 0x72,
    s = 0x73,
    t = 0x74,
    u = 0x75,
    v = 0x76,
    w = 0x77,
    x = 0x78,
    y = 0x79,
    z = 0x7a,
    braceleft = 0x7b,
    bar = 0x7c,
    braceright = 0x7d,
    asciitilde = 0x7e,

    // MODIFIERS
    Shift_L = 127,
    Shift_R = 128,
    Control_L = 129,
    Control_R = 130,
    Caps_Lock = 131,
    Shift_Lock = 132,
    Meta_L = 133,
    Meta_R = 134,
    Alt_L = 135,
    Alt_R = 136,
    Super_L = 137,
    Super_R = 138,
    Hyper_L = 139,
    Hyper_R = 140,

    // FUNCTIONAL
    F1 = 141,
    F2 = 142,
    F3 = 143,
    F4 = 144,
    F5 = 145,
    F6 = 146,
    F7 = 147,
    F8 = 148,
    F9 = 149,
    F10 = 150,
    F11 = 151,

    // TTY
    BackSpace = 152,
    Tab = 153,
    Linefeed = 154,
    Clear = 155,
    // skip 1
    Return = 157,
    // skip 5
    Pause = 163,
    Scroll_Lock = 164,
    Sys_Req = 165,
    // skip 5
    Escape = 171,

    // MOTION
    Home = 172,
    Left = 173,
    Up = 174,
    Right = 175,
    Down = 176
};

#endif // defined(__linux__)

struct Key {
    constexpr Key(KeyCode code) : code(code) {}

    bool operator&&(const Key &other) const noexcept {
        return ::hi::key_array[static_cast<unsigned char>(code)] &&
               ::hi::key_array[static_cast<unsigned char>(other.code)];
    }

    operator bool() const noexcept {
        return ::hi::key_array[static_cast<unsigned char>(code)] != 0;
    }

    KeyCode code;
};

// Mouse

constexpr Key mouse_left{KeyCode::mouse_left};
constexpr Key mouse_middle{KeyCode::mouse_middle};
constexpr Key mouse_right{KeyCode::mouse_right};
constexpr Key scroll_up{KeyCode::scroll_up};
constexpr Key scroll_down{KeyCode::scroll_down};
constexpr Key scroll_left{KeyCode::scroll_left};
constexpr Key scroll_right{KeyCode::scroll_right};
constexpr Key mouse_x1{KeyCode::mouse_x1};
constexpr Key mouse_x2{KeyCode::mouse_x2};

// ASCII

constexpr Key space{KeyCode::space};
constexpr Key exclam{KeyCode::exclam};
constexpr Key quotedbl{KeyCode::quotedbl};
constexpr Key numbersign{KeyCode::numbersign};
constexpr Key dollar{KeyCode::dollar};
constexpr Key percent{KeyCode::percent};
constexpr Key ampersand{KeyCode::ampersand};
constexpr Key apostrophe{KeyCode::apostrophe};
constexpr Key quoteright{KeyCode::quoteright};
constexpr Key parenleft{KeyCode::parenleft};
constexpr Key parenright{KeyCode::parenright};
constexpr Key asterisk{KeyCode::asterisk};
constexpr Key plus{KeyCode::plus};
constexpr Key comma{KeyCode::comma};
constexpr Key minus{KeyCode::minus};
constexpr Key period{KeyCode::period};
constexpr Key slash{KeyCode::slash};
constexpr Key digit_0{KeyCode::digit_0};
constexpr Key digit_1{KeyCode::digit_1};
constexpr Key digit_2{KeyCode::digit_2};
constexpr Key digit_3{KeyCode::digit_3};
constexpr Key digit_4{KeyCode::digit_4};
constexpr Key digit_5{KeyCode::digit_5};
constexpr Key digit_6{KeyCode::digit_6};
constexpr Key digit_7{KeyCode::digit_7};
constexpr Key digit_8{KeyCode::digit_8};
constexpr Key digit_9{KeyCode::digit_9};
constexpr Key colon{KeyCode::colon};
constexpr Key semicolon{KeyCode::semicolon};
constexpr Key less{KeyCode::less};
constexpr Key equal{KeyCode::equal};
constexpr Key greater{KeyCode::greater};
constexpr Key question{KeyCode::question};
constexpr Key at{KeyCode::at};
constexpr Key A{KeyCode::A};
constexpr Key B{KeyCode::B};
constexpr Key C{KeyCode::C};
constexpr Key D{KeyCode::D};
constexpr Key E{KeyCode::E};
constexpr Key F{KeyCode::F};
constexpr Key G{KeyCode::G};
constexpr Key H{KeyCode::H};
constexpr Key I{KeyCode::I};
constexpr Key J{KeyCode::J};
constexpr Key K{KeyCode::K};
constexpr Key L{KeyCode::L};
constexpr Key M{KeyCode::M};
constexpr Key N{KeyCode::N};
constexpr Key O{KeyCode::O};
constexpr Key P{KeyCode::P};
constexpr Key Q{KeyCode::Q};
constexpr Key R{KeyCode::R};
constexpr Key S{KeyCode::S};
constexpr Key T{KeyCode::T};
constexpr Key U{KeyCode::U};
constexpr Key V{KeyCode::V};
constexpr Key W{KeyCode::W};
constexpr Key X{KeyCode::X};
constexpr Key Y{KeyCode::Y};
constexpr Key Z{KeyCode::Z};
constexpr Key bracketleft{KeyCode::bracketleft};
constexpr Key backslash{KeyCode::backslash};
constexpr Key bracketright{KeyCode::bracketright};
constexpr Key asciicircum{KeyCode::asciicircum};
constexpr Key underscore{KeyCode::underscore};
constexpr Key grave{KeyCode::grave};
constexpr Key quoteleft{KeyCode::quoteleft};
constexpr Key a{KeyCode::a};
constexpr Key b{KeyCode::b};
constexpr Key c{KeyCode::c};
constexpr Key d{KeyCode::d};
constexpr Key e{KeyCode::e};
constexpr Key f{KeyCode::f};
constexpr Key g{KeyCode::g};
constexpr Key h{KeyCode::h};
constexpr Key i{KeyCode::i};
constexpr Key j{KeyCode::j};
constexpr Key k{KeyCode::k};
constexpr Key l{KeyCode::l};
constexpr Key m{KeyCode::m};
constexpr Key n{KeyCode::n};
constexpr Key o{KeyCode::o};
constexpr Key p{KeyCode::p};
constexpr Key q{KeyCode::q};
constexpr Key r{KeyCode::r};
constexpr Key s{KeyCode::s};
constexpr Key t{KeyCode::t};
constexpr Key u{KeyCode::u};
constexpr Key v{KeyCode::v};
constexpr Key w{KeyCode::w};
constexpr Key x{KeyCode::x};
constexpr Key y{KeyCode::y};
constexpr Key z{KeyCode::z};
constexpr Key braceleft{KeyCode::braceleft};
constexpr Key bar{KeyCode::bar};
constexpr Key braceright{KeyCode::braceright};
constexpr Key asciitilde{KeyCode::asciitilde};

// MODIFIERS

constexpr Key Shift_L{KeyCode::Shift_L};
constexpr Key Shift_R{KeyCode::Shift_R};
constexpr Key Control_L{KeyCode::Control_L};
constexpr Key Control_R{KeyCode::Control_R};
constexpr Key Caps_Lock{KeyCode::Caps_Lock};
constexpr Key Shift_Lock{KeyCode::Shift_Lock};
constexpr Key Meta_L{KeyCode::Meta_L};
constexpr Key Meta_R{KeyCode::Meta_R};
constexpr Key Alt_L{KeyCode::Alt_L};
constexpr Key Alt_R{KeyCode::Alt_R};
constexpr Key Super_L{KeyCode::Super_L};
constexpr Key Super_R{KeyCode::Super_R};
constexpr Key Hyper_L{KeyCode::Hyper_L};
constexpr Key Hyper_R{KeyCode::Hyper_R};

// FUNCTIONAL

constexpr Key F1{KeyCode::F1};
constexpr Key F2{KeyCode::F2};
constexpr Key F3{KeyCode::F3};
constexpr Key F4{KeyCode::F4};
constexpr Key F5{KeyCode::F5};
constexpr Key F6{KeyCode::F6};
constexpr Key F7{KeyCode::F7};
constexpr Key F8{KeyCode::F8};
constexpr Key F9{KeyCode::F9};
constexpr Key F10{KeyCode::F10};
constexpr Key F11{KeyCode::F11};

// TTY

constexpr Key BackSpace{KeyCode::BackSpace};
constexpr Key Tab{KeyCode::Tab};
constexpr Key Linefeed{KeyCode::Linefeed};
constexpr Key Clear{KeyCode::Clear};
constexpr Key Return{KeyCode::Return};
constexpr Key Pause{KeyCode::Pause};
constexpr Key Scroll_Lock{KeyCode::Scroll_Lock};
constexpr Key Sys_Req{KeyCode::Sys_Req};
constexpr Key Escape{KeyCode::Escape};

// MOTION

constexpr Key Home{KeyCode::Home};
constexpr Key Left{KeyCode::Left};
constexpr Key Up{KeyCode::Up};
constexpr Key Right{KeyCode::Right};
constexpr Key Down{KeyCode::Down};

} // namespace key

// ===== Callback stuff =====
struct Callback;

namespace internal {} // namespace internal

struct Callback {
    using Void = void (*)(const Callback &);
    using VoidInt = void (*)(const Callback &, int);
    using VoidKeycode = void (*)(const Callback &, key::KeyCode);
    using VoidIntInt = void (*)(const Callback &, int, int);

    inline Callback(void *user_data, VoidIntInt resize, VoidIntInt mouse_move,
                    VoidKeycode key_up, Void focus_gained,
                    Void focus_lost) noexcept
        : user_data{user_data}, resize{resize}, mouse_move{mouse_move},
          key_up{key_up}, focus_gained{focus_gained}, focus_lost{focus_lost} {}

    VoidIntInt resize;
    VoidIntInt mouse_move;
    VoidKeycode key_up;
    Void focus_gained;
    Void focus_lost;

    template <typename T> inline T *get_user_data() const noexcept {
        return reinterpret_cast<T *>(user_data);
    }

  private:
    void *user_data;
}; // struct Callback

} // namespace hi