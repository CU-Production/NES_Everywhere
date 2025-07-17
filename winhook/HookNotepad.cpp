#define UNICODE
#include <windows.h>
#include <stdio.h>
#pragma comment(lib, "msimg32.lib")  // for AlphaBlend

void DrawNoiseOverlay(HWND hwnd);

// 全局变量
HHOOK g_hHook = NULL;
HBITMAP g_hNoiseBitmap = NULL;
HWND g_hTargetWindow = NULL;
HWND g_hNotepadMain = NULL;
UINT_PTR g_timerId = 0;
int g_noiseOffset = 0;
bool g_needsRedraw = false;
HMODULE g_hModule = NULL;

// 保存原始窗口过程
WNDPROC g_originalWndProc = NULL;

// 自定义窗口过程 - 直接拦截消息
LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // 处理WM_TIMER消息 - 这是动画的关键
    if (uMsg == WM_TIMER && wParam == 1) {
        g_noiseOffset = (g_noiseOffset + 2) % 50;
        
        // 输出调试信息确认定时器工作
        wchar_t debugMsg[100];
        swprintf_s(debugMsg, L"WM_TIMER processed: offset = %d", g_noiseOffset);
        OutputDebugString(debugMsg);
        
        // 触发重绘
        InvalidateRect(hwnd, NULL, FALSE);
        return 0; // 消费这个定时器消息
    }
    
    // 处理WM_PAINT消息
    if (uMsg == WM_PAINT) {
        // 先让原始的WM_PAINT处理
        LRESULT result = CallWindowProc(g_originalWndProc, hwnd, uMsg, wParam, lParam);
        
        // 然后立即绘制我们的效果
        DrawNoiseOverlay(hwnd);
        
        // 输出当前偏移值确认动画
        wchar_t debugMsg[100];
        swprintf_s(debugMsg, L"WM_PAINT processed, current offset: %d", g_noiseOffset);
        OutputDebugString(debugMsg);
        
        return result;
    }
    
    // 处理其他可能影响绘制的消息
    if (uMsg == WM_ERASEBKGND) {
        LRESULT result = CallWindowProc(g_originalWndProc, hwnd, uMsg, wParam, lParam);
        DrawNoiseOverlay(hwnd);
        return result;
    }
    
    // 自定义消息
    if (uMsg == WM_USER + 100) {
        DrawNoiseOverlay(hwnd);
        OutputDebugString(L"Custom message WM_USER+100 processed");
        return 0;
    }
    
    // 其他消息正常处理
    return CallWindowProc(g_originalWndProc, hwnd, uMsg, wParam, lParam);
}

// 生成简单的噪声纹理
HBITMAP CreateSimpleNoiseTexture(int width, int height) {
    HDC hScreenDC = GetDC(NULL);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HDC hdc = CreateCompatibleDC(hScreenDC);
    ReleaseDC(NULL, hScreenDC);
    
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);

    // 生成简单的噪声图案
    srand(GetTickCount());
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            BYTE gray = rand() % 64 + 128; // 较亮的灰色噪声
            SetPixel(hdc, x, y, RGB(gray, gray, gray));
        }
    }
    
    SelectObject(hdc, hOldBitmap);
    DeleteDC(hdc);
    return hBitmap;
}

// 不再需要独立的定时器回调函数，定时器消息现在在窗口过程中处理

// 查找记事本窗口
HWND FindNotepadEditWindow() {
    HWND hNotepad = FindWindow(L"Notepad", NULL);
    if (hNotepad) {
        g_hNotepadMain = hNotepad; // 保存主窗口
        
        // 获取主窗口标题进行验证
        wchar_t mainTitle[256];
        GetWindowText(hNotepad, mainTitle, 256);
        wchar_t debugMsg1[512];
        swprintf_s(debugMsg1, L"Found classic Notepad - Main: 0x%p (title: %s)", hNotepad, mainTitle);
        OutputDebugString(debugMsg1);
        
        HWND hEdit = FindWindowEx(hNotepad, NULL, L"Edit", NULL);
        if (hEdit) {
            wchar_t debugMsg2[256];
            swprintf_s(debugMsg2, L"Edit window: 0x%p", hEdit);
            OutputDebugString(debugMsg2);
            return hEdit;
        }
    }
    
    // 尝试新版本记事本
    hNotepad = FindWindow(L"ApplicationFrameWindow", NULL);
    if (hNotepad) {
        // 检查是否是记事本应用
        wchar_t title[256];
        GetWindowText(hNotepad, title, 256);
        if (wcsstr(title, L"Notepad") != NULL) {
            g_hNotepadMain = hNotepad;
            wchar_t debugMsg3[512];
            swprintf_s(debugMsg3, L"Found UWP Notepad - Main: 0x%p (title: %s)", hNotepad, title);
            OutputDebugString(debugMsg3);
            
            HWND hCoreWindow = FindWindowEx(hNotepad, NULL, L"Windows.UI.Core.CoreWindow", NULL);
            if (hCoreWindow) {
                wchar_t debugMsg4[256];
                swprintf_s(debugMsg4, L"Core window: 0x%p", hCoreWindow);
                OutputDebugString(debugMsg4);
                return hCoreWindow;
            }
        }
    }
    
    OutputDebugString(L"No Notepad window found!");
    return NULL;
}

// 增强的绘制函数 - 确保绘制能够保持
void DrawNoiseOverlay(HWND hwnd) {
    if (!g_hNoiseBitmap || !hwnd) return;
    
    // 使用GetWindowDC获取整个窗口的DC，包括非客户区
    HDC hdc = GetWindowDC(hwnd);
    if (!hdc) return;
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    // 绘制多个测试元素确保可见性
    if (rect.right > 100 && rect.bottom > 100) {
        // 1. 绘制移动的红色方块
        int x = 50 + g_noiseOffset;
        int y = 50 + (g_noiseOffset / 2);
        
        HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 100, 100));
        RECT redRect = {x, y, x + 30, y + 30};
        FillRect(hdc, &redRect, hRedBrush);
        DeleteObject(hRedBrush);
        
        // 2. 绘制一个固定的蓝色方块作为参考
        HBRUSH hBlueBrush = CreateSolidBrush(RGB(100, 100, 255));
        RECT blueRect = {10, 10, 40, 40};
        FillRect(hdc, &blueRect, hBlueBrush);
        DeleteObject(hBlueBrush);
        
        // 3. 绘制移动的噪声点
        for (int i = 0; i < 30; i++) {
            int px = (g_noiseOffset * 3 + i * 7) % (rect.right - 50) + 25;
            int py = (g_noiseOffset * 2 + i * 11) % (rect.bottom - 50) + 25;
            
            // 绘制较大的噪声点
            HBRUSH hNoiseBrush = CreateSolidBrush(RGB(128, 128, 128));
            RECT noiseRect = {px, py, px + 3, py + 3};
            FillRect(hdc, &noiseRect, hNoiseBrush);
            DeleteObject(hNoiseBrush);
        }
        
        // 4. 绘制一些文字覆盖
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 255)); // 紫色文字
        wchar_t overlayText[50];
        swprintf_s(overlayText, L"NOISE OVERLAY %d", g_noiseOffset);
        TextOut(hdc, 100, 100, overlayText, wcslen(overlayText));
    }
    
    // 强制立即显示
    GdiFlush();
    ReleaseDC(hwnd, hdc);
}

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        {
            g_hModule = hModule;
            
            OutputDebugString(L"DLL loaded successfully!");
            
            // 查找目标窗口
            g_hTargetWindow = FindNotepadEditWindow();
            if (!g_hTargetWindow) {
                OutputDebugString(L"Cannot find Notepad edit window!");
                return FALSE;
            }
            
            // 创建简单的噪声纹理
            g_hNoiseBitmap = CreateSimpleNoiseTexture(50, 50);
            if (!g_hNoiseBitmap) {
                OutputDebugString(L"Failed to create noise texture!");
                return FALSE;
            }
            
            // 使用窗口子类化而不是hook
            g_originalWndProc = (WNDPROC)SetWindowLongPtr(g_hTargetWindow, GWLP_WNDPROC, (LONG_PTR)SubclassWndProc);
            if (!g_originalWndProc) {
                OutputDebugString(L"Failed to subclass window!");
                return FALSE;
            }
            
            OutputDebugString(L"Window subclassed successfully!");
            
            // 创建与窗口关联的定时器，而不是全局定时器
            g_timerId = SetTimer(g_hTargetWindow, 1, 200, NULL); // 使用窗口定时器
            if (g_timerId) {
                wchar_t timerMsg[100];
                swprintf_s(timerMsg, L"Window timer created successfully! Timer ID: %d", g_timerId);
                OutputDebugString(timerMsg);
                
                // 验证并设置记事本主窗口标题
                if (g_hNotepadMain) {
                    wchar_t currentTitle[256];
                    GetWindowText(g_hNotepadMain, currentTitle, 256);
                    wchar_t beforeMsg[512];
                    swprintf_s(beforeMsg, L"Before title change - Current title: %s", currentTitle);
                    OutputDebugString(beforeMsg);
                    
                    BOOL titleResult = SetWindowText(g_hNotepadMain, L"Notepad - Noise Effect Active");
                    if (titleResult) {
                        OutputDebugString(L"Title changed successfully!");
                        
                        // 验证标题是否真的改了
                        Sleep(50);
                        wchar_t newTitle[256];
                        GetWindowText(g_hNotepadMain, newTitle, 256);
                        wchar_t afterMsg[512];
                        swprintf_s(afterMsg, L"After title change - New title: %s", newTitle);
                        OutputDebugString(afterMsg);
                    } else {
                        DWORD error = GetLastError();
                        wchar_t errorMsg[200];
                        swprintf_s(errorMsg, L"SetWindowText failed! Error: %d", error);
                        OutputDebugString(errorMsg);
                    }
                } else {
                    OutputDebugString(L"g_hNotepadMain is NULL, cannot set title!");
                }
                
                // 立即测试绘制
                OutputDebugString(L"Testing initial draw...");
                DrawNoiseOverlay(g_hTargetWindow);
                
                // 强制重绘一次以触发定时器循环
                InvalidateRect(g_hTargetWindow, NULL, TRUE);
                UpdateWindow(g_hTargetWindow);
                
                OutputDebugString(L"Initialization complete!");
            } else {
                DWORD error = GetLastError();
                wchar_t errorMsg[200];
                swprintf_s(errorMsg, L"Window timer creation failed! Error: %d", error);
                OutputDebugString(errorMsg);
                return FALSE;
            }
            break;
        }
            
        case DLL_PROCESS_DETACH:
        {
            // 清理资源
            if (g_timerId && g_hTargetWindow) {
                KillTimer(g_hTargetWindow, g_timerId); // 使用窗口定时器的清理方法
                g_timerId = 0;
            }
            
            // 恢复原始窗口过程
            if (g_hTargetWindow && g_originalWndProc) {
                SetWindowLongPtr(g_hTargetWindow, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
                g_originalWndProc = NULL;
            }
            
            if (g_hHook) {
                UnhookWindowsHookEx(g_hHook);
                g_hHook = NULL;
            }
            if (g_hNoiseBitmap) {
                DeleteObject(g_hNoiseBitmap);
                g_hNoiseBitmap = NULL;
            }
            break;
        }
    }
    return TRUE;
}

// 导出函数（保留兼容性）
extern "C" __declspec(dllexport) void InstallHook() {
    // 这个函数现在由DllMain自动处理
}

