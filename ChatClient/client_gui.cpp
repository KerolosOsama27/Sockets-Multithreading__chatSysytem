#include "client_logic.h"
#include <windows.h>
#include <string>

// Modern Color Scheme
#define COLOR_PRIMARY RGB(41, 128, 185)      // Modern Blue
#define COLOR_ACCENT RGB(52, 152, 219)       // Light Blue
#define COLOR_SUCCESS RGB(46, 204, 113)      // Green
#define COLOR_DISCONNECT RGB(231, 76, 60)    // Red
#define COLOR_BG RGB(236, 240, 241)          // Light Gray
#define COLOR_WHITE RGB(255, 255, 255)       // White
#define COLOR_TEXT RGB(44, 62, 80)           // Dark Gray Text

// Global variables
ChatClient* client = nullptr;
HWND hwndChatHistory = NULL;
HWND hwndMessageInput = NULL;
HWND hwndUsernameInput = NULL;
HWND hwndServerIPInput = NULL;
HWND hwndConnectBtn = NULL;
HWND hwndDisconnectBtn = NULL;
HWND hwndSendBtn = NULL;
HWND hwndStatusLabel = NULL;
HBRUSH hBrushBg = NULL;
HBRUSH hBrushWhite = NULL;
HFONT hFontTitle = NULL;
HFONT hFontNormal = NULL;

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Helper function to add message to chat
void AddMessage(const std::string& message) {
    if (hwndChatHistory) {
        int len = GetWindowTextLength(hwndChatHistory);
        SendMessage(hwndChatHistory, EM_SETSEL, len, len);

        SYSTEMTIME st;
        GetLocalTime(&st);
        char timestamp[64];
        sprintf_s(timestamp, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);

        std::string fullMessage = timestamp + message + "\r\n";
        SendMessage(hwndChatHistory, EM_REPLACESEL, FALSE, (LPARAM)fullMessage.c_str());

        SendMessage(hwndChatHistory, EM_LINESCROLL, 0, SendMessage(hwndChatHistory, EM_GETLINECOUNT, 0, 0));
    }
}

// Helper function to update connection status
void UpdateConnectionStatus(bool connected) {
    if (connected) {
        EnableWindow(hwndConnectBtn, FALSE);
        EnableWindow(hwndDisconnectBtn, TRUE);
        EnableWindow(hwndSendBtn, TRUE);
        EnableWindow(hwndUsernameInput, FALSE);
        EnableWindow(hwndServerIPInput, FALSE);  // Disable server IP input
        EnableWindow(hwndMessageInput, TRUE);
        SetWindowText(hwndStatusLabel, "Status: Connected");
    }
    else {
        EnableWindow(hwndConnectBtn, TRUE);
        EnableWindow(hwndDisconnectBtn, FALSE);
        EnableWindow(hwndSendBtn, FALSE);
        EnableWindow(hwndUsernameInput, TRUE);
        EnableWindow(hwndServerIPInput, TRUE);  // Enable server IP input
        EnableWindow(hwndMessageInput, FALSE);
        SetWindowText(hwndStatusLabel, "Status: Disconnected");
    }
}

// Helper function to send message
void SendMessageToServer() {
    if (!client || !client->IsConnected()) return;

    char buffer[1024];
    GetWindowText(hwndMessageInput, buffer, sizeof(buffer));

    std::string message(buffer);
    if (message.empty()) return;

    if (client->SendMessage(message)) {
        SetWindowText(hwndMessageInput, "");
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Create brushes and fonts
    hBrushBg = CreateSolidBrush(COLOR_BG);
    hBrushWhite = CreateSolidBrush(COLOR_WHITE);

    hFontTitle = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    hFontNormal = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    // Register window class
    const char CLASS_NAME[] = "ChatClientWindow";

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
        "Chat Client",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 650, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Title
    HWND hwndTitle = CreateWindow(
        "STATIC",
        "Chat Client",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, 15, 595, 35,
        hwnd,
        NULL,
        hInstance,
        NULL
    );
    SendMessage(hwndTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

    // Connection Panel Label
    CreateWindow(
        "STATIC",
        "Connection Settings",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 60, 200, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Username label
    CreateWindow(
        "STATIC",
        "Username:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 90, 90, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Username input
    hwndUsernameInput = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        115, 87, 160, 27,
        hwnd,
        NULL,
        hInstance,
        NULL
    );
    SetWindowText(hwndUsernameInput, "User");
    SendMessage(hwndUsernameInput, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Server IP label
    CreateWindow(
        "STATIC",
        "Server IP:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 125, 90, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Server IP input
    hwndServerIPInput = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        115, 122, 160, 27,
        hwnd,
        (HMENU)5,
        hInstance,
        NULL
    );
    SetWindowText(hwndServerIPInput, "127.0.0.1");
    SendMessage(hwndServerIPInput, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Connect button
    hwndConnectBtn = CreateWindow(
        "BUTTON",
        "Connect",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        290, 105, 120, 47,
        hwnd,
        (HMENU)1,
        hInstance,
        NULL
    );
    SendMessage(hwndConnectBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Disconnect button
    hwndDisconnectBtn = CreateWindow(
        "BUTTON",
        "Disconnect",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        420, 105, 120, 47,
        hwnd,
        (HMENU)2,
        hInstance,
        NULL
    );
    SendMessage(hwndDisconnectBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    EnableWindow(hwndDisconnectBtn, FALSE);

    // Status label
    hwndStatusLabel = CreateWindow(
        "STATIC",
        "Status: Disconnected",
        WS_VISIBLE | WS_CHILD | SS_RIGHT,
        410, 60, 205, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
    );
    HFONT hStatusFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    SendMessage(hwndStatusLabel, WM_SETFONT, (WPARAM)hStatusFont, TRUE);

    // Separator
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

    // Chat history label
    CreateWindow(
        "STATIC",
        "Chat Messages",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 175, 200, 25,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Chat history
    hwndChatHistory = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        20, 205, 595, 275,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    HFONT hChatFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    SendMessage(hwndChatHistory, WM_SETFONT, (WPARAM)hChatFont, TRUE);

    // Message input label
    CreateWindow(
        "STATIC",
        "Type your message:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 490, 200, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
    );

    // Message input
    hwndMessageInput = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        20, 515, 465, 30,
        hwnd,
        (HMENU)4,
        hInstance,
        NULL
    );
    SendMessage(hwndMessageInput, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    EnableWindow(hwndMessageInput, FALSE);

    // Send button
    hwndSendBtn = CreateWindow(
        "BUTTON",
        "Send",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        495, 512, 120, 36,
        hwnd,
        (HMENU)3,
        hInstance,
        NULL
    );
    SendMessage(hwndSendBtn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    EnableWindow(hwndSendBtn, FALSE);

    // Initialize client
    client = new ChatClient("127.0.0.1", 12345);
    if (!client->Initialize()) {
        MessageBox(hwnd, "Failed to initialize Winsock!", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    client->SetMessageCallback(AddMessage);
    client->SetConnectionCallback(UpdateConnectionStatus);

    AddMessage("==============================================");
    AddMessage("      Welcome to Chat System!");
    AddMessage("==============================================");
    AddMessage("Enter your username and server IP and click Connect to start.");
    AddMessage("");

    ShowWindow(hwnd, nCmdShow);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    if (client) {
        client->Disconnect();
        delete client;
    }

    DeleteObject(hBrushBg);
    DeleteObject(hBrushWhite);
    DeleteObject(hFontTitle);
    DeleteObject(hFontNormal);
    DeleteObject(hStatusFont);
    DeleteObject(hChatFont);

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;

        SetBkColor(hdcStatic, COLOR_BG);

        // Color title
        if (hwndStatic == GetDlgItem(hwnd, 0)) {
            SetTextColor(hdcStatic, COLOR_PRIMARY);
        }

        // Color status label
        if (hwndStatic == hwndStatusLabel) {
            if (client && client->IsConnected()) {
                SetTextColor(hdcStatic, COLOR_SUCCESS);
            }
            else {
                SetTextColor(hdcStatic, COLOR_DISCONNECT);
            }
        }

        return (INT_PTR)hBrushBg;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // Connect button
            if (client && !client->IsConnected()) {
                char username[256];
                GetWindowText(hwndUsernameInput, username, sizeof(username));

                if (strlen(username) == 0) {
                    AddMessage("[ERROR] Please enter a username!");
                    return 0;
                }

                // Get server IP
                char serverIP[256];
                GetWindowText(hwndServerIPInput, serverIP, sizeof(serverIP));

                if (strlen(serverIP) == 0) {
                    AddMessage("[ERROR] Please enter server IP address!");
                    return 0;
                }

                // Validate IP format (basic check)
                bool validIP = false;
                int dots = 0;
                for (int i = 0; serverIP[i] != '\0'; i++) {
                    if (serverIP[i] == '.') dots++;
                    else if (!isdigit(serverIP[i]) && serverIP[i] != '.') {
                        break;
                    }
                }
                validIP = (dots == 3);

                if (!validIP) {
                    AddMessage("[ERROR] Invalid IP address format!");
                    AddMessage("Example: 192.168.1.100");
                    return 0;
                }

                // Delete old client and create new one with updated IP
                if (client) {
                    delete client;
                }

                client = new ChatClient(serverIP, 12345);
                if (!client->Initialize()) {
                    AddMessage("[ERROR] Failed to initialize!");
                    return 0;
                }

                client->SetMessageCallback(AddMessage);
                client->SetConnectionCallback(UpdateConnectionStatus);
                client->SetUsername(username);

                AddMessage("Connecting to " + std::string(serverIP) + ":12345...");

                if (!client->Connect()) {
                    AddMessage("[CONNECTION FAILED]");
                    AddMessage("Could not connect to " + std::string(serverIP) + ":12345");
                    AddMessage("");
                    AddMessage("Please check:");
                    AddMessage("- Server is running");
                    AddMessage("- IP address is correct");
                    AddMessage("");
                }
            }
        }
        else if (LOWORD(wParam) == 2) { // Disconnect button
            if (client && client->IsConnected()) {
                client->Disconnect();
            }
        }
        else if (LOWORD(wParam) == 3) { // Send button
            SendMessageToServer();
        }
        else if (LOWORD(wParam) == 4 && HIWORD(wParam) == EN_CHANGE) {
            char buffer[2];
            GetWindowText(hwndMessageInput, buffer, sizeof(buffer));
            EnableWindow(hwndSendBtn, client && client->IsConnected() && strlen(buffer) > 0);
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_RETURN && GetFocus() == hwndMessageInput) {
            SendMessageToServer();
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        if (client && client->IsConnected()) {
            int result = MessageBox(hwnd,
                "You are still connected. Are you sure you want to exit?",
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