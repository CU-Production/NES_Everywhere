-- For ZeroBrane debugger
if arg[#arg] == "-debug" then require("mobdebug").start() end

local ffi = require 'ffi'

ffi.cdef [[
enum {
    AGNES_SCREEN_WIDTH = 256,
    AGNES_SCREEN_HEIGHT = 240
};

typedef struct {
    bool a;
    bool b;
    bool select;
    bool start;
    bool up;
    bool down;
    bool left;
    bool right;
} agnes_input_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} agnes_color_t;

/************************************ CPU ************************************/

typedef enum {
    INTERRPUT_NONE = 0,
    INTERRUPT_NMI = 1,
    INTERRUPT_IRQ = 2
} cpu_interrupt_t;

typedef struct cpu {
    struct agnes *agnes;
    uint16_t pc;
    uint8_t sp;
    uint8_t acc;
    uint8_t x;
    uint8_t y;
    uint8_t flag_carry;
    uint8_t flag_zero;
    uint8_t flag_dis_interrupt;
    uint8_t flag_decimal;
    uint8_t flag_overflow;
    uint8_t flag_negative;
    uint32_t stall;
    uint64_t cycles;
    cpu_interrupt_t interrupt;
} cpu_t;

/************************************ PPU ************************************/

typedef struct {
    uint8_t y_pos;
    uint8_t tile_num;
    uint8_t attrs;
    uint8_t x_pos;
} sprite_t;

typedef struct ppu {
    struct agnes *agnes;

    uint8_t nametables[4 * 1024];
    uint8_t palette[32];

    uint8_t screen_buffer[AGNES_SCREEN_HEIGHT * AGNES_SCREEN_WIDTH];

    int scanline;
    int dot;

    uint8_t ppudata_buffer;
    uint8_t last_reg_write;

    struct {
        uint16_t v;
        uint16_t t;
        uint8_t x;
        uint8_t w;
    } regs;

    struct {
        bool show_leftmost_bg;
        bool show_leftmost_sprites;
        bool show_background;
        bool show_sprites;
    } masks;

    uint8_t nt;
    uint8_t at;
    uint8_t at_latch;
    uint16_t at_shift;
    uint8_t bg_hi;
    uint8_t bg_lo;
    uint16_t bg_hi_shift;
    uint16_t bg_lo_shift;

    struct {
        uint16_t addr_increment;
        uint16_t sprite_table_addr;
        uint16_t bg_table_addr;
        bool use_8x16_sprites;
        bool nmi_enabled;
    } ctrl;

    struct {
        bool in_vblank;
        bool sprite_overflow;
        bool sprite_zero_hit;
    } status;

    bool is_odd_frame;

    uint8_t oam_address;
    uint8_t oam_data[256];
    sprite_t sprites[8];
    int sprite_ixs[8];
    int sprite_ixs_count;
} ppu_t;

/********************************** MAPPERS **********************************/

typedef enum {
    MIRRORING_MODE_NONE,
    MIRRORING_MODE_SINGLE_LOWER,
    MIRRORING_MODE_SINGLE_UPPER,
    MIRRORING_MODE_HORIZONTAL,
    MIRRORING_MODE_VERTICAL,
    MIRRORING_MODE_FOUR_SCREEN
} mirroring_mode_t;

typedef struct mapper0 {
    struct agnes *agnes;

    unsigned prg_bank_offsets[2];
    bool use_chr_ram;
    uint8_t chr_ram[8 * 1024];
} mapper0_t;

typedef struct mapper1 {
    struct agnes *agnes;

    uint8_t shift;
    int shift_count;
    uint8_t control;
    int prg_mode;
    int chr_mode;
    int chr_banks[2];
    int prg_bank;
    unsigned chr_bank_offsets[2];
    unsigned prg_bank_offsets[2];
    bool use_chr_ram;
    uint8_t chr_ram[8 * 1024];
    uint8_t prg_ram[8 * 1024];
} mapper1_t;

typedef struct mapper2 {
    struct agnes *agnes;

    unsigned prg_bank_offsets[2];
    uint8_t chr_ram[8 * 1024];
} mapper2_t;

typedef struct mapper4 {
    struct agnes *agnes;

    unsigned prg_mode;
    unsigned chr_mode;
    bool irq_enabled;
    int reg_ix;
    uint8_t regs[8];
    uint8_t counter;
    uint8_t counter_reload;
    unsigned chr_bank_offsets[8];
    unsigned prg_bank_offsets[4];
    uint8_t prg_ram[8 * 1024];
    bool use_chr_ram;
    uint8_t chr_ram[8 * 1024];
} mapper4_t;

/********************************* GAMEPACK **********************************/

typedef struct {
    const uint8_t *data;
    unsigned prg_rom_offset;
    unsigned chr_rom_offset;
    int prg_rom_banks_count;
    int chr_rom_banks_count;
    bool has_prg_ram;
    unsigned char mapper;
} gamepack_t;

/******************************** CONTROLLER *********************************/

typedef struct controller {
    uint8_t state;
    uint8_t shift;
} controller_t;

/*********************************** AGNES ***********************************/
typedef struct agnes {
    cpu_t cpu;
    ppu_t ppu;
    uint8_t ram[2 * 1024];
    gamepack_t gamepack;
    controller_t controllers[2];
    bool controllers_latch;

    union {
        mapper0_t m0;
        mapper1_t m1;
        mapper2_t m2;
        mapper4_t m4;
    } mapper;

    mirroring_mode_t mirroring_mode;
} agnes_t;

typedef struct agnes_state {
    agnes_t agnes;
} agnes_state_t;

agnes_t* agnes_make(void);
void agnes_destroy(agnes_t *agn);
bool agnes_load_ines_data(agnes_t *agnes, void *data, size_t data_size);
void agnes_set_input(agnes_t *agnes, const agnes_input_t *input_1, const agnes_input_t *input_2);
size_t agnes_state_size(void);
void agnes_dump_state(const agnes_t *agnes, agnes_state_t *out_res);
bool agnes_restore_state(agnes_t *agnes, const agnes_state_t *state);
bool agnes_tick(agnes_t *agnes, bool *out_new_frame);
bool agnes_next_frame(agnes_t *agnes);

agnes_color_t agnes_get_screen_pixel(const agnes_t *agnes, int x, int y);

static void* agnes_read_file(const char *filename, size_t *out_len);
bool agnes_load_ines_data_from_path(agnes_t *agnes, const char *filename);

void exit(int);
]]

local libnes = ffi.load("nes")

state = {}

function love.load()
  state.agnes = libnes.agnes_make()
  local ok = libnes.agnes_load_ines_data_from_path(state.agnes, "roms/mario.nes")
  if not ok then
    love.graphics.print("mario.nes load failed!!!", 50, 10)
  end
  state.input = ffi.new("agnes_input_t")
end


function love.update(dt)
  state.input.a      = love.keyboard.isDown("z")
  state.input.b      = love.keyboard.isDown("x")
  state.input.select = love.keyboard.isDown("backspace")
  state.input.start  = love.keyboard.isDown("return")
  state.input.up     = love.keyboard.isDown("up")
  state.input.down   = love.keyboard.isDown("down")
  state.input.left   = love.keyboard.isDown("left")
  state.input.right  = love.keyboard.isDown("right")
  
  libnes.agnes_set_input(state.agnes, state.input, null);
  local ok = libnes.agnes_next_frame(state.agnes);
  if not ok then
    love.graphics.print("Getting next frame failed!!!", 50, 10)
  end
end

function love.keypressed()
	-- ffi.C.exit(666)
end

function love.draw()
	-- love.graphics.print("Press a key to quit", 50, 50)
  for j = 1, 240 do
    for i = 1, 256 do
      local color = libnes.agnes_get_screen_pixel(state.agnes, (i-1), (j-1));
      love.graphics.setColor(color.r/255.0, color.g/255.0, color.b/255.0)
      love.graphics.points({i,j}) -- only draw one point
    end
  end
end