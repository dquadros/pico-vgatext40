/*
    Generates 15 lines x 40 columns alphanumeric video

    (C) 2021, Daniel Quadros
*/

#include "pico.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "pico/stdlib.h"
#include "font.h"

// Video mode
#define VGA_MODE vga_mode_320x240_60

// The alphanumeric screen
// For each character there is an attribute byte defining fore and background colors
#define NLINES 15
#define NCOLUMS 40
static uint8_t text_screen[NLINES*NCOLUMS];
static uint8_t attr_screen[NLINES*NCOLUMS];

// 16 color pallet
#define PICO_COLOR_FROM_RGB5(r, g, b)  ((r & 0x1F) | ((g & 0x1F) << 6) | ((b & 0x1F) << 11))
static uint32_t pallet[] = {
    PICO_COLOR_FROM_RGB5(0, 0, 0),
    PICO_COLOR_FROM_RGB5(0, 0, 15),
    PICO_COLOR_FROM_RGB5(0, 15, 0),
    PICO_COLOR_FROM_RGB5(0, 15, 15),
    PICO_COLOR_FROM_RGB5(15, 0, 0),
    PICO_COLOR_FROM_RGB5(15, 0, 15),
    PICO_COLOR_FROM_RGB5(15, 15, 0),
    PICO_COLOR_FROM_RGB5(15, 15, 15),
    PICO_COLOR_FROM_RGB5(7, 7, 7),
    PICO_COLOR_FROM_RGB5(0, 0, 31),
    PICO_COLOR_FROM_RGB5(0, 31, 0),
    PICO_COLOR_FROM_RGB5(0, 31, 31),
    PICO_COLOR_FROM_RGB5(31, 0, 0),
    PICO_COLOR_FROM_RGB5(31, 0, 31),
    PICO_COLOR_FROM_RGB5(31, 31, 0),
    PICO_COLOR_FROM_RGB5(31, 31, 31)
};

// Semaphore to hold rendering core until setup is complete
// (not reallyt necessary, as we are using just one core)
struct semaphore video_setup_complete;

// Local rotines
static void render_loop(void);
static void frame_update_logic(void);
static void render_scanline(struct scanvideo_scanline_buffer *dest);

// Main Program
int main(void) {

    // Fill alpha screen
    for (int pos = 0; pos < NLINES*NCOLUMS; pos++) {
        text_screen[pos] = (pos % 95) + 0x20;
        attr_screen[pos] = 0x17;    // White on Blue
    }

    // Initialize video
    sem_init(&video_setup_complete, 0, 1);
    scanvideo_setup(&VGA_MODE);
    scanvideo_timing_enable(true);
    sem_release(&video_setup_complete);

    // Execute rendering forever 
    render_loop();

    // Should never be reached
    return 0;
}

// Loop forever rendering the scanlines
static void render_loop() {
   static uint32_t last_frame_num = 0;

    while (true) {
        // Get the scanline buffer, will block until one is available
        struct scanvideo_scanline_buffer *scanline_buffer = scanvideo_begin_scanline_generation(true);

        // Test if it is a new frame
        uint32_t frame_num = scanvideo_frame_number(scanline_buffer->scanline_id);
        if (frame_num != last_frame_num) {
            // Start of a new frame
            last_frame_num = frame_num;
            frame_update_logic();
        }

        // Do the actual rendering
        render_scanline(scanline_buffer);

        // Release the rendered buffer into the wild
        scanvideo_end_scanline_generation(scanline_buffer);
    }
}

// Logic called at the start of a new frame
// (not used so far)
void frame_update_logic() {
}

// Render a scanline
void render_scanline(struct scanvideo_scanline_buffer *dest) {
    uint32_t *buf = dest->data;
    int l = scanvideo_scanline_number(dest->scanline_id);
    int off_line = (l >> 4)*NCOLUMS;
    int cline = l & 0x0F;
    int pos = 0;
    int c, dot;
    uint32_t fgcolor, bgcolor, pixel1, pixel2;

    // | RAW_RUN | COLOR1 | N-3 | COLOR2 | COLOR3 …​ | COLOR(N) |
    for (c = 0; c < NCOLUMS; c++) {
        uint8_t c1 = text_screen[off_line+c];
        if ((c1 >= 0x20) && (c1 < 0x7F)) {
            c1 = font [((c1-0x20)<<4)+cline];
        } else {
            c1 = 0x00;
        }
        fgcolor = pallet[attr_screen[off_line+c] & 0x0F];
        bgcolor = pallet[attr_screen[off_line+c] >> 4];
        for (dot = 0; dot < 8; dot += 2) {
            pixel1 = pixel2 = bgcolor;
            if (c1 & 0x80) {
                pixel1 = fgcolor;
            }
            if (c1 & 0x40) {
                pixel2 = fgcolor;
            }
            if (pos == 0) {
                buf[pos++] = COMPOSABLE_RAW_RUN | (pixel1 << 16);
                buf[pos++] = (NCOLUMS*8-3) | (pixel2 << 16);
            } else {
                buf[pos++] = pixel1 | (pixel2 << 16);
            }
            c1 = c1 << 2;
        }
    }
    buf[pos-1] &= 0xFFFF;   // last pixel in line MUST be black (or bad things happen!)
    buf[pos++] = COMPOSABLE_EOL_SKIP_ALIGN;
    dest->data_used = pos;
    dest->status = SCANLINE_OK;
}
