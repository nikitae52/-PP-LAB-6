#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <string>

#define ID_CLIENT_SEND    32771
#define ID_CLIENT_DEFAULT 32772
#define IDM_EXIT          203

HINSTANCE hInst;
static HWND hwndEdit;
char szBuf[1024];

HMENU CreateClientMenu() {
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hClientMenu = CreatePopupMenu();
    AppendMenuA(hFileMenu, MF_STRING, IDM_EXIT, "Exit");
    AppendMenuA(hClientMenu, MF_STRING, ID_CLIENT_SEND, "Send Message");
    AppendMenuA(hClientMenu, MF_STRING, ID_CLIENT_DEFAULT, "Restore Default");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hClientMenu, "Client");
    return hMenuBar;
}

void SetDefaultMetrics(HWND hWnd) {
    // В Windows немає прямої метрики для міжрядкового інтервалу, використовуємо висоту шрифту меню/тексту як аналог
    int lineSpacing = 0; // Або GetSystemMetrics(SM_CYMENU)
    int iconWidth = GetSystemMetrics(SM_CXICON);
    int iconHeight = GetSystemMetrics(SM_CYICON);

    char mess[1024];
    sprintf_s(mess, sizeof(mess), "Дані Клієнта #2:\r\n- міжрядковий інтервал = %d\r\n- ширина піктограми = %d\r\n- висота піктограми = %d",
        lineSpacing, iconWidth, iconHeight);
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        hwndEdit = CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 0, 0, 400, 200, hWnd, NULL, hInst, NULL);
        SetDefaultMetrics(hWnd);
        break;
    case WM_SIZE:
        MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_CLIENT_SEND: {
            DWORD cbWritten = SendMessageA(hwndEdit, WM_GETTEXTLENGTH, 0, 0);
            SendMessageA(hwndEdit, WM_GETTEXT, cbWritten + 1, (LPARAM)szBuf);

            HANDLE hMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, "MyLab6Mutex");
            HANDLE hEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, "MyLab6Event");
            HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "MyLab6FileMapping");

            if (!hMapFile || !hMutex || !hEvent) {
                MessageBoxA(hWnd, "Сервер FileMapping не запущено!", "Помилка", MB_OK | MB_ICONERROR);
                if (hMapFile) CloseHandle(hMapFile);
                if (hMutex) CloseHandle(hMutex);
                if (hEvent) CloseHandle(hEvent);
                break;
            }

            WaitForSingleObject(hMutex, INFINITE);
            LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024);
            if (pBuf) {
                CopyMemory((PVOID)pBuf, szBuf, strlen(szBuf) + 1);
                UnmapViewOfFile(pBuf);
            }
            ReleaseMutex(hMutex);
            SetEvent(hEvent);

            CloseHandle(hMapFile);
            CloseHandle(hMutex);
            CloseHandle(hEvent);
        } break;
        case ID_CLIENT_DEFAULT: SetDefaultMetrics(hWnd); break;
        case IDM_EXIT: DestroyWindow(hWnd); break;
        } break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProcA(hWnd, message, wParam, lParam);
    } return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSEXA wcex = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, "MapClient2Class", LoadIcon(NULL, IDI_APPLICATION) };
    RegisterClassExA(&wcex);
    HWND hWnd = CreateWindowA("MapClient2Class", "Client 2 - FileMapping", WS_OVERLAPPEDWINDOW, 580, 390, 460, 270, NULL, CreateClientMenu(), hInstance, NULL);
    ShowWindow(hWnd, nCmdShow); UpdateWindow(hWnd);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); } return (int)msg.wParam;
}