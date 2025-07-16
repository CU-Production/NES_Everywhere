using System;
using System.Runtime.InteropServices;
using Sandbox;

public sealed class NesEmuTex : Component
{
	[Property] public Texture _noiseTexture;
	
	[Property] public ModelRenderer _renderer;
	[Property] public Material _material;
	
	private Agnes _nes;
	private Agnes.InputState _input1;
	
	[DllImport("kernel32.dll", SetLastError = true)]
	static extern bool SetDllDirectory(string lpPathName);
	
	void UpdateNoiseTexture()
	{
		var rand = new Random();
		var pixels = _noiseTexture.GetPixels();
		for(int y=0; y<256; y++) {
			for(int x=0; x<256; x++) {
				var value = (float)rand.NextDouble();
				pixels[y*256+x]  = new Color(value, value, value, 1);
			}
		}
		_noiseTexture.Update(pixels);

		_material.Set( "Color", _noiseTexture );
	}

	protected override void OnStart()
	{
		SetDllDirectory( @"E:\SW\EMU\NES_Everywhere\sbox\sboxnesemu\Plugins" );
		
		_noiseTexture = Texture.Create( 256, 256 )
			.WithFormat( ImageFormat.RGBA8888 )
			.WithDynamicUsage()
			.Finish();
		
		_renderer = GetComponent<ModelRenderer>();

		_material = _renderer.Materials.GetOriginal(0);

		UpdateNoiseTexture();
		
		// init nes        
		_nes = new Agnes();
		// bool ok = _nes.LoadInesDataFromPath("roms/mario.nes");
		bool ok = _nes.LoadInesDataFromPath(@"E:\SW\EMU\NES_Everywhere\sbox\sboxnesemu\Assets\roms\mario.nes");
		if (!ok)
		{
			Log.Error("mario.nes load failed!!!");
		}
		_input1 = new Agnes.InputState();
	}
	
	protected override void OnUpdate()
	{
		UpdateNoiseTexture();
		
		_input1.A      = Input.Down("NES_A");
		_input1.B      = Input.Down("NES_B");
		_input1.Select = Input.Down("NES_SELECT");
		_input1.Start  = Input.Down("NES_START");
		_input1.Up     = Input.Down("NES_UP");
		_input1.Down   = Input.Down("NES_DOWN");
		_input1.Left   = Input.Down("NES_LEFT");
		_input1.Right  = Input.Down("NES_RIGHT");
		
		byte input1U8 = 0;
		if(_input1.A      ) input1U8 |=  1 << 0;
		if(_input1.B      ) input1U8 |=  1 << 1;
		if(_input1.Select ) input1U8 |=  1 << 2;
		if(_input1.Start  ) input1U8 |=  1 << 3;
		if(_input1.Up     ) input1U8 |=  1 << 4;
		if(_input1.Down   ) input1U8 |=  1 << 5;
		if(_input1.Left   ) input1U8 |=  1 << 6;
		if(_input1.Right  ) input1U8 |=  1 << 7;
		
		_nes.SetInputU8(input1U8, 0);
		bool ok = _nes.NextFrame();
		if (!ok)
		{
			System.Diagnostics.Debug.WriteLine("agnes_next_frame failed!!!");
		}
		
		var pixels = _noiseTexture.GetPixels();
		for(int y=0; y<Agnes.SCREEN_HEIGHT; y++) {
			for(int x=0; x<Agnes.SCREEN_WIDTH; x++) {
				Agnes.Color color = _nes.GetScreenPixel(x, y);
				pixels[y*Agnes.SCREEN_WIDTH+x]  = new Color(color.R/255.0f, color.G/255.0f, color.B/255.0f, 1);
			}
		}
		_noiseTexture.Update(pixels);

		_material.Set( "Color", _noiseTexture );
	}
}
