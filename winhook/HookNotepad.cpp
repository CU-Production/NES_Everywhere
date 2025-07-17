#define UNICODE
#include <windows.h>
#include <stdio.h>
#include "agnes.h"
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

// Agnes模拟器相关变量
agnes_t* g_agnes = NULL;
bool g_agnesInitialized = false;

// 保存原始窗口过程
WNDPROC g_originalWndProc = NULL;

// 自定义窗口过程 - 直接拦截消息
LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // 处理WM_TIMER消息 - 运行NES模拟器
    if (uMsg == WM_TIMER && wParam == 1) {
        if (g_agnes && g_agnesInitialized) {
            // 运行模拟器到下一帧
            bool newFrame = agnes_next_frame(g_agnes);
            if (newFrame) {
                // 输出调试信息确认模拟器正在运行
                OutputDebugString(L"NES frame updated");
                
                // 触发重绘显示新的游戏画面
                InvalidateRect(hwnd, NULL, FALSE);
            }
        } else {
            // 如果模拟器未初始化，仍然显示旧的动画
            g_noiseOffset = (g_noiseOffset + 2) % 50;
            OutputDebugString(L"Fallback animation");
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0; // 消费这个定时器消息
    }
    
    // 处理WM_PAINT消息
    if (uMsg == WM_PAINT) {
        // 先让原始的WM_PAINT处理
        LRESULT result = CallWindowProc(g_originalWndProc, hwnd, uMsg, wParam, lParam);
        
        // 然后立即绘制我们的效果
        DrawNoiseOverlay(hwnd);
        
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


// // 增强的绘制函数 - 确保绘制能够保持
// void DrawNoiseOverlay(HWND hwnd) {
//     if (!g_hNoiseBitmap || !hwnd) return;
//
//     // 使用GetWindowDC获取整个窗口的DC，包括非客户区
//     HDC hdc = GetWindowDC(hwnd);
//     if (!hdc) return;
//
//     RECT rect;
//     GetClientRect(hwnd, &rect);
//
//     // 绘制多个测试元素确保可见性
//     if (rect.right > 100 && rect.bottom > 100) {
//         // 1. 绘制移动的红色方块
//         int x = 50 + g_noiseOffset;
//         int y = 50 + (g_noiseOffset / 2);
//
//         HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 100, 100));
//         RECT redRect = {x, y, x + 30, y + 30};
//         FillRect(hdc, &redRect, hRedBrush);
//         DeleteObject(hRedBrush);
//
//         // 2. 绘制一个固定的蓝色方块作为参考
//         HBRUSH hBlueBrush = CreateSolidBrush(RGB(100, 100, 255));
//         RECT blueRect = {10, 10, 40, 40};
//         FillRect(hdc, &blueRect, hBlueBrush);
//         DeleteObject(hBlueBrush);
//
//         // 3. 绘制移动的噪声点
//         for (int i = 0; i < 30; i++) {
//             int px = (g_noiseOffset * 3 + i * 7) % (rect.right - 50) + 25;
//             int py = (g_noiseOffset * 2 + i * 11) % (rect.bottom - 50) + 25;
//
//             // 绘制较大的噪声点
//             HBRUSH hNoiseBrush = CreateSolidBrush(RGB(128, 128, 128));
//             RECT noiseRect = {px, py, px + 3, py + 3};
//             FillRect(hdc, &noiseRect, hNoiseBrush);
//             DeleteObject(hNoiseBrush);
//         }
//
//         // 4. 绘制一些文字覆盖
//         SetBkMode(hdc, TRANSPARENT);
//         SetTextColor(hdc, RGB(255, 0, 255)); // 紫色文字
//         wchar_t overlayText[50];
//         swprintf_s(overlayText, L"NOISE OVERLAY %d", g_noiseOffset);
//         TextOut(hdc, 100, 100, overlayText, wcslen(overlayText));
//     }
//
//     // 强制立即显示
//     GdiFlush();
//     ReleaseDC(hwnd, hdc);
// }

// NES画面绘制函数
void DrawNoiseOverlay(HWND hwnd) {
    if (!hwnd) return;

    HDC hdc = GetDC(hwnd);
    // 使用GetWindowDC获取整个窗口的DC，包括非客户区
    // HDC hdc = GetWindowDC(hwnd);
    if (!hdc) return;

    RECT rect;
    GetClientRect(hwnd, &rect);

    if (g_agnes && g_agnesInitialized)
    {
        // 绘制NES游戏画面


        // 左上角显示
        int offsetX = 0;
        int offsetY = 30;

        // 创建内存DC来绘制NES画面
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP nesFrame = CreateCompatibleBitmap(hdc, AGNES_SCREEN_WIDTH, AGNES_SCREEN_HEIGHT);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, nesFrame);

        // 逐像素绘制NES画面
        for (int y = 0; y < AGNES_SCREEN_HEIGHT; y++) {
            for (int x = 0; x < AGNES_SCREEN_WIDTH; x++) {
                // 获取NES像素颜色
                agnes_color_t color = agnes_get_screen_pixel(g_agnes, x, y);

                // 绘制像素
                SetPixel(memDC, x, y, RGB(color.r, color.g, color.b)); //very slow
            }
        }

        // 将NES画面绘制到记事本上
        BitBlt(hdc, offsetX, offsetY, AGNES_SCREEN_WIDTH, AGNES_SCREEN_HEIGHT, memDC, 0, 0, SRCCOPY);

        // 绘制一些半透明的干扰效果
        BLENDFUNCTION blend = {0};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 64; // 25% 透明度
        blend.AlphaFormat = 0;

        // 在游戏画面上叠加一些噪声点
        for (int i = 0; i < 20; i++) {
            int px = (GetTickCount() / 100 + i * 17) % rect.right;
            int py = (GetTickCount() / 150 + i * 23) % rect.bottom;
            SetPixel(hdc, px, py, RGB(255, 255, 255));
        }

        // 显示游戏信息
        SetBkMode(hdc, TRANSPARENT);
        // SetTextColor(hdc, RGB(255, 255, 0)); // 黄色文字
        SetTextColor(hdc, RGB(255, 0, 255)); // 紫色文字
        wchar_t gameText[] = L"NES RUNNING ON NOTEPAD!";
        TextOut(hdc, 10, 10, gameText, wcslen(gameText));

        SelectObject(memDC, oldBitmap);
        DeleteObject(nesFrame);
        DeleteDC(memDC);

    } else {
        // 如果模拟器未初始化，显示错误信息和备用动画
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 0)); // 红色文字
        wchar_t errorText[] = L"NES EMULATOR NOT LOADED";
        TextOut(hdc, 10, 10, errorText, wcslen(errorText));

        // 显示简单的备用动画
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 100, 100));
        RECT animRect = {50 + g_noiseOffset, 50, 80 + g_noiseOffset, 80};
        FillRect(hdc, &animRect, hBrush);
        DeleteObject(hBrush);
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
            
            // 初始化Agnes NES模拟器
            OutputDebugString(L"Initializing Agnes NES emulator...");
            g_agnes = agnes_make();
            if (g_agnes) {
                // 构建ROM文件路径
                char currentDir[MAX_PATH];
                GetCurrentDirectoryA(MAX_PATH, currentDir);
                char romPath[MAX_PATH];
                sprintf_s(romPath, "%s\\mario.nes", currentDir);
                
                // 加载超级马里奥ROM
                if (agnes_load_ines_data_from_path(g_agnes, romPath)) {
                    g_agnesInitialized = true;
                    OutputDebugString(L"Mario ROM loaded successfully!");
                    
                    // 设置一些默认输入（让马里奥自动向右走）
                    agnes_input_t input = {0};
                    input.right = true; // 持续按右键
                    agnes_set_input(g_agnes, &input, NULL);
                    
                } else {
                    OutputDebugString(L"Failed to load Mario ROM!");
                    wchar_t romPathW[MAX_PATH];
                    MultiByteToWideChar(CP_ACP, 0, romPath, -1, romPathW, MAX_PATH);
                    wchar_t debugMsg[512];
                    swprintf_s(debugMsg, L"ROM path: %s", romPathW);
                    OutputDebugString(debugMsg);
                }
            } else {
                OutputDebugString(L"Failed to create Agnes emulator instance!");
            }
            
            // 创建简单的噪声纹理作为备用
            g_hNoiseBitmap = CreateSimpleNoiseTexture(50, 50);
            
            // 使用窗口子类化
            g_originalWndProc = (WNDPROC)SetWindowLongPtr(g_hTargetWindow, GWLP_WNDPROC, (LONG_PTR)SubclassWndProc);
            if (!g_originalWndProc) {
                OutputDebugString(L"Failed to subclass window!");
                return FALSE;
            }
            
            OutputDebugString(L"Window subclassed successfully!");
            
            // 创建定时器 - 60FPS for smooth NES emulation
            g_timerId = SetTimer(g_hTargetWindow, 1, 16, NULL); // ~60 FPS
            if (g_timerId) {
                OutputDebugString(L"High-speed timer created for NES emulation!");
                
                // 设置记事本标题
                if (g_hNotepadMain) {
                    SetWindowText(g_hNotepadMain, L"Notepad - NES Emulator Active");
                }
                
                // 立即测试绘制
                DrawNoiseOverlay(g_hTargetWindow);
                InvalidateRect(g_hTargetWindow, NULL, TRUE);
                UpdateWindow(g_hTargetWindow);
                
                OutputDebugString(L"NES emulator initialization complete!");
            } else {
                OutputDebugString(L"Timer creation failed!");
                return FALSE;
            }
            break;
        }
            
        case DLL_PROCESS_DETACH:
        {
            OutputDebugString(L"Cleaning up NES emulator...");
            
            // 清理定时器
            if (g_timerId && g_hTargetWindow) {
                KillTimer(g_hTargetWindow, g_timerId);
                g_timerId = 0;
            }
            
            // 清理Agnes模拟器
            if (g_agnes) {
                agnes_destroy(g_agnes);
                g_agnes = NULL;
                g_agnesInitialized = false;
                OutputDebugString(L"Agnes emulator destroyed");
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
            
            OutputDebugString(L"Cleanup complete!");
            break;
        }
    }
    return TRUE;
}

// 导出函数（保留兼容性）
extern "C" __declspec(dllexport) void InstallHook() {
    // 这个函数现在由DllMain自动处理
}

