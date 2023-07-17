#pragma once

#include "windows.h"
#include "Types.h"

// Create a memory device context (DC) compatible with the specified device
HDC create_compatible_dc(HDC hdc) { 
    return CreateCompatibleDC(hdc);
}

int release_dc(HWND hWnd, HDC hDC) {
    return ReleaseDC(hWnd, hDC);
}

// Delete a handle to a logical pen, brush, font, bitmap, region, or palette.
BOOL delete_object(HGDIOBJ ho) { 
    return DeleteObject(ho);
}

// Get DC from HWND
HDC get_dc(HWND window_handle) { 
    return GetDC(window_handle);
}

// Output a string for debugging
void output_debug_string(const char *output_string) {
    OutputDebugStringA(output_string);
}

// Create a message box
int message_box(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType) {
    return MessageBox(hWnd, lpText, lpCaption, uType);
}

HMODULE get_module_handle(const char *module_name=NULL) {
    return GetModuleHandleA(module_name);
}

WNDCLASS create_window_class(UINT style,  WNDPROC lpfnWndProc, int cbClsExtra,       int cbWndExtra,      HINSTANCE hInstance,
                             HICON hIcon, HCURSOR hCursor,     HBRUSH hbrBackground, LPCSTR lpszMenuName, LPCSTR lpszClassName) { 
    WNDCLASS window_class      = {};
    window_class.style         = style;
    window_class.lpfnWndProc   = lpfnWndProc;
    window_class.cbClsExtra    = cbClsExtra; 
    window_class.cbWndExtra    = cbWndExtra;
    window_class.hInstance     = hInstance;
    window_class.hIcon         = hIcon;
    window_class.hCursor       = hCursor;
    window_class.hbrBackground = hbrBackground;
    window_class.lpszMenuName  = lpszMenuName;
    window_class.lpszClassName = lpszClassName;
    return window_class;
}

// Default processing for any windows messages
LRESULT default_window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) { 
    return DefWindowProc(window, message, w_param, l_param);
}

ATOM register_window_class(WNDCLASS *window_class) { 
    return RegisterClass(window_class);
}

HWND create_window(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, 
                   int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) { 
    return CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

// This is a blocking call to get window messages given a window handle.
BOOL get_message(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    return GetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

// This is a non-blocking call to get window messages given a window handle.
BOOL peek_message(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
    return PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

BOOL translate_and_dispatch_message(MSG *message) {
    BOOL result = TranslateMessage(message);
    DispatchMessage(message);
    return result;
}

// Signal that you are beginning to paint to the window.
HDC begin_paint(HWND window_handle, PAINTSTRUCT *paint) {
    return BeginPaint(window_handle, paint);
}

// Paint to the window.
BOOL pat_blt(HDC device_context, s32 x, s32 y, s32 width, s32 height, DWORD raster_operation) {
    return PatBlt(device_context, x, y, width, height, raster_operation);
}

// Signal that you are done painting to the window.
BOOL end_paint(HWND window_handle, PAINTSTRUCT *paint) {
    return EndPaint(window_handle, paint);
}

// Get Coordinates of window's client area.
BOOL get_client_rect(HWND window_handle, LPRECT lpRect) { 
    return GetClientRect(window_handle, lpRect);
}

// Create a backing-buffer bitmap
HBITMAP create_dib_section(HDC hdc, const BITMAPINFO *pbmi, UINT usage, VOID **ppvBits, HANDLE hSection, DWORD offset) {
    return CreateDIBSection(hdc, pbmi, usage, ppvBits, hSection, offset);
}

// Copy color data from given source DIB to destination rectangle.
int stretch_dib_bits(HDC hdc, int xDest, int yDest, int DestWidth, int DestHeight, int xSrc, int ySrc, int SrcWidth, int SrcHeight, const VOID *lpBits, const BITMAPINFO *lpbmi, UINT iUsage, DWORD rop) { 
     return StretchDIBits(hdc, xDest, yDest, DestWidth, DestHeight, xSrc, ySrc, SrcWidth, SrcHeight, lpBits, lpbmi, iUsage, rop);
}
