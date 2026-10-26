#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <string>

#define ID_FILEMAPPING_START 32771
#define ID_FILEMAPPING_STOP  32772
#define IDM_EXIT             107
#define WM_USER_NEW_DATA     (WM_USER + 1)

HINSTANCE hInst;
static HWND hwndEdit;
char mess[4096] = "";
char* m_mess = mess;

HANDLE hMapFile = NULL;
HANDLE hEvent = NULL;
HANDLE hMutex = NULL;
HANDLE hThread = NULL;
BOOL bRunning = FALSE;

// Фоновий потік для відстеження змін (Завдання "з зірочкою")
DWORD WINAPI WatcherThread(LPVOID lpParam) {
    HWND hWnd = (HWND)lpParam;
    while (bRunning) {
        // Очікуємо сигналу від клієнта (таймаут 500мс для перевірки прапорця виходу)
        DWORD dwWait = WaitForSingleObject(hEvent, 500);
        if (dwWait == WAIT_OBJECT_0) {
            // Дані змінилися - сповіщаємо головне вікно
            PostMessageA(hWnd, WM_USER_NEW_DATA, 0, 0);
        }
    }
    return 0;
}

HMENU CreateServerMenu() {
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hMapMenu = CreatePopupMenu();

    AppendMenuA(hFileMenu, MF_STRING, IDM_EXIT, "Exit");
    AppendMenuA(hMapMenu, MF_STRING, ID_FILEMAPPING_START, "Start");
    AppendMenuA(hMapMenu, MF_STRING, ID_FILEMAPPING_STOP, "Stop");

    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenuA(hMenuBar, MF_POPUP, (UINT_PTR)hMapMenu, "FileMapping");

    return hMenuBar;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        strcpy_s(mess, sizeof(mess), "Лабораторна робота - Відображення (проекція) файлу в пам'ять.\r\n");
        hwndEdit = CreateWindowA("EDIT", mess,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 400, 460, hWnd, NULL, hInst, NULL);
        break;

    case WM_SIZE:
        MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_FILEMAPPING_START:
            if (hMapFile != NULL) {
                MessageBoxA(hWnd, "FileMapping вже запущено!", "Info", MB_OK | MB_ICONINFORMATION);
                break;
            }

            // Створення синхронізації та File Mapping
            hMutex = CreateMutexA(NULL, FALSE, "MyLab6Mutex");
            hEvent = CreateEventA(NULL, FALSE, FALSE, "MyLab6Event"); // Авто-скидання
            hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, "MyLab6FileMapping");

            if (hMapFile == NULL || hMapFile == INVALID_HANDLE_VALUE) {
                MessageBoxA(hWnd, "Не можливо відкрити відображений у пам'яті об'єкт", "Помилка", MB_OK);
                break;
            }

            bRunning = TRUE;
            hThread = CreateThread(NULL, 0, WatcherThread, hWnd, 0, NULL);

            strcat_s(mess, sizeof(mess), "\r\nСервер FileMapping запущено\r\n");
            SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
            break;

        case ID_FILEMAPPING_STOP:
            if (hMapFile != NULL) {
                bRunning = FALSE;
                if (hThread) { WaitForSingleObject(hThread, 1000); CloseHandle(hThread); hThread = NULL; }
                CloseHandle(hMapFile); hMapFile = NULL;
                CloseHandle(hEvent); hEvent = NULL;
                CloseHandle(hMutex); hMutex = NULL;

                strcat_s(mess, sizeof(mess), "\r\nСервер FileMapping зупинено\r\n");
                SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
            }
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProcA(hWnd, message, wParam, lParam);
        }
    } break;

    case WM_USER_NEW_DATA: {
        // Автоматичне читання при сигналі події
        if (hMapFile && hMutex) {
            WaitForSingleObject(hMutex, INFINITE); // Блокуємо доступ іншим процесам під час читання
            LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024);
            if (pBuf != NULL) {
                strcat_s(mess, sizeof(mess), "\r\n");
                strcat_s(mess, sizeof(mess), (char*)pBuf);
                strcat_s(mess, sizeof(mess), "\r\n");
                SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM)m_mess);
                UnmapViewOfFile(pBuf);
            }
            ReleaseMutex(hMutex); // Відпускаємо м'ютекс
        }
    } break;

    case WM_DESTROY:
        bRunning = FALSE;
        if (hThread) { WaitForSingleObject(hThread, 1000); CloseHandle(hThread); }
        if (hMapFile) CloseHandle(hMapFile);
        if (hEvent) CloseHandle(hEvent);
        if (hMutex) CloseHandle(hMutex);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    const char* szClassName = "FileMapServerClass";
    WNDCLASSEXA wcex = { sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, szClassName, LoadIcon(NULL, IDI_APPLICATION) };
    if (!RegisterClassExA(&wcex)) return FALSE;
    HWND hWnd = CreateWindowA(szClassName, "Server - FileMapping (Variant 1)", WS_OVERLAPPEDWINDOW, 100, 100, 460, 560, NULL, CreateServerMenu(), hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return (int)msg.wParam;
}