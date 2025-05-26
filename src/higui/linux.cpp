#include "../external/glad.hpp"
#include "debug.hpp"
#include "platform.hpp"
#include "types.hpp"

#include <cstring>

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

namespace hi {

unsigned char key_array[256]{};

void trim_working_set() noexcept {
    // Do nothing
}

void *alloc(size_t size) noexcept {
    return mmap(nullptr, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void free(void *ptr, size_t size) noexcept { munmap(ptr, size); }

int exit(int code) noexcept { _exit(code); }

static void write_err(const char *s) noexcept {
    if (!s)
        return;
    write(STDERR_FILENO, s, strlen(s));
}

void panic_notify(const char *msg) noexcept {
    write_err(msg);
    write_err("\n");
    __builtin_trap(); // asm "ud2"
}

void panic(Result result) noexcept {
    static_assert(sizeof(Result) == 8U);
    static_assert(sizeof(Stage) == 4U);
    static_assert(sizeof(Error) == 4U);
    static_assert(static_cast<unsigned char>(Stage::__Max__) == 100U);
    static_assert(static_cast<unsigned char>(Error::__Max__) == 100U);

    char buf[16] = "Crash: S00 C00\n";
    unsigned s = (unsigned)result.stage % 100;
    unsigned e = (unsigned)result.error % 100;
    buf[8] = '0' + (s / 10);
    buf[9] = '0' + (s % 10);
    buf[12] = '0' + (e / 10);
    buf[13] = '0' + (e % 10);
    panic_notify(buf);
}

void sleep(unsigned ms) noexcept {
    struct timespec req = {ms / 1000, (long)(ms % 1000) * 1000000};
    nanosleep(&req, nullptr);
}

// returns seconds
double time() noexcept {
    timespec ts{};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
        return 0.0;
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

double calculate_delta_time() noexcept {
    static double last_time = hi::time();
    double current_time = hi::time();
    double delta = current_time - last_time;
    last_time = current_time;
    return delta;
}
} // namespace hi

namespace hi::window {
static Display *dsp = nullptr;
static Window win = 0;
static GLXContext ctx = nullptr;
static const hi::Callback *cb = nullptr;
static Atom wm_delete;
static int win_width = 0;
static int win_height = 0;

// not tested
int x11_error_handler(Display *display, XErrorEvent *error) noexcept {
    static_assert(sizeof(Result) == 8U);
    static_assert(sizeof(Stage) == 4U);
    static_assert(sizeof(Error) == 4U);

    char buf[] = "X11: C000, R000\n";
    unsigned c = (unsigned)error->error_code % 1000;
    unsigned r = (unsigned)error->request_code % 1000;

    // using magic numbers
    buf[6] = '0' + (c / 100) % 10; // code [0]..
    buf[7] = '0' + (c / 10) % 10;  // code .[0].
    buf[8] = '0' + (c % 10);       // code ..[0]

    buf[12] = '0' + (r / 100) % 10; // request [0]..
    buf[13] = '0' + (r / 10) % 10;  // request .[0].
    buf[14] = '0' + (r % 10);       // request ..[0]
    write_err(buf);
    return 0;
}

Handler create(const Callback *callbacks, GraphicsContext &graphics_context,
               int width, int height) noexcept {

    XSetErrorHandler(x11_error_handler);
    dsp = XOpenDisplay(0);
    if (!dsp)
        hi::panic({Stage::CreateWindow, Error::OpenDisplay});

    int scr = DefaultScreen(dsp);

    int fb_attribs[] = {GLX_RENDER_TYPE,
                        GLX_RGBA_BIT,
                        GLX_DRAWABLE_TYPE,
                        GLX_WINDOW_BIT,
                        GLX_DOUBLEBUFFER,
                        True,
                        GLX_RED_SIZE,
                        8,
                        GLX_GREEN_SIZE,
                        8,
                        GLX_BLUE_SIZE,
                        8,
                        GLX_DEPTH_SIZE,
                        24,
                        GLX_SAMPLE_BUFFERS,
                        0,
                        None};

    int fbcount;
    GLXFBConfig *fbc = glXChooseFBConfig(dsp, scr, fb_attribs, &fbcount);
    if (!fbc || fbcount == 0)
        hi::panic({Stage::Opengl, Error::ChooseFbConfig});

    GLXFBConfig best_fb_config = fbc[0];

    XVisualInfo *vi = glXGetVisualFromFBConfig(dsp, best_fb_config);
    if (!vi)
        hi::panic({Stage::Opengl, Error::GetVisualFromFbConfig});

    Colormap cmap =
        XCreateColormap(dsp, RootWindow(dsp, scr), vi->visual, AllocNone);

    XSetWindowAttributes swa = {};
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                     ButtonMotionMask | ButtonPressMask | PointerMotionMask |
                     StructureNotifyMask | FocusChangeMask;

    win = XCreateWindow(dsp, RootWindow(dsp, scr), 0, 0, width, height, 0,
                        vi->depth, InputOutput, vi->visual,
                        CWColormap | CWEventMask, &swa);

    wm_delete = XInternAtom(dsp, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dsp, win, &wm_delete, 1);

    XMapWindow(dsp, win);

    typedef GLXContext (*glXCreateContextAttribsARBProc)(
        Display *, GLXFBConfig, GLXContext, Bool, const int *);
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB =
        (glXCreateContextAttribsARBProc)glXGetProcAddressARB(
            (const GLubyte *)"glXCreateContextAttribsARB");

    if (!glXCreateContextAttribsARB)
        hi::panic({Stage::Opengl, Error::CreateContextAttribsArb});

    int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB,
                             4,
                             GLX_CONTEXT_MINOR_VERSION_ARB,
                             3,
                             GLX_CONTEXT_PROFILE_MASK_ARB,
                             GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                             None};

    ctx = glXCreateContextAttribsARB(dsp, best_fb_config, 0, True,
                                     context_attribs);
    if (!ctx)
        hi::panic({Stage::Opengl, Error::ModernOpenglContext});

    glXMakeCurrent(dsp, win, ctx);
    XFree(vi);

    // #define HI_VSYNC // Uncomment this line to enable VSync

#ifndef HI_VSYNC
    // try EXT
    auto glXSwapIntervalEXT =
        (void (*)(Display *, GLXDrawable, int))glXGetProcAddressARB(
            (const GLubyte *)"glXSwapIntervalEXT");

    if (glXSwapIntervalEXT)
        glXSwapIntervalEXT(dsp, win, 0);
    else {
        // try MESA
        auto glXSwapIntervalMESA = (int (*)(unsigned int))glXGetProcAddressARB(
            (const GLubyte *)"glXSwapIntervalMESA");

        if (glXSwapIntervalMESA)
            glXSwapIntervalMESA(0);
        else {
            // try SGI
            auto glXSwapIntervalSGI = (int (*)(int))glXGetProcAddressARB(
                (const GLubyte *)"glXSwapIntervalSGI");

            if (glXSwapIntervalSGI)
                glXSwapIntervalSGI(0);
        }
    }
#endif

    graphics_context = (GraphicsContext)ctx;
    cb = callbacks;
    return (Handler)win;
}

void swap_buffers(const GraphicsContext) noexcept { glXSwapBuffers(dsp, win); }

void destroy(Handler) noexcept {
    glXDestroyContext(dsp, ctx);
    XDestroyWindow(dsp, win);
    XCloseDisplay(dsp);
}

static inline unsigned char get_keycode(KeySym ks) {
    // See hi::KeyCode

    /* ASCII
        XK_space      0x0020
        ...
        XK_asciitilde 0x007e (U+007E TILDE) */

    /* MODIFIERS
         XK_Shift_L    0xffe1 // 65505
         ...
         XK_Hyper_R    0xffee // 65518 */

    /* FUNCTIONAL
       XK_F1  0xffbe // 65470
       ...
       XK_F11 0xffc8 // 65480 */

    /* TTY
         XK_BackSpace 0xff08   // 652,88
                               // empty 652,92 (1 char)
                               // empty 652,94-652,98 (5 chars)
                               // empty 653,02 - 653,06 (5 chars)
         XK_Escape 0xff1b      // 653,07 */

    /* MOTION
       XK_Home  0xff50 // 65360
       ...
       XK_Down  0xff54 // 65364 */

    struct RangeShort {
        unsigned short begin;
        unsigned short end;

        consteval RangeShort(unsigned short begin, unsigned short end) noexcept
            : begin{begin}, end{end} {}

        unsigned short offset() const noexcept { return end - begin; }

        bool in_range(KeySym key_sym) const noexcept {
            return key_sym >= begin && key_sym <= end;
        }

        unsigned char calculate_key(unsigned char global_offset,
                                    KeySym key_sym) const noexcept {
            return global_offset + (key_sym - begin);
        }
    }; // struct RangeShort

    // clang-format off
    constexpr RangeShort ranges[]{
        /* ascii  */ { /* begin */ XK_space,     /* end */ XK_asciitilde + 1},
        /* mods   */ { /* begin */ XK_Shift_L,   /* end */ XK_Hyper_R + 1},
        /* func   */ { /* begin */ XK_F1,        /* end */ XK_F11 + 1},
        /* tty    */ { /* begin */ XK_BackSpace, /* end */ XK_Escape + 1},
        /* motion */ { /* begin */ XK_Home,      /* end */ XK_Down + 1}};
    // clang-format on

    unsigned char global_offset = XK_space;
    for (auto &r : ranges) {
        if (r.in_range(ks))
            return r.calculate_key(global_offset, ks);
        global_offset += r.offset();
    }
    return 0; // not found
}

// returns `false` if user closed the window
static inline bool handle_event(XEvent &e) noexcept {
    static Time last_key_time = 0;
    static KeyCode last_keycode = 0;

    switch (e.type) {
    case MotionNotify:
        cb->mouse_move(*cb, e.xmotion.x, e.xmotion.y);
        break;
    case KeyPress: {
        if (e.xkey.time == last_key_time && e.xkey.keycode == last_keycode)
            break;
        last_key_time = e.xkey.time;
        last_keycode = e.xkey.keycode;

        unsigned char hi_keycode = get_keycode(XLookupKeysym(&e.xkey, 0));
        hi::key_array[hi_keycode] = 1;
        break;
    }
    case KeyRelease: {
        if (XEventsQueued(dsp, QueuedAfterReading)) {
            XEvent nev;
            XPeekEvent(dsp, &nev);
            if (nev.type == KeyPress && nev.xkey.time == e.xkey.time &&
                nev.xkey.keycode == e.xkey.keycode) {
                break;
            }
        }
        unsigned char hi_keycode = get_keycode(XLookupKeysym(&e.xkey, 0));
        hi::key_array[hi_keycode] = 0;
        cb->key_up(*cb, static_cast<::hi::key::KeyCode>(hi_keycode));
        break;
    }
    case ButtonPress:
        break;
    case ConfigureNotify:
        win_width = e.xconfigure.width;
        win_height = e.xconfigure.height;
        cb->resize(*cb, e.xconfigure.width, e.xconfigure.height);
        break;
    case FocusIn:
        cb->focus_gained(*cb);
        break;
    case FocusOut:
        cb->focus_lost(*cb);
        break;
    case ClientMessage:
        if ((Atom)e.xclient.data.l[0] == wm_delete)
            return false;
        break;
    }
    return true;
}

// `false` if user closed the window
bool poll_events(const Handler) noexcept {
    bool still_running = true;
    while (XPending(dsp)) {
        XEvent e;
        XNextEvent(dsp, &e);
        if (handle_event(e) == false)
            still_running = false;
    }
    return still_running;
}

bool is_valid(const Handler) noexcept { return dsp && win; }

void set_title(const Handler, const char *title) noexcept {
    XStoreName(dsp, win, title);
}

void get_size(const Handler, int &w, int &h) noexcept {
    XWindowAttributes wa;
    if (XGetWindowAttributes(dsp, win, &wa)) {
        w = wa.width;
        h = wa.height;
    } else {
        w = h = 0;
    }
}

void setup_opengl_context(const Handler, const GraphicsContext) noexcept {
    // context has been already created in `create()`
}

void load_gl() noexcept {
    // We don't check if functions loaded correctly
    // and let's hope that everything will go according to the plan
    gladLoadGLLoader((GLADloadproc)glXGetProcAddress);

    debug_print("OpenGL version: %s\n", glGetString(GL_VERSION));
    debug_print("Renderer: %s\n", glGetString(GL_RENDERER));
    debug_print("Vendor: %s\n", glGetString(GL_VENDOR));
}

bool is_anisotropy_supported() noexcept {
    const char *exts =
        reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
    return exts && std::strstr(exts, "GL_EXT_texture_filter_anisotropic");
}

float query_max_anisotropy() noexcept {
    float max = 1.0f;
    glGetFloatv(0x84FF /*GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT*/, &max);
    return max;
}

void set_fullscreen(const Handler, bool) noexcept {
    // not implemented
}

void center_cursor(const Handler handler) noexcept {
    XWarpPointer(dsp, None, win, 0, 0, 0, 0, (int)(win_width / 2),
                 (int)(win_height / 2));
}

void set_cursor_visible(const Handler handler, bool visible) noexcept {
    if (visible) {
        XUndefineCursor(dsp, win);
        return;
    }
    Pixmap blank = XCreatePixmap(dsp, win, 1, 1, 1);
    XColor dummy;
    char data[1] = {0};
    Pixmap mask = XCreateBitmapFromData(dsp, win, data, 1, 1);
    Cursor invisible =
        XCreatePixmapCursor(dsp, blank, mask, &dummy, &dummy, 0, 0);

    XDefineCursor(dsp, win, invisible);
    XFreePixmap(dsp, blank);
    XFreePixmap(dsp, mask);
    XFreeCursor(dsp, invisible);
}

void send_quit_message(const Handler handler) noexcept {
    XEvent ev{};
    ev.xclient.type = ClientMessage;
    ev.xclient.serial = 0;
    ev.xclient.send_event = True;
    ev.xclient.display = dsp;
    ev.xclient.window = win;
    ev.xclient.message_type = XInternAtom(dsp, "WM_PROTOCOLS", True);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = wm_delete;
    ev.xclient.data.l[1] = CurrentTime;

    XSendEvent(dsp, win, False, NoEventMask, &ev);
    XFlush(dsp);
}

} // namespace hi::window
