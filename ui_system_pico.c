#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/bootrom.h"
// note : 0-9 A-F(10-15)    A-B-C-D-E-F

/*lcd commands :
  init :
  0x01 -> reset 0x11 -> wake the lcd  0x29 -> display on
  0x3A -> color mode 0x55 -> rgb 16bit 0x66 -> rgb 18bit
  pixel :
  0x2A -> column address set (x range for pixle writes)
  0x2B -> page address set (y range for pixle writes)
  0X2C -> memory write

  scrolling :
  0x33 -> vertical scroll definition
  0x37 -> vertical scroll start address


  more stuff :
  0x36 memory access control
  0x21 / 0x20 -> display mode
  0x12 / 0x13 -> partial display (on/off)


*/

// lcd pins
#define CS 17
#define DC 22
#define RST 15

#define SPI_PORT spi0
#define MOSI 19
#define MISO 16
#define SCK 18

// button pins
#define BTN_BOOTSEL 7
#define BTN_A 8
#define BTN_B 9
// screen size
#define Sx 320
#define Sy 480

// font
#define FONT_Scale 3
/*
static const uint8_t font5x7[6][5] = {
    {0b0011100,
     0b0100010,
     0b0111110,
     0b0100010,
     0b0100010}, // A

    {0b0111100,
     0b0100010,
     0b0111100,
     0b0100010,
     0b0111100}, // B

    {0b0011100,
     0b0100010,
     0b0100000,
     0b0100010,
     0b0011100}, // c

    {0b0111100,
     0b0100010,
     0b0100010,
     0b0100010,
     0b0111100}, // D

    {0b0111110,
     0b0100000,
     0b0111110,
     0b0100000,
     0b0111110}, // E

    {0b0111110,
     0b0100000,
     0b0111110,
     0b0100000,
     0b0100000}, // F
};
*/

// ASCII 32..127  (96 chars)
static const uint8_t font5x7_ascii[96][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /

    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9

    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @

    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O

    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z

    {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `

    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o

    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z

    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}, // ~
    {0x00, 0x06, 0x09, 0x09, 0x06}  // DEL
};

void button_init()
{
    gpio_init(BTN_BOOTSEL);
    gpio_set_dir(BTN_BOOTSEL, GPIO_IN);
    gpio_pull_up(BTN_BOOTSEL);

    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
}

// lcd :
void lcd_cmd(uint8_t cmd)
{
    gpio_put(DC, 0);
    gpio_put(CS, 0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(CS, 1);
}

void lcd_data(uint8_t data)
{
    gpio_put(DC, 1);
    gpio_put(CS, 0);
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(CS, 1);
}

// Write a buffer while keeping CS asserted for the entire transfer — required
// by many LCD controllers for correct pixel streaming and to avoid byte
// misalignment/artifacts when sending multi-byte pixel data.
void lcd_write(const uint8_t *buf, size_t len)
{
    gpio_put(DC, 1);
    gpio_put(CS, 0);
    spi_write_blocking(SPI_PORT, buf, len);
    gpio_put(CS, 1);
}

void lcd_reset()
{
    gpio_put(RST, 0);
    sleep_ms(50);
    gpio_put(RST, 1);
    sleep_ms(200);
}
// sets the changed window and sends the command for data (0x2c)
void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_cmd(0x2A);
    lcd_data(x0 >> 8);
    lcd_data(x0 & 0xFF);
    lcd_data(x1 >> 8);
    lcd_data(x1 & 0xFF);

    lcd_cmd(0x2B);
    lcd_data(y0 >> 8);
    lcd_data(y0 & 0xFF);
    lcd_data(y1 >> 8);
    lcd_data(y1 & 0xFF);

    lcd_cmd(0x2C);
}
static inline void rgb565(uint8_t r, uint8_t g, uint8_t b, uint8_t *hi, uint8_t *lo)
{
    *hi = (r & 0xF8) | (g >> 5);
    *lo = ((g & 0x1C) << 3) | (b >> 3);
}
void lcd_clear(uint8_t r, uint8_t g, uint8_t b)
{
    // Column address set (x start, x end)
    lcd_cmd(0x2A);
    lcd_data(0x00);
    lcd_data(0x00);
    lcd_data((Sx - 1) >> 8);
    lcd_data((Sx - 1) & 0xFF);

    // Page address set (y start, y end)
    lcd_cmd(0x2B);
    lcd_data(0x00);
    lcd_data(0x00);
    lcd_data((Sy - 1) >> 8);
    lcd_data((Sy - 1) & 0xFF);

    lcd_cmd(0x2C);

    // Build one scanline in RGB565 and stream it per-line. Keeps memory use small
    // while ensuring CS stays asserted for each scanline transfer.
    static uint8_t linebuf[2 * Sx];
    uint8_t hi;
    uint8_t lo;
    rgb565(r, g, b, &hi, &lo);
    for (int x = 0, idx = 0; x < Sx; ++x)
    {
        linebuf[idx++] = hi;
        linebuf[idx++] = lo;
    }

    for (int y = 0; y < Sy; ++y)
    {
        lcd_write(linebuf, sizeof(linebuf));
    }
}

void lcd_init()
{
    lcd_reset();

    lcd_cmd(0x11); // wake up
    sleep_ms(200);

    lcd_cmd(0x3A);
    lcd_data(0x55); // RGB565 (16-bit: R5 G6 B5) - matches controller's expected 2-byte pixel format

    lcd_cmd(0x36); // memory access control
    // MADCTL bits: [7 MY][6 MX][5 MV][4 ML][3 RGB][2 MH][1:0 - unused]
    // bit3 = 0 -> RGB order, bit3 = 1 -> BGR order
    lcd_data(0x08);     // BGR order
    lcd_cmd(0x28);      // display off
    lcd_clear(0, 0, 0); // black
}

void spi_init_()
{
    spi_init(SPI_PORT, 30000000); //  30 MHz

    gpio_set_function(MOSI, GPIO_FUNC_SPI);
    gpio_set_function(MISO, GPIO_FUNC_SPI);
    gpio_set_function(SCK, GPIO_FUNC_SPI);

    gpio_init(CS);
    gpio_init(DC);
    gpio_init(RST);

    gpio_set_dir(CS, GPIO_OUT);
    gpio_set_dir(DC, GPIO_OUT);
    gpio_set_dir(RST, GPIO_OUT);

    gpio_put(CS, 1);
}
// draw stuff
void draw_pixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x >= Sx || y >= Sy)
        return;

    // Column address set
    lcd_cmd(0x2A);

    lcd_data(x >> 8);   // start high
    lcd_data(x & 0xFF); // start low

    lcd_data(x >> 8);   // end high
    lcd_data(x & 0xFF); // end low

    // Page address set
    lcd_cmd(0x2B);
    lcd_data(y >> 8);
    lcd_data(y & 0xFF);
    lcd_data(y >> 8);
    lcd_data(y & 0xFF);

    // Memory write
    lcd_cmd(0x2C);
    /* map 8-bit (r,g,b) -> RGB565 */
    uint8_t hi;
    uint8_t lo;
    rgb565(r, g, b, &hi, &lo);
    uint8_t pix[2] = {hi, lo};
    lcd_write(pix, 2);
}

// Fill a rectangle using RGB565 (keeps CS low for each line)
void fill_rect_rgb565(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint8_t r, uint8_t g, uint8_t b)
{
    if (x0 >= Sx || y0 >= Sy)
        return;
    if (x0 + w > Sx)
        w = Sx - x0;
    if (y0 + h > Sy)
        h = Sy - y0;

    lcd_set_window(x0, y0, x0 + w - 1, y0 + h - 1);

    static uint8_t linebuf[2 * Sx];
    uint8_t hi;
    uint8_t lo;
    rgb565(r, g, b, &hi, &lo);
    for (int x = 0, idx = 0; x < w; ++x)
    {
        linebuf[idx++] = hi;
        linebuf[idx++] = lo;
    }

    for (int y = 0; y < h; ++y)
        lcd_write(linebuf, w * 2);
}

void draw_char(char c, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t scale, uint8_t br, uint8_t bg, uint8_t bb)
{
    if (c < 32 || c > 127)
        c = '?';

    const uint8_t *bitmap = font5x7_ascii[c - 32];

    uint16_t w = 5 * scale;
    uint16_t h = 7 * scale;

    // rotated dimensions
    uint16_t rw = h;
    uint16_t rh = w;

    uint8_t fg_hi = (r & 0xF8) | (g >> 5);
    uint8_t fg_lo = ((g & 0x1C) << 3) | (b >> 3);

    uint8_t bg_hi = (br & 0xF8) | (bg >> 5);
    uint8_t bg_lo = ((bg & 0x1C) << 3) | (bb >> 3);

    static uint8_t buf[7 * 5 * FONT_Scale * FONT_Scale * 2];

    // clear buffer
    memset(buf, 0, sizeof(buf));

    for (int row = 0; row < 7; row++)
    {
        for (int col = 0; col < 5; col++)
        {
            bool set = bitmap[col] & (1 << (6 - row));

            for (int sy = 0; sy < scale; sy++)
            {
                for (int sx = 0; sx < scale; sx++)
                {
                    int px = col * scale + sx;
                    int py = row * scale + sy;

                    // rotate 90°
                    int rx = h - 1 - py;
                    int ry = px;

                    int idx = (ry * rw + rx) * 2;

                    if (set)
                    {
                        buf[idx] = fg_hi;
                        buf[idx + 1] = fg_lo;
                    }
                    else // Set background pixel
                    {
                        buf[idx] = bg_hi;     // High byte for background color
                        buf[idx + 1] = bg_lo; // Low byte for background color
                    }
                }
            }
        }
    }

    lcd_set_window(x, y, x + rw - 1, y + rh - 1);
    lcd_write(buf, rw * rh * 2);
}

void draw_string(char *str, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t scale, uint8_t br, uint8_t bg, uint8_t bb)
{
    int space = FONT_Scale * 7;
    int xx = (((FONT_Scale + 2) * 7) / 2) - (space / 2);

    for (int i = 0; i <= strlen(str) - 1; i++)
    {
        draw_char(str[i], x + xx, y + space * i, r, g, b, scale, br, bg, bb);
    }
}

void button_test(int *scroll, int scrollable_items)
{
    //  printf("Scroll value: %d\n", *scroll);

    if (gpio_get(BTN_A) == 0) // Active low
    {
        printf("BTN_A pressed!\n");
        if (*scroll >= 1)
        {
            *scroll = *scroll + 1;
        }
        if (*scroll > scrollable_items)
        {
            *scroll = 1;
        }
        sleep_ms(200); // Debounce delay
    }
    if (gpio_get(BTN_B) == 0) // Active low
    {
        if (*scroll > 1)
        {
            *scroll = *scroll - 1;
        }
        else if (*scroll <= 1)
        {
            *scroll = scrollable_items;
        }

        printf("BTN_B pressed!\n");
        sleep_ms(200); // Debounce delay
    }
    if (gpio_get(BTN_BOOTSEL) == 0)
    {
        reset_usb_boot(0, 0);
    }
    sleep_ms(50); // Polling delay
}

void draw_menu(int selected, char items[][20], int item_count, int *previous)
{
    static int initialized = 0;
    int space = (FONT_Scale * 12);
    int xx = (((FONT_Scale + 2) * 7) / 2) - (space / 2);
    if (!initialized)
    {
        initialized = 1;
        for (int i = 1; i <= item_count; i++)
        {
            // printf("Drawing item %d: %s\n", i, items[i-1]);
            if (i == selected)
            {
                fill_rect_rgb565(0 + space * (i - 1), 0, 0 + ((FONT_Scale + 2) * 7), Sy, 0xff, 0xff, 0xff);
                draw_string(items[i - 1], 0 + space * (i - 1), 5, 0, 0, 0, FONT_Scale, 255, 255, 255);
            }
            else
            {
                fill_rect_rgb565(0 + space * (i - 1), 0, 0 + ((FONT_Scale + 2) * 7), Sy, 0, 0, 0);
                draw_string(items[i - 1], 0 + space * (i - 1), 5, 255, 255, 255, FONT_Scale, 0, 0, 0);
            }
        }
    }
    else
    {
        // only redraw changed items
        if (*previous != selected)
        {
            // redraw previous
            fill_rect_rgb565(0 + space * (*previous - 1), 0, 0 + ((FONT_Scale + 2) * 7), Sy, 0, 0, 0);
            draw_string(items[*previous - 1], 0 + space * (*previous - 1), 5, 255, 255, 255, FONT_Scale, 0, 0, 0);

            // draw selected
            fill_rect_rgb565(0 + space * (selected - 1), 0, 0 + ((FONT_Scale + 2) * 7), Sy, 0xff, 0xff, 0xff);
            draw_string(items[selected - 1], 0 + space * (selected - 1), 5, 0, 0, 0, FONT_Scale, 255, 255, 255);
        }
    }
}

int main()
{
    int space = FONT_Scale * 7;
    int xx = (((FONT_Scale + 2) * 7) / 2) - (space / 2);
    int scrollabe_items = 1;
    stdio_init_all();
    spi_init_();
    lcd_init();
    button_init();

    space = 0;
    xx = 0;
    int save = 1;
    draw_menu(scrollabe_items, (char[][20]){"Install", "Version", "Games", "Settings"}, 4, &save);
    lcd_cmd(0x29); // display on
    while (1)
    {
        if (scrollabe_items != save)
        {
            draw_menu(scrollabe_items, (char[][20]){"Install", "Version", "Games", "Settings"}, 4, &save);
            save = scrollabe_items;
        }

        button_test(&scrollabe_items, 4);
    }

    return 0;
}
