
// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Krzysztof Gabis

using System;
using System.Runtime.InteropServices;

public class Agnes
{
    public const int SCREEN_WIDTH = 256;
    public const int SCREEN_HEIGHT = 240;
    
    public struct InputState
    {
        public bool A;
        public bool B;
        public bool Select;
        public bool Start;
        public bool Up;
        public bool Down;
        public bool Left;
        public bool Right;
    }

    public struct Color
    {
        public byte R;
        public byte G;
        public byte B;
        public byte A;
    }

    private IntPtr _handle;

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr agnes_make();

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern void agnes_destroy(IntPtr agn);

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool agnes_load_ines_data(IntPtr agnes, IntPtr data, UIntPtr dataSize);

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern void agnes_set_input(IntPtr agnes, ref InputState input1, ref InputState input2);
    
    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern void agnes_set_input_u8(IntPtr agnes, byte input1, byte input2);

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern UIntPtr agnes_state_size();

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern void agnes_dump_state(IntPtr agnes, IntPtr outRes);

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool agnes_restore_state(IntPtr agnes, IntPtr state);

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool agnes_tick(IntPtr agnes, out bool outNewFrame);

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool agnes_next_frame(IntPtr agnes);

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern Color agnes_get_screen_pixel(IntPtr agnes, int x, int y);

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr agnes_read_file(string filename, out UIntPtr outLen);

    [DllImport("nes", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool agnes_load_ines_data_from_path(IntPtr agnes, string filename);

    public Agnes()
    {
        _handle = agnes_make();
    }

    ~Agnes()
    {
        if (_handle != IntPtr.Zero)
        {
            agnes_destroy(_handle);
        }
    }

    public bool LoadInesData(byte[] data)
    {
        IntPtr dataPtr = Marshal.AllocHGlobal(data.Length);
        Marshal.Copy(data, 0, dataPtr, data.Length);
        bool result = agnes_load_ines_data(_handle, dataPtr, (UIntPtr)data.Length);
        Marshal.FreeHGlobal(dataPtr);
        return result;
    }

    public void SetInput(ref InputState input1, ref InputState input2)
    {
        agnes_set_input(_handle, ref input1, ref input2);
    }
    
    public void SetInputU8(byte input1, byte input2)
    {
        agnes_set_input_u8(_handle, input1, input2);
    }

    public uint GetStateSize()
    {
        return (uint)agnes_state_size();
    }

    public bool Tick(out bool newFrame)
    {
        return agnes_tick(_handle, out newFrame);
    }

    public bool NextFrame()
    {
        return agnes_next_frame(_handle);
    }

    public Color GetScreenPixel(int x, int y)
    {
        return agnes_get_screen_pixel(_handle, x, y);
    }

    public bool LoadInesDataFromPath(string path)
    {
        return agnes_load_ines_data_from_path(_handle, path);
    }
}
