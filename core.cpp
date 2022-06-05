#include <Python.h>
#include <Windows.h>
#include <GL/GL.h>

#include <powersetting.h>

#define WGL_CONTEXT_PROFILE_MASK 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT 0x0001
#define WGL_CONTEXT_MAJOR_VERSION 0x2091
#define WGL_CONTEXT_MINOR_VERSION 0x2092
#define WGL_CONTEXT_FLAGS 0x2094
#define WGL_CONTEXT_DEBUG_BIT 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT 0x0002
#define WGL_CONTEXT_FLAG_NO_ERROR_BIT 0x0008

typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hdc, HGLRC hrc, const int * attrib_list);
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int interval);

typedef void (APIENTRY * GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char * message, const void * user_param);
typedef void (APIENTRY * PFNGLDEBUGMESSAGECONTROLPROC)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint * ids, GLboolean enabled);
typedef void (APIENTRY * PFNGLDEBUGMESSAGECALLBACKPROC)(GLDEBUGPROC callback, const void * user_param);

void APIENTRY debug_output(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char * message, const void * user_param) {
    fprintf(stderr, "%s\n", message);
}

HWND hwnd;
HDC hdc;
HGLRC hrc;

int width;
int height;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    switch (umsg) {
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

PyObject * meth_init(PyObject * self, PyObject * args, PyObject * kwargs) {
    static char * keywords[] = {"debug", "vsync", NULL};

    int debug = false;
    int vsync = true;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|pp", keywords, &debug, &vsync)) {
        return NULL;
    }

    HANDLE process = GetCurrentProcess();
    SetPriorityClass(process, HIGH_PRIORITY_CLASS);
    SetProcessPriorityBoost(process, false);
    PowerSetActiveScheme(NULL, &GUID_MIN_POWER_SAVINGS);

    HINSTANCE hinst = GetModuleHandle(NULL);
    HCURSOR hcursor = (HCURSOR)LoadCursor(NULL, IDC_ARROW);
    WNDCLASS wnd_class = {CS_OWNDC, WindowProc, 0, 0, hinst, NULL, hcursor, NULL, NULL, "mywindow"};
    RegisterClass(&wnd_class);

    width = 1280;
    height = 720;

    int style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, style, false);

    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    int x = (sw - w) / 2;
    int y = (sh - h) / 2;

    const char * title = debug ? "OpenGL Window (debug)" : "OpenGL Window";
    hwnd = CreateWindow("mywindow", title, style, x, y, w, h, NULL, NULL, hinst, NULL);
    if (!hwnd) {
        PyErr_BadInternalCall();
        return NULL;
    }

    hdc = GetDC(hwnd);
    if (!hdc) {
        PyErr_BadInternalCall();
        return NULL;
    }

    DWORD pfd_flags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED | PFD_DOUBLEBUFFER;
    PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR), 1, pfd_flags, 0, 32};

    int pixelformat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelformat) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (!SetPixelFormat(hdc, pixelformat, &pfd)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    HGLRC loader_hglrc = wglCreateContext(hdc);
    if (!loader_hglrc) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (!wglMakeCurrent(hdc, loader_hglrc)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if (!wglCreateContextAttribsARB) {
        PyErr_BadInternalCall();
        return NULL;
    }

    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (!wglSwapIntervalEXT) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (!wglMakeCurrent(NULL, NULL)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (!wglDeleteContext(loader_hglrc)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    int context_flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT;

    if (debug) {
        context_flags |= WGL_CONTEXT_DEBUG_BIT;
    } else {
        context_flags |= WGL_CONTEXT_FLAG_NO_ERROR_BIT;
    }

    int attribs[] = {
        WGL_CONTEXT_FLAGS, context_flags,
        WGL_CONTEXT_PROFILE_MASK, WGL_CONTEXT_CORE_PROFILE_BIT,
        WGL_CONTEXT_MAJOR_VERSION, 4,
        WGL_CONTEXT_MINOR_VERSION, 6,
        0, 0,
    };

    hrc = wglCreateContextAttribsARB(hdc, NULL, attribs);

    if (!hrc) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (!wglMakeCurrent(hdc, hrc)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    wglSwapIntervalEXT(vsync);

    if (debug) {
        PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
        PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)wglGetProcAddress("glDebugMessageControl");
        if (!glDebugMessageCallback || !glDebugMessageControl) {
            PyErr_BadInternalCall();
            return NULL;
        }

        glDebugMessageCallback(debug_output, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);
    }

    PyModule_AddObject(self, "window_size", Py_BuildValue("(ii)", width, height));
    Py_RETURN_NONE;
}

PyObject * meth_update(PyObject * self) {
    SwapBuffers(hdc);

    MSG msg = {};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            Py_RETURN_FALSE;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Py_RETURN_TRUE;
}

PyMethodDef module_methods[] = {
    {"init", (PyCFunction)meth_init, METH_VARARGS | METH_KEYWORDS, NULL},
    {"update", (PyCFunction)meth_update, METH_NOARGS, NULL},
    {},
};

PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "core", NULL, -1, module_methods};

extern "C" PyObject * PyInit_core() {
    PyObject * module = PyModule_Create(&module_def);
    return module;
}
