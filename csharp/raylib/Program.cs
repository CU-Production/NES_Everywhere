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
        Agnes nes = new Agnes();
        bool ok = nes.LoadInesDataFromPath("mario.nes");
        if (!ok)
        {
            System.Diagnostics.Debug.WriteLine("mario.nes load failed!!!");
        }
        Agnes.InputState input1 = new Agnes.InputState();

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

            input1.A      = IsKeyDown(KeyboardKey.Z);
            input1.B      = IsKeyDown(KeyboardKey.X);
            input1.Select = IsKeyDown(KeyboardKey.Backspace);
            input1.Start  = IsKeyDown(KeyboardKey.Enter);
            input1.Up     = IsKeyDown(KeyboardKey.Up);
            input1.Down   = IsKeyDown(KeyboardKey.Down);
            input1.Left   = IsKeyDown(KeyboardKey.Left);
            input1.Right  = IsKeyDown(KeyboardKey.Right);
            
            byte input1U8 = 0;
            if(input1.A      ) input1U8 |=  1 << 0;
            if(input1.B      ) input1U8 |=  1 << 1;
            if(input1.Select ) input1U8 |=  1 << 2;
            if(input1.Start  ) input1U8 |=  1 << 3;
            if(input1.Up     ) input1U8 |=  1 << 4;
            if(input1.Down   ) input1U8 |=  1 << 5;
            if(input1.Left   ) input1U8 |=  1 << 6;
            if(input1.Right  ) input1U8 |=  1 << 7;

            // Draw
            //----------------------------------------------------------------------------------
            BeginDrawing();
            ClearBackground(Raylib_cs.Color.RayWhite);
            
            nes.SetInputU8(input1U8, 0);
            ok = nes.NextFrame();
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
                        Agnes.Color color = nes.GetScreenPixel(i, j);
                        pixels[j*256+i].R = color.R;
                        pixels[j*256+i].G = color.G;
                        pixels[j*256+i].B = color.B;
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