
using System;
using System.Drawing;
using System.Windows.Forms;

public class GameForm : Form
{
    private GameCanvas canvas;
    
    public GameForm()
    {
        Text = "C# nes demo";
        Size = new Size(300, 300);
        
        canvas = new GameCanvas();
        canvas.Dock = DockStyle.Fill;
        Controls.Add(canvas);
        
        KeyPreview = true;
        KeyDown += OnKeyDown;
        KeyUp += OnKeyUp;
    }
    
    private void OnKeyDown(object sender, KeyEventArgs e)
    {
        canvas.SetKeyState(e.KeyCode, true);
    }
    
    private void OnKeyUp(object sender, KeyEventArgs e)
    {
        canvas.SetKeyState(e.KeyCode, false);
    }
    
    [STAThread]
    static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new GameForm());
    }
}

public class GameCanvas : Control
{
    private BufferedGraphicsContext context;
    private BufferedGraphics backBuffer;
    private System.Windows.Forms.Timer gameTimer;
    private int targetFPS = 120;
    private DateTime lastFrameTime;
    private float frameCounter;
    private float fps;
    
    private Agnes nes;
    private Agnes.InputState input1;
    private Bitmap tmpBuffer;
    
    private bool[] keyStates = new bool[256];
    
    public GameCanvas()
    {
        SetStyle(ControlStyles.OptimizedDoubleBuffer | 
                ControlStyles.UserPaint | 
                ControlStyles.AllPaintingInWmPaint, true);
        
        context = BufferedGraphicsManager.Current;
        ResizeBuffer();
        
        gameTimer = new System.Windows.Forms.Timer();
        gameTimer.Interval = (int)(1000f / targetFPS);
        gameTimer.Tick += GameLoop;
        gameTimer.Start();
        
        lastFrameTime = DateTime.Now;
        
        Width = 256;
        Height = 240;
        
        nes = new Agnes();
        bool ok = nes.LoadInesDataFromPath("mario.nes");
        if (!ok)
        {
            System.Diagnostics.Debug.WriteLine("mario.nes load failed!!!");
        }
        input1 = new Agnes.InputState();
    }
    
    private void ResizeBuffer()
    {
        if (Width > 0 && Height > 0)
        {
            context.MaximumBuffer = new Size(Width + 1, Height + 1);
            backBuffer = context.Allocate(this.CreateGraphics(), 
                new Rectangle(0, 0, Width, Height));
            
            tmpBuffer = new Bitmap(Width, Height);
        }
    }
    
    protected override void OnPaint(PaintEventArgs e)
    {
        backBuffer.Render(e.Graphics);
        
        var fpsText = $"FPS: {fps:0.0}";
        e.Graphics.DrawString(fpsText, Font, Brushes.Red, 10, 10);
    }
    
    protected override void OnPaintBackground(PaintEventArgs e)
    {
    }
    
    protected override void OnResize(EventArgs e)
    {
        base.OnResize(e);
        ResizeBuffer();
    }
    
    public void SetKeyState(Keys key, bool state)
    {
        keyStates[(int)key] = state;
    }
    
    private void GameLoop(object sender, EventArgs e)
    {
        DateTime now = DateTime.Now;
        float deltaTime = (float)(now - lastFrameTime).TotalSeconds;
        lastFrameTime = now;
        
        frameCounter++;
        if (frameCounter >= 10)
        {
            fps = 1f / deltaTime;
            frameCounter = 0;
        }
        
        backBuffer.Graphics.Clear(Color.White);

        input1.A      = keyStates[(int)Keys.J];
        input1.B      = keyStates[(int)Keys.K];
        input1.Select = keyStates[(int)Keys.Back];
        input1.Start  = keyStates[(int)Keys.Enter];
        input1.Up     = keyStates[(int)Keys.W];
        input1.Down   = keyStates[(int)Keys.S];
        input1.Left   = keyStates[(int)Keys.A];
        input1.Right  = keyStates[(int)Keys.D];
        
        byte input1U8 = 0;
        if(input1.A      ) input1U8 |=  1 << 0;
        if(input1.B      ) input1U8 |=  1 << 1;
        if(input1.Select ) input1U8 |=  1 << 2;
        if(input1.Start  ) input1U8 |=  1 << 3;
        if(input1.Up     ) input1U8 |=  1 << 4;
        if(input1.Down   ) input1U8 |=  1 << 5;
        if(input1.Left   ) input1U8 |=  1 << 6;
        if(input1.Right  ) input1U8 |=  1 << 7;
        
        nes.SetInputU8(input1U8, 0);
        bool ok = nes.NextFrame();
        if (!ok)
        {
            System.Diagnostics.Debug.WriteLine("agnes_next_frame failed!!!");
        }

        for (int j = 0; j < 240; j++)
        {
            for (int i = 0; i < 256; i++)
            {
                Agnes.Color color = nes.GetScreenPixel(i, j);
                Color c2 = Color.FromArgb(255, color.R, color.G, color.B);
                tmpBuffer.SetPixel(i, j, c2);
            }
        }
        backBuffer.Graphics.DrawImage(tmpBuffer, 0, 0);
        
        Invalidate();
    }
    
    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            gameTimer.Stop();
            backBuffer?.Dispose();
        }
        base.Dispose(disposing);
    }
}
