#include "framework.h"
#include "lab1.h"
#include <windows.h>
#include <psapi.h>
#include <string>
#include <vector>  // Для хранения списка процессов
#include <commdlg.h>  // Для работы с диалогом открытия файла

#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

// Структура для хранения информации о процессе
struct ProcessInfo {
    DWORD processID;
    std::wstring processName;
};

// Глобальная переменная для ListBox
HWND hListBox;

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// Обработчики для меню:
void ShowProcessList(HWND hWnd);
void TerminateProcess(HWND hWnd);
void StartProcess(HWND hWnd);
void LaunchProcessFromPath(HWND hWnd, const std::wstring& path);
void ChooseProcessToTerminate(HWND hWnd, std::vector<ProcessInfo>& processes);
void TerminateChosenProcess(HWND hWnd, const ProcessInfo& selectedProcess);
INT_PTR CALLBACK About(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hWnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LAB1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB1));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LAB1);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    // Создание ListBox для отображения процессов
    hListBox = CreateWindowW(L"LISTBOX", NULL,
        WS_CHILD | WS_VISIBLE | LBS_STANDARD,
        50, 50, 400, 200, hWnd, (HMENU)1001, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Разобрать выбор в меню:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case ID_PROCESS_LIST:
            ShowProcessList(hWnd);
            break;
        case ID_TERMINATE_PROCESS:
            TerminateProcess(hWnd);
            break;
        case ID_START_PROCESS:
            StartProcess(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Добавьте сюда любой код прорисовки, использующий HDC...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчики для меню
void ShowProcessList(HWND hWnd) {
    DWORD processList[1024], cbNeeded, processCount;
    HWND hListBox = GetDlgItem(hWnd, ID_LISTBOX_PROCESS); // Предполагаем, что ListBox имеет ID_LISTBOX_PROCESS

    if (!EnumProcesses(processList, sizeof(processList), &cbNeeded)) {
        MessageBox(hWnd, L"Не удалось получить список процессов", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    processCount = cbNeeded / sizeof(DWORD);
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0); // Очистить ListBox

    for (unsigned int i = 0; i < processCount; i++) {
        DWORD processID = processList[i];

        // Открываем процесс
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
        if (hProcess != NULL) {
            TCHAR processName[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, NULL, processName, MAX_PATH) > 0) {
                // Добавляем процесс в ListBox
                std::wstring processInfo = std::to_wstring(processID) + L" - " + processName;
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)processInfo.c_str());
            }
            CloseHandle(hProcess);
        }
    }
}


void TerminateProcess(HWND hWnd) {
    HWND hListBox = GetDlgItem(hWnd, ID_LISTBOX_PROCESS);
    int selectedIndex = SendMessage(hListBox, LB_GETCURSEL, 0, 0);

    if (selectedIndex == LB_ERR) {
        MessageBox(hWnd, L"Не выбран процесс", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    // Получаем строку с ID процесса и именем
    WCHAR processInfo[MAX_PATH];
    SendMessage(hListBox, LB_GETTEXT, selectedIndex, (LPARAM)processInfo);

    // Извлекаем ID процесса из строки
    DWORD processID = _wtoi(processInfo);

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
    if (hProcess != NULL) {
        if (TerminateProcess(hProcess, 0)) {
            MessageBox(hWnd, L"Процесс завершен", L"Успех", MB_OK);
        }
        else {
            MessageBox(hWnd, L"Не удалось завершить процесс", L"Ошибка", MB_OK | MB_ICONERROR);
        }
        CloseHandle(hProcess);
    }
    else {
        MessageBox(hWnd, L"Не удалось открыть процесс для завершения", L"Ошибка", MB_OK | MB_ICONERROR);
    }
}


void StartProcess(HWND hWnd)
{
    OPENFILENAME ofn;       // Диалоговое окно
    wchar_t szFile[260];    // Строка для пути к файлу

    // Инициализация структуры OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"Все файлы\0*.*\0Исполняемые файлы\0*.exe\0";
    ofn.nFilterIndex = 2;
    ofn.lpstrFile[0] = '\0';
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = L"Выберите файл для запуска";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Показываем диалоговое окно выбора файла
    if (GetOpenFileName(&ofn) == TRUE) {
        // Если файл выбран, запускаем его
        LaunchProcessFromPath(hWnd, ofn.lpstrFile);
    }
}
void LaunchProcessFromPath(HWND hWnd, const std::wstring& path)
{
    // Запуск процесса
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcess(path.c_str(),   // Путь к файлу
        NULL,                       // Командная строка
        NULL,                       // Процесс безопасности
        NULL,                       // Поток безопасности
        FALSE,                      // Ненаследование дескрипторов
        0,                          // Флаги создания
        NULL,                       // Переменные среды
        NULL,                       // Рабочая директория
        &si,                        // Структура STARTUPINFO
        &pi)                        // Структура PROCESS_INFORMATION
        ) {
        MessageBox(hWnd, L"Процесс успешно запущен", L"Успех", MB_OK);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        MessageBox(hWnd, L"Не удалось запустить процесс", L"Ошибка", MB_OK | MB_ICONERROR);
    }
}
