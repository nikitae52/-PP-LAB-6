#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#define ID_CLIENT_CALC_SEND 32771
#define IDM_EXIT            203

HINSTANCE hInst;
static HWND hwndInputA;
static HWND hwndInputB;
static HWND hwndEdit;
char szBuf[1024];

HMENU CreateClient3Menu() {
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hClientMenu = CreatePopupMenu();
    AppendMenuA(hFileMenu, MF_STRING, IDM_EXIT, "Exit");
    AppendMenuA(hClientMenu, MF_STRING, ID_CLIENT_CALC_SEND, "Calculate & Send");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hClientMenu, "Client");
    return hMenuBar;
}

void UpdateCalculationText() {
    char strA[64] = "", strB[64] = "";
    GetWindowTextA(hwndInputA, strA, sizeof(strA));
    GetWindowTextA(hwndInputB, strB, sizeof(strB));
    double a = atof(strA);
    double b = atof(strB);
    char mess[1024];
    sprintf_s(mess, sizeof(mess), "Дані Клієнта #3 (Бонус FileMapping):\r\n- ширина (a) = %.2f\r\n- висота (b) = %.2f\r\n- площа (S = a * b) = %.2f", a, b, a * b);
    SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)mess);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateWindowA("STATIC", "Сторона a:", WS_CHILD | WS_VISIBLE, 10, 10, 85, 25, hWnd, NULL, hInst, NULL);
        hwndInputA = CreateWindowA("EDIT", "15", WS_CHILD | WS_VISIBLE | WS_BORDER, 95, 8, 80, 25, hWnd, NULL, hInst, NULL);
        CreateWindowA("STATIC", "Сторона b:", WS_CHILD | WS_VISIBLE, 190, 10, 85, 25, hWnd, NULL, hInst, NULL);
        hwndInputB = CreateWindowA("EDIT", "24", WS_CHILD | WS_VISIBLE | WS_BORDER, 275, 8, 80, 25, hWnd, NULL, hInst, NULL);
        hwndEdit = CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | WS_BORDER, 0, 42, 440, 180, hWnd, NULL, hInst, NULL);
        UpdateCalculationText();
        break;
    case WM_SIZE:
        MoveWindow(hwndEdit, 0, 42, LOWORD(lParam), HIWORD(lParam) - 42, TRUE);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_CLIENT_CALC_SEND: {
            UpdateCalculationText();
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
        case IDM_EXIT: DestroyWindow(hWnd); break;
        } break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProcA(hWnd, message, wParam, lParam);
    } return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSEXA wcex = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, "MapClient3Class", LoadIcon(NULL, IDI_APPLICATION) };
    RegisterClassExA(&wcex);
    HWND hWnd = CreateWindowA("MapClient3Class", "Client 3 - FileMapping", WS_OVERLAPPEDWINDOW, 100, 670, 460, 270, NULL, CreateClient3Menu(), hInstance, NULL);
    ShowWindow(hWnd, nCmdShow); UpdateWindow(hWnd);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); } return (int)msg.wParam;
}