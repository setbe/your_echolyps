#include "higui_platform.hpp"
#include "higui_types.hpp"

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#if defined(__linux__) && defined(NDEBUG)
extern int main();
extern "C" __attribute__((used, noinline, optimize("O0"))) void _start() {
    int code = main();
    asm volatile("mov %0, %%rdi\n\t"
                 "mov $60, %%rax\n\t"
                 "syscall"
                 :
                 : "r"((long)code)
                 : "%rax", "%rdi");
}
#endif

extern "C" long sys(long n, long a = 0, long b = 0, long c = 0, long d = 0,
                    long e = 0, long f = 0) {
    long ret;
    asm volatile("movq %1, %%rax\n\t"
                 "movq %2, %%rdi\n\t"
                 "movq %3, %%rsi\n\t"
                 "movq %4, %%rdx\n\t"
                 "movq %5, %%r10\n\t"
                 "movq %6, %%r8\n\t"
                 "movq %7, %%r9\n\t"
                 "syscall\n\t"
                 : "=a"(ret)
                 : "r"(n), "r"(a), "r"(b), "r"(c), "r"(d), "r"(e), "r"(f)
                 : "rcx", "r11", "memory");
    return ret;
}

namespace hi {
unsigned key[256] = {0};

void trim_working_set() noexcept {
    // Do nothing
}

void *alloc(unsigned size) noexcept {
    constexpr long sys_mmap = 9;
    // PROT_READ|PROT_WRITE = 1|2 = 3, MAP_PRIVATE|MAP_ANONYMOUS = 2|32 = 34
    return (void *)sys(sys_mmap, 0UL, size, 3, 34, -1, 0UL);
}

void free(void *ptr, unsigned size) noexcept {
    constexpr long sys_munmap = 11;
    sys(sys_munmap, (long)ptr, size);
}

int exit(int code) noexcept { return 0; }

static void write_err(const char *s) noexcept {
    if (!s)
        return;
    constexpr long stderr_fileno = 2;
    const char *p = s;
    while (*p)
        ++p;
    long len = p - s;

    while (len > 0) {
        long written = sys(1 /* write */, stderr_fileno, (long)s, len);
        if (written <= 0)
            break;
        s += written;
        len -= written;
    }
}

void panic_notify(Error /*error*/, const char *msg) noexcept {
    write_err(msg);
    write_err("\n");
    asm volatile("ud2"); // crash here
}

void panic(Result result) noexcept {
    static_assert(sizeof(Result) == 8U);
    static_assert(sizeof(Stage) == 4U);
    static_assert(sizeof(Error) == 4U);
    char buf[16] = "Crash: S00 C00\n";
    unsigned s = (unsigned)result.stage % 100;
    unsigned e = (unsigned)result.error % 100;
    buf[8] = '0' + (s / 10);
    buf[9] = '0' + (s % 10);
    buf[12] = '0' + (e / 10);
    buf[13] = '0' + (e % 10);
    panic_notify(result.error, buf);
}

void sleep(unsigned ms) noexcept {
    constexpr long sys_nanosleep = 35;
    struct timespec req = {ms / 1000, (long)(ms % 1000) * 1000000};
    sys(sys_nanosleep, (long)&req, 0);
}

// returns seconds
double time() noexcept {
    timespec ts;
    constexpr long clock_monotonic = 1;
    constexpr long sys_clock_gettime = 228;
    long res = sys(sys_clock_gettime, clock_monotonic, (long)&ts);
    if (res < 0)
        hi::panic(Result{Stage::LinuxSyscall, Error::SyscallGetTime});
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
} // namespace hi

namespace hi::window {
static Display *dsp = nullptr;
static Window win = 0;
static GLXContext ctx = nullptr;
static const hi::Callback *cb = nullptr;
static Atom wm_delete;

int x11_error_handler(Display *display, XErrorEvent *error) noexcept {
    static_assert(sizeof(Result) == 8U);
    static_assert(sizeof(Stage) == 4U);
    static_assert(sizeof(Error) == 4U);

    char buf[] = "X11: C000, R000\n";
    unsigned c = (unsigned)error->error_code % 1000;
    unsigned r = (unsigned)error->request_code % 1000;

    buf[6] = '0' + (c / 100) % 10;
    buf[7] = '0' + (c / 10) % 10;
    buf[8] = '0' + (c % 10);
    buf[12] = '0' + (r / 100) % 10;
    buf[13] = '0' + (r / 10) % 10;
    buf[14] = '0' + (r % 10);
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
    static int vis_attribs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, None};
    XVisualInfo *vi = glXChooseVisual(dsp, scr, vis_attribs);
    XSync(dsp, False);
    if (!vi)
        hi::panic({Stage::CreateWindow, Error::CreateWindow});

    Colormap cmap =
        XCreateColormap(dsp, RootWindow(dsp, scr), vi->visual, AllocNone);
    XSync(dsp, False);

    XSetWindowAttributes swa = {};
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                     ButtonMotionMask | StructureNotifyMask | FocusChangeMask;

    win = XCreateWindow(dsp, RootWindow(dsp, scr), 0, 0, width, height, 0,
                        vi->depth, InputOutput, vi->visual,
                        CWColormap | CWEventMask, &swa);
    XSync(dsp, False);

    wm_delete = XInternAtom(dsp, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dsp, win, &wm_delete, 1);
    XSync(dsp, False);

    XMapWindow(dsp, win);
    XSync(dsp, False);

    ctx = glXCreateContext(dsp, vi, 0, True);
    XSync(dsp, False);
    if (!ctx)
        hi::panic({Stage::Opengl, Error::ModernOpenglContext});

    glXMakeCurrent(dsp, win, ctx);
    XSync(dsp, False);
    graphics_context = (GraphicsContext)ctx;
    cb = callbacks;
    return (Handler)win;
}

void swap_buffers(const GraphicsContext) noexcept { glXSwapBuffers(dsp, win); }

void destroy(Handler) noexcept {
    if (ctx) {
        glXDestroyContext(dsp, ctx);
#ifdef HI_MULTITHREADING_USED
        ctx = 0;
#endif
    }
    if (win) {
        XDestroyWindow(dsp, win);
#ifdef HI_MULTITHREADING_USED
        win = 0;
#endif
    }
    if (dsp) {
        XCloseDisplay(dsp);
#ifdef HI_MULTITHREADING_USED
        dsp = 0;
#endif
    }
}

// returns `false` if user closed the window
static inline bool handle_event(XEvent &e) noexcept {
    switch (e.type) {
    case MotionNotify:
        cb->mouse_move(*cb, e.xmotion.x, e.xmotion.y);
        break;
    case KeyPress: {
        KeySym ks = XLookupKeysym(&e.xkey, 0);
        if (ks < 256)
            hi::key[ks] = 1;
        break;
    }
    case KeyRelease: {
        KeySym ks = XLookupKeysym(&e.xkey, 0);
        if (ks < 256) {
            hi::key[ks] = 0;
            cb->key_up(*cb, ks);
        }
        break;
    }
    case ConfigureNotify:
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
    // We don't check this one, because I'm lazy
    // and let's just hope that everything will go according to the plan
    gladLoadGLLoader((GLADloadproc)glXGetProcAddress);
}

void set_fullscreen(const Handler, bool) noexcept {
    // not implemented
}
} // namespace hi::window
