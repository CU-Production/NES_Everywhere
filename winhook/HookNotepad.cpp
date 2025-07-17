#define UNICODE
#include <windows.h>
#include <stdio.h>
#include "agnes.h"
#pragma comment(lib, "msimg32.lib")  // for AlphaBlend

void DrawNoiseOverlay(HWND hwnd);

// Global variables
HHOOK g_hHook = NULL;
HBITMAP g_hNoiseBitmap = NULL;
HWND g_hTargetWindow = NULL;
HWND g_hNotepadMain = NULL;
UINT_PTR g_timerId = 0;
int g_noiseOffset = 0;
bool g_needsRedraw = false;
HMODULE g_hModule = NULL;

// Agnes emulator related variables
agnes_t* g_agnes = NULL;
bool g_agnesInitialized = false;

// NES display buffer - using more efficient method
HBITMAP g_nesBitmap = NULL;
HDC g_nesMemDC = NULL;
BITMAPINFO g_nesBitmapInfo = {0};
uint32_t* g_nesPixelBuffer = NULL; // 32-bit RGBA pixel buffer

// Save original window procedure
WNDPROC g_originalWndProc = NULL;

// Custom window procedure - directly intercept messages
LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Handle WM_TIMER messages - run NES emulator
    if (uMsg == WM_TIMER && wParam == 1) {
        if (g_agnes && g_agnesInitialized) {
            // Run emulator to next frame
            bool newFrame = agnes_next_frame(g_agnes);
            if (newFrame) {
                // Output debug info to confirm emulator is running
                OutputDebugString(L"NES frame updated");
                
                // Trigger redraw to display new game screen
                InvalidateRect(hwnd, NULL, FALSE);
            }
        } else {
            // If emulator not initialized, still show old animation
            g_noiseOffset = (g_noiseOffset + 2) % 50;
            OutputDebugString(L"Fallback animation");
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0; // Consume this timer message
    }
    
    // Handle WM_PAINT messages
    if (uMsg == WM_PAINT) {
        // Let original WM_PAINT handle first
        LRESULT result = CallWindowProc(g_originalWndProc, hwnd, uMsg, wParam, lParam);
        
        // Then immediately draw our effects
        DrawNoiseOverlay(hwnd);
        
        return result;
    }
    
    // Handle other messages that might affect drawing
    if (uMsg == WM_ERASEBKGND) {
        LRESULT result = CallWindowProc(g_originalWndProc, hwnd, uMsg, wParam, lParam);
        DrawNoiseOverlay(hwnd);
        return result;
    }
    
    // Custom messages
    if (uMsg == WM_USER + 100) {
        DrawNoiseOverlay(hwnd);
        OutputDebugString(L"Custom message WM_USER+100 processed");
        return 0;
    }
    
    // Handle other messages normally
    return CallWindowProc(g_originalWndProc, hwnd, uMsg, wParam, lParam);
}

// Generate simple noise texture
HBITMAP CreateSimpleNoiseTexture(int width, int height) {
    HDC hScreenDC = GetDC(NULL);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HDC hdc = CreateCompatibleDC(hScreenDC);
    ReleaseDC(NULL, hScreenDC);
    
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);

    // Generate simple noise pattern
    srand(GetTickCount());
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            BYTE gray = rand() % 64 + 128; // Brighter gray noise
            SetPixel(hdc, x, y, RGB(gray, gray, gray));
        }
    }
    
    SelectObject(hdc, hOldBitmap);
    DeleteDC(hdc);
    return hBitmap;
}

// No longer need independent timer callback function, timer messages are now handled in window procedure

// Find Notepad windows
HWND FindNotepadEditWindow() {
    HWND hNotepad = FindWindow(L"Notepad", NULL);
    if (hNotepad) {
        g_hNotepadMain = hNotepad; // Save main window
        
        // Get main window title for verification
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
    
    // Try new version Notepad
    hNotepad = FindWindow(L"ApplicationFrameWindow", NULL);
    if (hNotepad) {
        // Check if it's Notepad application
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

// Efficient NES screen drawing function
void DrawNoiseOverlay(HWND hwnd) {
    if (!hwnd) return;
    
    HDC hdc = GetDC(hwnd);
    if (!hdc) return;
    
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    if (g_agnes && g_agnesInitialized && g_nesPixelBuffer && g_nesMemDC)
    {
        // Efficient method: directly write to memory buffer
        uint32_t* pixel = g_nesPixelBuffer;
        
        // Quickly fill entire NES screen
        for (int y = 0; y < AGNES_SCREEN_HEIGHT; y++) {
            for (int x = 0; x < AGNES_SCREEN_WIDTH; x++) {
                // Get NES pixel color
                agnes_color_t color = agnes_get_screen_pixel(g_agnes, x, y);
                
                // Directly write 32-bit pixel (BGRA format, Windows standard)
                *pixel++ = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
            }
        }
        
        // Calculate display position and scaling
        float scaleX = (float)(rect.right - 40) / AGNES_SCREEN_WIDTH;
        float scaleY = (float)(rect.bottom - 60) / AGNES_SCREEN_HEIGHT;
        float scale = min(scaleX, scaleY);
        scale = max(scale, 1.0f); // At least 1x size
        
        int displayWidth = (int)(AGNES_SCREEN_WIDTH * scale);
        int displayHeight = (int)(AGNES_SCREEN_HEIGHT * scale);
        
        // Display in top-left, leave space for title
        int offsetX = 10;
        int offsetY = 30;
        
        // Use StretchBlt for efficient scaled drawing
        StretchBlt(hdc, offsetX, offsetY, displayWidth, displayHeight,
                   g_nesMemDC, 0, 0, AGNES_SCREEN_WIDTH, AGNES_SCREEN_HEIGHT, SRCCOPY);
        
        // // Draw some interference effects (reduced quantity to improve performance)
        // for (int i = 0; i < 10; i++) {
        //     int px = (GetTickCount() / 100 + i * 17) % rect.right;
        //     int py = (GetTickCount() / 150 + i * 23) % rect.bottom;
        //     SetPixel(hdc, px, py, RGB(255, 255, 255));
        // }
        
        // Display game information
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 255)); // Purple text
        wchar_t gameText[] = L"NES RUNNING ON NOTEPAD!";
        TextOut(hdc, 10, 10, gameText, wcslen(gameText));
        
        // Display performance information
        static DWORD lastTime = 0;
        static int frameCount = 0;
        static float fps = 0.0f;
        
        DWORD currentTime = GetTickCount();
        frameCount++;
        
        if (currentTime - lastTime > 1000) { // Update FPS once per second
            fps = frameCount * 1000.0f / (currentTime - lastTime);
            frameCount = 0;
            lastTime = currentTime;
        }
        
        wchar_t fpsText[100];
        swprintf_s(fpsText, L"FPS: %.1f | Scale: %.1fx", fps, scale);
        TextOut(hdc, 10, rect.bottom - 30, fpsText, wcslen(fpsText));
        
    } else {
        // If emulator not initialized, display error message
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 0, 0)); // Red text
        wchar_t errorText[] = L"NES EMULATOR NOT LOADED";
        TextOut(hdc, 10, 10, errorText, wcslen(errorText));
        
        // Display simple fallback animation
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 100, 100));
        RECT animRect = {50 + g_noiseOffset, 50, 80 + g_noiseOffset, 80};
        FillRect(hdc, &animRect, hBrush);
        DeleteObject(hBrush);
    }
    
    ReleaseDC(hwnd, hdc);
}

// Initialize NES display buffer
bool InitializeNESDisplay() {
    // Set DIB format
    g_nesBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_nesBitmapInfo.bmiHeader.biWidth = AGNES_SCREEN_WIDTH;
    g_nesBitmapInfo.bmiHeader.biHeight = -AGNES_SCREEN_HEIGHT; // Negative value indicates top-down DIB
    g_nesBitmapInfo.bmiHeader.biPlanes = 1;
    g_nesBitmapInfo.bmiHeader.biBitCount = 32; // 32-bit RGBA
    g_nesBitmapInfo.bmiHeader.biCompression = BI_RGB;
    g_nesBitmapInfo.bmiHeader.biSizeImage = 0;
    
    // Create DIB
    HDC hdc = GetDC(NULL);
    g_nesBitmap = CreateDIBSection(hdc, &g_nesBitmapInfo, DIB_RGB_COLORS, 
                                   (void**)&g_nesPixelBuffer, NULL, 0);
    ReleaseDC(NULL, hdc);
    
    if (!g_nesBitmap || !g_nesPixelBuffer) {
        OutputDebugString(L"Failed to create NES display buffer!");
        return false;
    }
    
    // Create memory DC
    g_nesMemDC = CreateCompatibleDC(NULL);
    if (!g_nesMemDC) {
        OutputDebugString(L"Failed to create NES memory DC!");
        return false;
    }
    
    SelectObject(g_nesMemDC, g_nesBitmap);
    OutputDebugString(L"NES display buffer initialized successfully!");
    return true;
}

// Cleanup NES display buffer
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

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        {
            g_hModule = hModule;
            
            OutputDebugString(L"DLL loaded successfully!");
            
            // Find target window
            g_hTargetWindow = FindNotepadEditWindow();
            if (!g_hTargetWindow) {
                OutputDebugString(L"Cannot find Notepad edit window!");
                return FALSE;
            }
            
            // Initialize Agnes NES emulator
            OutputDebugString(L"Initializing Agnes NES emulator...");
            g_agnes = agnes_make();
            if (g_agnes) {
                // Build ROM file path
                char currentDir[MAX_PATH];
                GetCurrentDirectoryA(MAX_PATH, currentDir);
                char romPath[MAX_PATH];
                sprintf_s(romPath, "%s\\mario.nes", currentDir);
                
                // Load Super Mario ROM
                if (agnes_load_ines_data_from_path(g_agnes, romPath)) {
                    g_agnesInitialized = true;
                    OutputDebugString(L"Mario ROM loaded successfully!");
                    
                    // Initialize efficient display buffer
                    if (InitializeNESDisplay()) {
                        OutputDebugString(L"NES display buffer created successfully!");
                    } else {
                        OutputDebugString(L"Failed to create NES display buffer!");
                        g_agnesInitialized = false;
                    }
                    
                    // Set some default input (make Mario automatically walk right)
                    agnes_input_t input = {0};
                    input.right = true; // Continuously press right key
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
            
            // Create simple noise texture as backup
            g_hNoiseBitmap = CreateSimpleNoiseTexture(50, 50);
            
            // Initialize NES display buffer
            InitializeNESDisplay();

            // Use window subclassing
            g_originalWndProc = (WNDPROC)SetWindowLongPtr(g_hTargetWindow, GWLP_WNDPROC, (LONG_PTR)SubclassWndProc);
            if (!g_originalWndProc) {
                OutputDebugString(L"Failed to subclass window!");
                return FALSE;
            }
            
            OutputDebugString(L"Window subclassed successfully!");
            
            // Create timer - 60FPS for smooth NES emulation
            g_timerId = SetTimer(g_hTargetWindow, 1, 16, NULL); // ~60 FPS
            if (g_timerId) {
                OutputDebugString(L"High-speed timer created for NES emulation!");
                
                // Set Notepad title
                if (g_hNotepadMain) {
                    SetWindowText(g_hNotepadMain, L"Notepad - NES Emulator Active");
                }
                
                // Immediately test drawing
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
            
            // Cleanup timer
            if (g_timerId && g_hTargetWindow) {
                KillTimer(g_hTargetWindow, g_timerId);
                g_timerId = 0;
            }
            
            // Cleanup Agnes emulator
            if (g_agnes) {
                agnes_destroy(g_agnes);
                g_agnes = NULL;
                g_agnesInitialized = false;
                OutputDebugString(L"Agnes emulator destroyed");
            }
            
            // Cleanup NES display buffer
            CleanupNESDisplay();
            
            // Restore original window procedure
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

// Export function (maintain compatibility)
extern "C" __declspec(dllexport) void InstallHook() {
    // This function is now automatically handled by DllMain
}

