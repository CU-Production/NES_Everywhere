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

// NES画面缓冲区 - 使用更高效的方法
HBITMAP g_nesBitmap = NULL;
HDC g_nesMemDC = NULL;
BITMAPINFO g_nesBitmapInfo = {0};
uint32_t* g_nesPixelBuffer = NULL; // 32位RGBA像素缓冲区

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

// 高效的NES画面绘制函数
void DrawNoiseOverlay(HWND hwnd) {
    if (!hwnd) return;
    
    HDC hdc = GetDC(hwnd);
    if (!hdc) return;
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    if (g_agnes && g_agnesInitialized && g_nesPixelBuffer && g_nesMemDC)
    {
        // 高效方法：直接写入内存缓冲区
        uint32_t* pixel = g_nesPixelBuffer;
        
        // 快速填充整个NES画面
        for (int y = 0; y < AGNES_SCREEN_HEIGHT; y++) {
            for (int x = 0; x < AGNES_SCREEN_WIDTH; x++) {
                // 获取NES像素颜色
                agnes_color_t color = agnes_get_screen_pixel(g_agnes, x, y);
                
                // 直接写入32位像素 (BGRA格式，Windows标准)
                *pixel++ = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
            }
        }
        
        // 计算显示位置和缩放
        float scaleX = (float)(rect.right - 40) / AGNES_SCREEN_WIDTH;
        float scaleY = (float)(rect.bottom - 60) / AGNES_SCREEN_HEIGHT;
        float scale = min(scaleX, scaleY);
        scale = max(scale, 1.0f); // 至少1倍大小
        
        int displayWidth = (int)(AGNES_SCREEN_WIDTH * scale);
        int displayHeight = (int)(AGNES_SCREEN_HEIGHT * scale);
        
        // 左上角显示，留出标题空间
        int offsetX = 10;
        int offsetY = 30;
        
        // 使用StretchBlt进行高效缩放绘制
        StretchBlt(hdc, offsetX, offsetY, displayWidth, displayHeight,
                   g_nesMemDC, 0, 0, AGNES_SCREEN_WIDTH, AGNES_SCREEN_HEIGHT, SRCCOPY);
        
        // 绘制一些干扰效果（减少数量以提高性能）
        for (int i = 0; i < 10; i++) {
            int px = (GetTickCount() / 100 + i * 17) % rect.right;
            int py = (GetTickCount() / 150 + i * 23) % rect.bottom;
            SetPixel(hdc, px, py, RGB(255, 255, 255));
        }
        
        // 显示游戏信息
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 255)); // 紫色文字
        wchar_t gameText[] = L"NES RUNNING ON NOTEPAD! (High Performance)";
        TextOut(hdc, 10, 10, gameText, wcslen(gameText));
        
        // 显示性能信息
        static DWORD lastTime = 0;
        static int frameCount = 0;
        static float fps = 0.0f;
        
        DWORD currentTime = GetTickCount();
        frameCount++;
        
        if (currentTime - lastTime > 1000) { // 每秒更新一次FPS
            fps = frameCount * 1000.0f / (currentTime - lastTime);
            frameCount = 0;
            lastTime = currentTime;
        }
        
        wchar_t fpsText[100];
        swprintf_s(fpsText, L"FPS: %.1f | Scale: %.1fx", fps, scale);
        TextOut(hdc, 10, rect.bottom - 40, fpsText, wcslen(fpsText));
        
    } else {
        // 如果模拟器未初始化，显示错误信息
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
    
    ReleaseDC(hwnd, hdc);
}

// 初始化NES显示缓冲区
bool InitializeNESDisplay() {
    // 设置DIB格式
    g_nesBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_nesBitmapInfo.bmiHeader.biWidth = AGNES_SCREEN_WIDTH;
    g_nesBitmapInfo.bmiHeader.biHeight = -AGNES_SCREEN_HEIGHT; // 负值表示top-down DIB
    g_nesBitmapInfo.bmiHeader.biPlanes = 1;
    g_nesBitmapInfo.bmiHeader.biBitCount = 32; // 32位RGBA
    g_nesBitmapInfo.bmiHeader.biCompression = BI_RGB;
    g_nesBitmapInfo.bmiHeader.biSizeImage = 0;
    
    // 创建DIB
    HDC hdc = GetDC(NULL);
    g_nesBitmap = CreateDIBSection(hdc, &g_nesBitmapInfo, DIB_RGB_COLORS, 
                                   (void**)&g_nesPixelBuffer, NULL, 0);
    ReleaseDC(NULL, hdc);
    
    if (!g_nesBitmap || !g_nesPixelBuffer) {
        OutputDebugString(L"Failed to create NES display buffer!");
        return false;
    }
    
    // 创建内存DC
    g_nesMemDC = CreateCompatibleDC(NULL);
    if (!g_nesMemDC) {
        OutputDebugString(L"Failed to create NES memory DC!");
        return false;
    }
    
    SelectObject(g_nesMemDC, g_nesBitmap);
    OutputDebugString(L"NES display buffer initialized successfully!");
    return true;
}

// 清理NES显示缓冲区
void CleanupNESDisplay() {
    if (g_nesMemDC) {
        DeleteDC(g_nesMemDC);
        g_nesMemDC = NULL;
    }
    if (g_nesBitmap) {
        DeleteObject(g_nesBitmap);
        g_nesBitmap = NULL;
    }
    g_nesPixelBuffer = NULL;
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
                    
                    // 初始化高效的显示缓冲区
                    if (InitializeNESDisplay()) {
                        OutputDebugString(L"NES display buffer created successfully!");
                    } else {
                        OutputDebugString(L"Failed to create NES display buffer!");
                        g_agnesInitialized = false;
                    }
                    
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
            
            // 初始化NES显示缓冲区
            InitializeNESDisplay();

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
            
            // 清理NES显示缓冲区
            CleanupNESDisplay();
            
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

