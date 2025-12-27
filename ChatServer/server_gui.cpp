#include "server_logic.h"
#include <windows.h>
#include <string>
#include <sstream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// Modern Color Scheme
#define COLOR_PRIMARY RGB(41, 128, 185)      // Modern Blue
#define COLOR_ACCENT RGB(52, 152, 219)       // Light Blue
#define COLOR_SUCCESS RGB(46, 204, 113)      // Green
#define COLOR_BG RGB(236, 240, 241)          // Light Gray
#define COLOR_WHITE RGB(255, 255, 255)       // White
#define COLOR_TEXT RGB(44, 62, 80)           // Dark Gray Text

// Global variables
ChatServer* server = nullptr;
HWND hwndLog = NULL;
HWND hwndClientCount = NULL;
HWND hwndServerIP = NULL;
HWND hwndStartBtn = NULL;
HWND hwndStopBtn = NULL;
HBRUSH hBrushBg = NULL;
HBRUSH hBrushWhite = NULL;
HFONT hFontTitle = NULL;
HFONT hFontNormal = NULL;

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Function to get local IP address
std::string GetLocalIPAddress() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        return "Unable to get IP";
    }

    struct addrinfo hints, * info;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, NULL, &hints, &info) != 0) {
        return "Unable to get IP";
    }

    char ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in* addr = (struct sockaddr_in*)info->ai_addr;
    inet_ntop(AF_INET, &(addr->sin_addr), ipstr, INET_ADDRSTRLEN);

    std::string result(ipstr);
    freeaddrinfo(info);

    return result;
}

// Helper function to add log
void AddLog(const std::string& message) {
    if (hwndLog) {
        int len = GetWindowTextLength(hwndLog);
        SendMessage(hwndLog, EM_SETSEL, len, len);

        SYSTEMTIME st;
        GetLocalTime(&st);
        char timestamp[64];
        sprintf_s(timestamp, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);

        std::string fullMessage = timestamp + message + "\r\n";
        SendMessage(hwndLog, EM_REPLACESEL, FALSE, (LPARAM)fullMessage.c_str());

        SendMessage(hwndLog, EM_LINESCROLL, 0, SendMessage(hwndLog, EM_GETLINECOUNT, 0, 0));
    }
}

// Helper function to update client count
void UpdateClientCount(int count) {
    if (hwndClientCount) {
        std::string text = "Connected Clients: " + std::to_string(count);
        SetWindowText(hwndClientCount, text.c_str());
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Create brushes for colors
    hBrushBg = CreateSolidBrush(COLOR_BG);
    hBrushWhite = CreateSolidBrush(COLOR_WHITE);

    // Create fonts
    hFontTitle = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    hFontNormal = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    // Register window class
    const char CLASS_NAME[] = "ChatServerWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = hBrushBg;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Chat Server - Port 12345",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 650, 550,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Title label
    HWND hwndTitle = CreateWindow(
        "STATIC",
        "Chat Server Control Panel",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, 15, 595, 35,
        hwnd,
        NULL,
        hInstance,
        NULL
    );
    SendMessage(hwndTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

    // Start button
    hwndStartBtn = CreateWindow(
        "BUTTON",
        "Start Server",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 65, 150, 40,
        hwnd,
        (HMENU)1,
        hInstance,
        NULL
    );
    SendMessage(hwndStartBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Stop button
    hwndStopBtn = CreateWindow(
        "BUTTON",
        "Stop Server",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        185, 65, 150, 40,
        hwnd,
        (HMENU)2,
        hInstance,
        NULL
    );
    SendMessage(hwndStopBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    EnableWindow(hwndStopBtn, FALSE);

    // Status panel background (white box for client count)
    HWND hwndStatusPanel = CreateWindowEx(
        0,
        "STATIC",
        "",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        350, 65, 265, 40,
        hwnd,
        (HMENU)100,
        hInstance,
        NULL
    );

    // Client count label
    hwndClientCount = CreateWindow(
        "STATIC",
        "Connected Clients: 0",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        355, 73, 255, 25,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    HFONT hFontBold = CreateFont(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    SendMessage(hwndClientCount, WM_SETFONT, (WPARAM)hFontBold, TRUE);

    // Server IP panel background (white box)
    HWND hwndIPPanel = CreateWindowEx(
        0,
        "STATIC",
        "",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        350, 115, 265, 35,
        hwnd,
        (HMENU)101,
        hInstance,
        NULL
    );

    // Server IP label
    hwndServerIP = CreateWindow(
        "STATIC",
        "Server IP: Getting...",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        355, 123, 255, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
    );
    SendMessage(hwndServerIP, WM_SETFONT, (WPARAM)hFontBold, TRUE);

    // Separator line
    CreateWindow(
        "STATIC",
        "",
        WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
        20, 165, 595, 2,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Log label
    HWND hwndLogLabel = CreateWindow(
        "STATIC",
        "Server Activity Log",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 175, 200, 25,
        hwnd,
        (HMENU)300,
        hInstance,
        NULL
    );
    SendMessage(hwndLogLabel, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Log text box
    hwndLog = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        20, 205, 595, 285,
        hwnd,
        (HMENU)200,
        hInstance,
        NULL
    );

    HFONT hLogFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    SendMessage(hwndLog, WM_SETFONT, (WPARAM)hLogFont, TRUE);

    // Initialize server
    server = new ChatServer(12345);
    if (!server->Initialize()) {
        MessageBox(hwnd, "Failed to initialize Winsock!", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    server->SetLogCallback(AddLog);
    server->SetClientCountCallback(UpdateClientCount);

    // Get and display local IP
    std::string localIP = GetLocalIPAddress();
    std::string ipText = "Server IP: " + localIP;
    SetWindowText(hwndServerIP, ipText.c_str());

    AddLog("==============================================");
    AddLog("            Chat Server");
    AddLog("==============================================");
    AddLog("Server initialized successfully.");
    AddLog("Local IP: " + localIP);
    AddLog("Click 'Start Server' to begin accepting connections.");
    AddLog("");

    ShowWindow(hwnd, nCmdShow);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    if (server) {
        server->Stop();
        delete server;
    }

    DeleteObject(hBrushBg);
    DeleteObject(hBrushWhite);
    DeleteObject(hFontTitle);
    DeleteObject(hFontNormal);
    DeleteObject(hFontBold);
    DeleteObject(hLogFont);

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CTLCOLOREDIT: {
        // Color the log text box (Black text on White background - like Client)
        if ((HWND)lParam == hwndLog) {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(0, 0, 0));        // Black text
            SetBkColor(hdcEdit, RGB(255, 255, 255));    // White background
            static HBRUSH hBrushLogWhite = CreateSolidBrush(RGB(255, 255, 255));
            return (INT_PTR)hBrushLogWhite;
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;

        // Color the title
        SetTextColor(hdcStatic, COLOR_PRIMARY);
        SetBkColor(hdcStatic, COLOR_BG);

        // White background for status panel
        if (GetDlgCtrlID(hwndStatic) == 100 || GetDlgCtrlID(hwndStatic) == 101) {
            return (INT_PTR)hBrushWhite;
        }

        // Color the client count (Blue)
        if (hwndStatic == hwndClientCount) {
            SetTextColor(hdcStatic, COLOR_PRIMARY);  // Blue instead of green
            SetBkColor(hdcStatic, COLOR_WHITE);
            return (INT_PTR)hBrushWhite;
        }

        // Color the server IP (Blue)
        if (hwndStatic == hwndServerIP) {
            SetTextColor(hdcStatic, COLOR_PRIMARY);  // Blue
            SetBkColor(hdcStatic, COLOR_WHITE);
            return (INT_PTR)hBrushWhite;
        }

        // Color "Server Activity Log" label (Black)
        if (GetDlgCtrlID(hwndStatic) == 300) {
            SetTextColor(hdcStatic, RGB(0, 0, 0));  // Black text
            SetBkColor(hdcStatic, COLOR_BG);
            return (INT_PTR)hBrushBg;
        }

        return (INT_PTR)hBrushBg;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // Start button
            if (server && !server->IsRunning()) {
                if (server->Start()) {
                    EnableWindow(hwndStartBtn, FALSE);
                    EnableWindow(hwndStopBtn, TRUE);
                    AddLog("Server started successfully!");
                }
            }
        }
        else if (LOWORD(wParam) == 2) { // Stop button
            if (server && server->IsRunning()) {
                server->Stop();
                EnableWindow(hwndStartBtn, TRUE);
                EnableWindow(hwndStopBtn, FALSE);
                AddLog("Server stopped.");
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        if (server && server->IsRunning()) {
            int result = MessageBox(hwnd,
                "Server is still running. Are you sure you want to exit?",
                "Confirm Exit",
                MB_YESNO | MB_ICONQUESTION);
            if (result == IDNO) {
                return 0;
            }
        }
        DestroyWindow(hwnd);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}