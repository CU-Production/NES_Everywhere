
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
    
    private SWIGTYPE_p_agnes nes;
    private agnes_input_t input;
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
        
        nes = agnes.agnes_make();
        bool ok = agnes.agnes_load_ines_data_from_path(nes, "mario.nes");
        if (!ok)
        {
            System.Diagnostics.Debug.WriteLine("mario.nes load failed!!!");
        }
        input = new agnes_input_t();
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

        input.a      = keyStates[(int)Keys.J];
        input.b      = keyStates[(int)Keys.K];
        input.select = keyStates[(int)Keys.Back];
        input.start  = keyStates[(int)Keys.Return];
        input.up     = keyStates[(int)Keys.W];
        input.down   = keyStates[(int)Keys.S];
        input.left   = keyStates[(int)Keys.A];
        input.right  = keyStates[(int)Keys.D];
        
        agnes.agnes_set_input(nes, input, null);
        bool ok = agnes.agnes_next_frame(nes);
        if (!ok)
        {
            System.Diagnostics.Debug.WriteLine("agnes_next_frame failed!!!");
        }

        for (int j = 0; j < 240; j++)
        {
            for (int i = 0; i < 256; i++)
            {
                agnes_color_t color = agnes.agnes_get_screen_pixel(nes, i, j);
                Color c2 = Color.FromArgb(255, color.r, color.g, color.b);
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
