// Ctrl+V digita o conteudo da area de transferencia.
// Sair: menu do icone na bandeja.
#include <windows.h>
#include <shellapi.h>
#include "resource.h"

#define WM_TRAY (WM_APP + 1)
#define ID_TRAY 1
#define ID_EXIT 1

static void type_char(wchar_t c) {
    INPUT in[2] = {0};
    in[0].type = in[1].type = INPUT_KEYBOARD;
    in[0].ki.wScan = in[1].ki.wScan = c;
    in[0].ki.dwFlags = KEYEVENTF_UNICODE;
    in[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    SendInput(2, in, sizeof(INPUT));
    Sleep(10);
}

static void type_clipboard(HWND window) {
    // Espera soltar o Ctrl para evitar atalhos durante a digitacao.
    while (GetAsyncKeyState(VK_CONTROL) & 0x8000) Sleep(10);

    if (!OpenClipboard(window)) return;
    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    wchar_t *text = data ? GlobalLock(data) : NULL;
    if (text) {
        for (wchar_t *p = text; *p; p++)
            if (*p != L'\n') type_char(*p); // \r\n -> apenas Enter.
        GlobalUnlock(data);
    }
    CloseClipboard();
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        if (!RegisterHotKey(window, 1, MOD_CONTROL | MOD_NOREPEAT, 'V')) return -1;

        NOTIFYICONDATAW icon = {0};
        icon.cbSize = sizeof(icon);
        icon.hWnd = window;
        icon.uID = ID_TRAY;
        icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        icon.uCallbackMessage = WM_TRAY;
        icon.hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL),
            MAKEINTRESOURCEW(IDI_AUTOWRITE), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
        lstrcpyW(icon.szTip, L"AutoWrite");
        if (!Shell_NotifyIconW(NIM_ADD, &icon)) {
            UnregisterHotKey(window, 1);
            return -1;
        }
        return 0;
    }
    case WM_HOTKEY:
        type_clipboard(window);
        return 0;
    case WM_TRAY:
        if (lparam == WM_RBUTTONUP) {
            POINT position;
            GetCursorPos(&position);
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, ID_EXIT, L"Sair");
            SetForegroundWindow(window);
            UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                position.x, position.y, 0, window, NULL);
            DestroyMenu(menu);
            PostMessageW(window, WM_NULL, 0, 0);
            if (command == ID_EXIT) DestroyWindow(window);
        }
        return 0;
    case WM_DESTROY: {
        NOTIFYICONDATAW icon = {0};
        icon.cbSize = sizeof(icon);
        icon.hWnd = window;
        icon.uID = ID_TRAY;
        Shell_NotifyIconW(NIM_DELETE, &icon);
        UnregisterHotKey(window, 1);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show) {
    UNREFERENCED_PARAMETER(previous);
    UNREFERENCED_PARAMETER(command_line);
    UNREFERENCED_PARAMETER(show);

    WNDCLASSW window_class = {0};
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"AutoWriteTray";
    if (!RegisterClassW(&window_class)) return 1;
    if (!CreateWindowExW(0, window_class.lpszClassName, L"AutoWrite", 0,
        0, 0, 0, 0, NULL, NULL, instance, NULL)) return 1;

    MSG message;
    BOOL result;
    while ((result = GetMessageW(&message, NULL, 0, 0)) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return result == -1 ? 1 : (int)message.wParam;
}
