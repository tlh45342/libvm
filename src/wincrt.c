#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "arm-vm.h"
#include "wincrt.h"

static BOOL          g_enabled = FALSE;
static HWND          g_hwnd    = NULL;
static HDC           g_hdc     = NULL;
static int           g_cols    = 0;
static int           g_rows    = 0;
static int           g_cw      = 8;   // cell width
static int           g_ch      = 16;  // cell height
static const uint8_t *g_vram   = NULL;
static volatile BOOL g_dirty   = FALSE;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 100, NULL); // periodic refresh
        return 0;

    case WM_TIMER:
        if (g_dirty) {
            InvalidateRect(hwnd, NULL, FALSE);
            g_dirty = FALSE;
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HFONT hFont = (HFONT)GetStockObject(OEM_FIXED_FONT);
        SelectObject(hdc, hFont);

        // green on black like before
        SetTextColor(hdc, RGB(0,255,0));
        SetBkColor(hdc,   RGB(0,0,0));
        SetBkMode(hdc, OPAQUE);

        RECT client; GetClientRect(hwnd, &client);
        HBRUSH black = CreateSolidBrush(RGB(0,0,0));
        FillRect(hdc, &client, black);
        DeleteObject(black);

        if (g_vram) {
            for (int row = 0; row < g_rows; ++row) {
                for (int col = 0; col < g_cols; ++col) {
                    char ch = g_vram[(row * g_cols + col) * 2];
                    if (ch >= 32 && ch <= 126) {
                        TextOutA(hdc, col * g_cw, row * g_ch, &ch, 1);
                    }
                }
            }
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void wincrt_init_text(int cols, int rows) {
    g_cols = cols; g_rows = rows;

    // measure OEM fixed font for cell size (fallback to 8x16 if it fails)
    HDC screen = GetDC(NULL);
    HFONT f = (HFONT)GetStockObject(OEM_FIXED_FONT);
    HFONT old = (HFONT)SelectObject(screen, f);
    TEXTMETRICA tm;
    if (GetTextMetricsA(screen, &tm)) {
        g_cw = tm.tmAveCharWidth;
        g_ch = tm.tmHeight;
    }
    SelectObject(screen, old);
    ReleaseDC(NULL, screen);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = "WinCRTClass";
    RegisterClassA(&wc);

    RECT rect = {0, 0, g_cols * g_cw, g_rows * g_ch};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    g_hwnd = CreateWindowA("WinCRTClass", "ARM CRT", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           rect.right - rect.left,
                           rect.bottom - rect.top,
                           NULL, NULL, wc.hInstance, NULL);
    ShowWindow(g_hwnd, SW_SHOWNORMAL);
    g_hdc = GetDC(g_hwnd);
    g_enabled = TRUE;
}

void wincrt_pump_messages(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void wincrt_present_text(const uint8_t *vram, int x, int y, int w, int h) {
    (void)x; (void)y; (void)w; (void)h;
    if (!g_enabled || !g_hdc) return;
    g_vram = vram;           // remember source for WM_PAINT
    g_dirty = TRUE;          // schedule repaint on timer or next WM_PAINT
    InvalidateRect(g_hwnd, NULL, FALSE);
    UpdateWindow(g_hwnd);    // draw now if not in a tight loop
}

void wincrt_shutdown(void) {
    if (g_hwnd && g_hdc) {
        ReleaseDC(g_hwnd, g_hdc);
    }
    if (g_hwnd) {
        DestroyWindow(g_hwnd);
    }
    g_hwnd = NULL;
    g_hdc = NULL;
    g_enabled = FALSE;
}

