using Godot;
using System;

public partial class Sprite3d : Sprite3D
{
	private Image _image;
	
	private Agnes _nes;
	private Agnes.InputState _input1;
	
	public override void _Ready()
	{
		_image = Image.Create(256, 256, false, Image.Format.Rgba8);
		
		var random = new Random();
		for (int x = 0; x < _image.GetWidth(); x++)
		{
			for (int y = 0; y < _image.GetHeight(); y++)
			{
				var color = new Color(
					(float)random.NextDouble(),
					(float)random.NextDouble(),
					(float)random.NextDouble()
				);
				_image.SetPixel(x, y, color);
			}
		}

		this.Texture = ImageTexture.CreateFromImage(_image);
		
		// nes init
		_nes = new Agnes();
		bool ok = _nes.LoadInesDataFromPath("Roms/mario.nes");
		if (!ok)
		{
			System.Diagnostics.Debug.WriteLine("mario.nes load failed!!!");
		}
		_input1 = new Agnes.InputState();
	}

	public override void _Input(InputEvent ev)
	{
		if (ev is InputEventKey eventKey)
		{
			_input1.A      = (eventKey.Keycode == Key.Z && eventKey.Pressed);
			_input1.B      = (eventKey.Keycode == Key.X && eventKey.Pressed);
			_input1.Select = (eventKey.Keycode == Key.Back && eventKey.Pressed);
			_input1.Start  = (eventKey.Keycode == Key.Enter && eventKey.Pressed);
			_input1.Up     = (eventKey.Keycode == Key.Up && eventKey.Pressed);
			_input1.Down   = (eventKey.Keycode == Key.Down && eventKey.Pressed);
			_input1.Left   = (eventKey.Keycode == Key.Left && eventKey.Pressed);
			_input1.Right  = (eventKey.Keycode == Key.Right && eventKey.Pressed);
		}
	}
	
	public override void _Process(double delta)
	{
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

		for (int j = 0; j < Agnes.SCREEN_HEIGHT; j++)
		{
			for (int i = 0; i < Agnes.SCREEN_WIDTH; i++)
			{
				Agnes.Color color = _nes.GetScreenPixel(i, j);
				Color c2 = new Color(color.R/255.0f, color.G/255.0f, color.B/255.0f);
				_image.SetPixel(i, j, c2);
			}
		}
		
		this.Texture = ImageTexture.CreateFromImage(_image);
	}
}
