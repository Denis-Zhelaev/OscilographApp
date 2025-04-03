#include <windows.h>
#include <commdlg.h>
#include <fstream>
#include <string>
#include <shlobj.h>

// Глобальные переменные
HWND g_hLogEdit;       // Поле для лога
HWND g_hMainWindow;    // Главное окно
const wchar_t* g_logFileName = L"log.txt"; // Имя файла лога

// Объявления функций
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void RegisterWindowClass(HINSTANCE hInstance);
HWND CreateMainWindow(HINSTANCE hInstance);
void AddToLog(const wchar_t* message);
void SaveLogToFile();
std::wstring GetExecutableDirectory();

// Главная функция
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
    // Создание основного окна
    HWND hWnd = CreateWindowExW(
        0,
        L"MyWindowClass",
        L"Simple WinAPI App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return nullptr;

    // Создание меню
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hConnectionMenu = CreatePopupMenu();

    // Меню File
    AppendMenuW(hFileMenu, MF_STRING, 3, L"Save");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");

    // Меню Connection
    AppendMenuW(hConnectionMenu, MF_STRING, 4, L"Connect RS-232");
    AppendMenuW(hConnectionMenu, MF_STRING, 5, L"Connect USB");
    AppendMenuW(hConnectionMenu, MF_STRING, 6, L"Connect Ethernet");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hConnectionMenu, L"Connection");

    SetMenu(hWnd, hMenu);

    // Создание поля для лога
    g_hLogEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        10, 400, 760, 150,
        hWnd,
        (HMENU)101,
        hInstance,
        nullptr);

    // Установка шрифта
    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    SendMessageW(g_hLogEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

    return hWnd;
}

// Добавление сообщения в лог
void AddToLog(const wchar_t* message)
{
    // Получение текущего времени
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeStr[64];
    swprintf_s(timeStr, L"[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);

    // Формирование полного сообщения
    wchar_t fullMessage[1024];
    swprintf_s(fullMessage, L"%s%s\n", timeStr, message);

    // Добавление в edit control
    int len = GetWindowTextLengthW(g_hLogEdit);
    SendMessageW(g_hLogEdit, EM_SETSEL, len, len);
    SendMessageW(g_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)fullMessage);
}

// Получение директории исполняемого файла
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
    // Генерация имени файла с датой
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t szFileName[MAX_PATH];
    swprintf_s(szFileName, L"log_%04d-%02d-%02d.txt", st.wYear, st.wMonth, st.wDay);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hMainWindow;
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"txt";

    // Установка начальной директории
    std::wstring initialDir = GetExecutableDirectory();
    ofn.lpstrInitialDir = initialDir.c_str();

    if (GetSaveFileNameW(&ofn))
    {
        // Получение текста из edit control
        int len = GetWindowTextLengthW(g_hLogEdit) + 1;
        wchar_t* buf = new wchar_t[len];
        GetWindowTextW(g_hLogEdit, buf, len);
        
        // Обработка текста
        std::wstring logText(buf);
        
        // Заменяем все последовательности \r\n на \n
        size_t pos = 0;
        while ((pos = logText.find(L"\r\n", pos)) != std::wstring::npos) {
            logText.replace(pos, 2, L"\n");
        }
        
        // Удаляем дублирующиеся переносы строк
        pos = 0;
        while ((pos = logText.find(L"\n\n", pos)) != std::wstring::npos) {
            logText.replace(pos, 2, L"\n");
        }
        
        // Удаляем завершающий перенос строки
        if (!logText.empty() && logText.back() == L'\n') {
            logText.pop_back();
        }

        // Запись в файл
        std::wofstream outFile(ofn.lpstrFile);
        if (outFile.is_open())
        {
            outFile << logText;
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
        case 3: // Save
            SaveLogToFile();
            break;
        case 4: // Connect RS-232
            AddToLog(L"RS-232 connection selected");
            break;
        case 5: // Connect USB
            AddToLog(L"USB connection selected");
            break;
        case 6: // Connect Ethernet
            AddToLog(L"Ethernet connection selected");
            break;
        }
    }
    break;

    case WM_CLOSE:
        // Очистка лога перед закрытием
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
