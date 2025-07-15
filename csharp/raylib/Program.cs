using System.Numerics;
using System;
using System.Text;
using Raylib_cs;
using static Raylib_cs.Raylib;
using static Raylib_cs.Rlgl;

public class NesEmu
{
    public static int Main()
    {
        // Initialization
        //--------------------------------------------------------------------------------------
        const int screenWidth = 256;
        const int screenHeight = 240;

        InitWindow(screenWidth, screenHeight, "raylib-cs Nes Emu");

        SetTargetFPS(60);
        
        // init nes        
        SWIGTYPE_p_agnes nes = agnes.agnes_make();
        bool ok = agnes.agnes_load_ines_data_from_path(nes, "mario.nes");
        if (!ok)
        {
            System.Diagnostics.Debug.WriteLine("mario.nes load failed!!!");
        }
        agnes_input_t input = new agnes_input_t();

        Image image = GenImageColor(screenWidth, screenHeight, Color.Green);
        Texture2D tmpTexture = LoadTextureFromImage(image);
        //--------------------------------------------------------------------------------------

        // Main game loop
        DateTime lastFrameTime = DateTime.Now;
        float frameCounter = 0.0f;
        float fps = 0.0f;
        while (!WindowShouldClose())
        {
            // Update
            //----------------------------------------------------------------------------------
            DateTime now = DateTime.Now;
            float deltaTime = (float)(now - lastFrameTime).TotalSeconds;
            lastFrameTime = now;
            
            frameCounter++;
            // if (frameCounter >= 10)
            {
                fps = 1f / deltaTime;
                frameCounter = 0;
            }

            input.a      = IsKeyDown(KeyboardKey.Z);
            input.b      = IsKeyDown(KeyboardKey.X);
            input.select = IsKeyDown(KeyboardKey.Backspace);
            input.start  = IsKeyDown(KeyboardKey.Enter);
            input.up     = IsKeyDown(KeyboardKey.Up);
            input.down   = IsKeyDown(KeyboardKey.Down);
            input.left   = IsKeyDown(KeyboardKey.Left);
            input.right  = IsKeyDown(KeyboardKey.Right);

            // Draw
            //----------------------------------------------------------------------------------
            BeginDrawing();
            ClearBackground(Raylib_cs.Color.RayWhite);
            
            agnes.agnes_set_input(nes, input, null);
            ok = agnes.agnes_next_frame(nes);
            if (!ok)
            {
                System.Diagnostics.Debug.WriteLine("agnes_next_frame failed!!!");
            }
            
            unsafe
            {
                Raylib_cs.Color* pixels = LoadImageColors(image);
                for (int j = 0; j < 240; j++)
                {
                    for (int i = 0; i < 256; i++)
                    {
                        agnes_color_t color = agnes.agnes_get_screen_pixel(nes, i, j);
                        pixels[j*256+i].R = color.r;
                        pixels[j*256+i].G = color.g;
                        pixels[j*256+i].B = color.b;
                        pixels[j*256+i].A = 255;
                    }
                }
                UpdateTexture(tmpTexture, pixels);
                UnloadImageColors(pixels);
            }

            DrawTexture(tmpTexture, 0, 0, Raylib_cs.Color.White);

            DrawText($"FPS: {fps}", 10, 10, 20, Raylib_cs.Color.Maroon);
            
            EndDrawing();
            //----------------------------------------------------------------------------------
        }

        // De-Initialization
        //--------------------------------------------------------------------------------------
        CloseWindow();
        //--------------------------------------------------------------------------------------

        return 0;
    }
}