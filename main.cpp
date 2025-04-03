#include <windows.h>
#include <commdlg.h>
#include <fstream>
#include <string>
#include <shlobj.h>

// Глобальные переменные
HWND g_hLogEdit;       // Окно для лога
HWND g_hMainWindow;    // Главное окно
const wchar_t* g_logFileName = L"log.txt"; // Имя файла лога

// Прототипы функций
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void RegisterWindowClass(HINSTANCE hInstance);
HWND CreateMainWindow(HINSTANCE hInstance);
void AddToLog(const wchar_t* message);
void SaveLogToFile();
std::wstring GetExecutableDirectory();

// Точка входа
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    RegisterWindowClass(hInstance);
    g_hMainWindow = CreateMainWindow(hInstance);

    if (!g_hMainWindow) return 1;

    ShowWindow(g_hMainWindow, nCmdShow);
    UpdateWindow(g_hMainWindow);

    // Инициализация лога
    AddToLog(L"Application started");

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Регистрация класса окна
void RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"MyWindowClass";
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClassExW(&wcex);
}

// Создание главного окна
HWND CreateMainWindow(HINSTANCE hInstance)
{
    // Увеличиваем размер главного окна
    HWND hWnd = CreateWindowExW(
        0,
        L"MyWindowClass",
        L"Simple WinAPI App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, // Увеличенный размер
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return nullptr;

    // Создаем меню
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();

    AppendMenuW(hSubMenu, MF_STRING, 1, L"Check");
    AppendMenuW(hSubMenu, MF_STRING, 2, L"Translate");
    AppendMenuW(hSubMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hSubMenu, MF_STRING, 3, L"Save");

    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, L"Menu");
    SetMenu(hWnd, hMenu);

    // Создаем окно для лога (занимает 1/4 высоты окна)
    g_hLogEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        10, 400, 760, 150, // Позиция и размер (1/4 высоты окна)
        hWnd,
        (HMENU)101,
        hInstance,
        nullptr);

    // Устанавливаем шрифт
    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    SendMessageW(g_hLogEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

    return hWnd;
}

// Добавление сообщения в лог
void AddToLog(const wchar_t* message)
{
    // Получаем текущее время
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeStr[64];
    swprintf_s(timeStr, L"[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);

    // Добавляем время к сообщению
    wchar_t fullMessage[1024];
    swprintf_s(fullMessage, L"%s%s\r\n", timeStr, message);

    // Добавляем в edit control
    int len = GetWindowTextLengthW(g_hLogEdit);
    SendMessageW(g_hLogEdit, EM_SETSEL, len, len);
    SendMessageW(g_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)fullMessage);
}

// Получение директории, где находится исполняемый файл
std::wstring GetExecutableDirectory()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring wsPath(path);
    size_t pos = wsPath.find_last_of(L"\\/");
    return wsPath.substr(0, pos);
}

// Сохранение лога в файл
void SaveLogToFile()
{
    OPENFILENAMEW ofn;
    wchar_t szFileName[MAX_PATH] = L"log.txt";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hMainWindow;
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"txt";

    // Устанавливаем начальную директорию
    std::wstring initialDir = GetExecutableDirectory();
    ofn.lpstrInitialDir = initialDir.c_str();

    if (GetSaveFileNameW(&ofn))
    {
        // Получаем текст из edit control
        int len = GetWindowTextLengthW(g_hLogEdit) + 1;
        wchar_t* buf = new wchar_t[len];
        GetWindowTextW(g_hLogEdit, buf, len);

        // Записываем в файл
        std::wofstream outFile(ofn.lpstrFile);
        if (outFile.is_open())
        {
            outFile << buf;
            outFile.close();
            AddToLog(L"Log saved successfully");
        }
        else
        {
            AddToLog(L"Error saving log file");
        }

        delete[] buf;
    }
}

// Оконная процедура
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case 1: // Check
            AddToLog(L"Check button clicked");
            break;
        case 2: // Translate
            AddToLog(L"Translate button clicked");
            break;
        case 3: // Save
            SaveLogToFile();
            break;
        }
    }
    break;

    case WM_CLOSE:
        // Очищаем лог при закрытии
        SetWindowTextW(g_hLogEdit, L"");
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}