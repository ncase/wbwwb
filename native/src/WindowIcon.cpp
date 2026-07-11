#include "WindowIcon.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "resource.h"
#endif

namespace wb {

void applyEmbeddedWindowIcon(void* nativeWindowHandle) {
#ifdef _WIN32
    HWND hwnd = static_cast<HWND>(nativeWindowHandle);
    if (!hwnd) {
        return;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    static HICON bigIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_WBWWB), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
    static HICON smallIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_WBWWB), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

    if (bigIcon) {
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(bigIcon));
        SetClassLongPtrW(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(bigIcon));
    }
    if (smallIcon) {
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
        SetClassLongPtrW(hwnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(smallIcon));
    }
#else
    (void)nativeWindowHandle;
#endif
}

}  // namespace wb
