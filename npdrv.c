#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "pico/stdlib.h"
#include "pico/sem.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "neopixel.pio.h"

#define IS_RGBW false
#define FRAC_BITS 4
#define NUM_PIXELS 8
#define WS2812_PIN_BASE 0

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

static inline void put_all(uint32_t col[NUM_PIXELS]) {
    for(int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(col[i]);
    }
}
uint32_t all_zero[NUM_PIXELS] = {0};

static inline void blink_all(uint32_t col, int reps, int delay) {
    
    uint32_t all_col[NUM_PIXELS] = { 0 };
    for (int i = 0; i < NUM_PIXELS; i++) {
        all_col[i] = col;
    }
    for(int j = 0; j < reps; j++) {
        for(int i = 0; i < NUM_PIXELS; i++) {
            put_all(all_zero);
        }
        sleep_ms(delay);
        for(int i = 0; i < NUM_PIXELS; i++) {
            put_all(all_col);
        }
        sleep_ms(delay);
    }
}

static inline void audi_blinker(uint32_t col) {
    uint32_t current_cols[NUM_PIXELS] = {0};
    for(int i = 0; i < NUM_PIXELS; i++) {
        memset(current_cols, 0, NUM_PIXELS * sizeof *current_cols);
        for(int j = 0; j <= i; j++) {
            current_cols[j] = col;
        }
        put_all(current_cols);
        sleep_ms(150);
    }
    for(int i = 0; i < NUM_PIXELS; i++) {
        current_cols[i] = col;
    }
    put_all(all_zero);
    sleep_ms(150);
}

uint32_t party_mode_current_cols[NUM_PIXELS] = {
        0x00ff00,
        0xffff00,
        0xff00ff,
        0x0000ff,
        0x00ffff,
        0xffff00,
        0xff0000,
        0xff00ff
    };

static inline void party_mode(void) {
    
    for(int i = 0; i < NUM_PIXELS; i++) {
        put_all(party_mode_current_cols);
        sleep_ms(10);
    }

    // rotate all pixels
    uint32_t tmp[NUM_PIXELS] = {0};
    for(int i = 0; i < NUM_PIXELS-1; i++) {
        tmp[i] = party_mode_current_cols[i+1];
    }
    tmp[NUM_PIXELS-1] = party_mode_current_cols[0];

    memcpy(party_mode_current_cols, tmp, sizeof party_mode_current_cols);
}

// state machine:
// idle -> any
// audi_blinker -> idle, notify
// notify -> idle
// party -> idle, notify
// raw -> idle
// change state on serial read:
// n (i32, i32, i32) -> notification (color, blink speed, repetitions)
// a -> audi mode
// i -> immediately cancel and go to idle mode
// p -> party mode
// r -> raw mode
typedef enum pixel_mode {
    IDLE,
    AUDI_BLINKER,
    NOTIFY,
    PARTY,
    RAW
} pixel_mode_t;

static inline uint32_t rgb_to_grb(uint32_t rgb) {
    uint8_t r = rgb >> 16;
    uint8_t g = rgb >> 8;
    uint8_t b = rgb;
    return (
        (g << 16) |
        (r << 8) |
        b
    );
}

// raw mode:
// every byte received is immediately passed straight through to the next pixel;
// if no bytes are received for 1 second, we go back into idle mode

int main() {
    // initialise serial interface (over USB or UART depending on compile-time configuration)
    stdio_init_all();

    // use pio pin 0 as data tx pin for neopixel data
    PIO pio = pio0;
    int sm = 0;
    // where the program is loaded
    uint offset = pio_add_program(pio, &neopixel_program);

    printf("Using pin %d\n", WS2812_PIN_BASE);

    puts("Initialising...");
    neopixel_program_init(pio, sm, offset, WS2812_PIN_BASE, 800000, IS_RGBW);

    int t = 0;

    puts("done");
    sleep_ms(1000);

    uint32_t col = 0x0;
    int pixel_count = 0;
    uint32_t pixels[NUM_PIXELS];
    int last_byte_received_timestamp = 0;
    pixel_mode_t current_mode = IDLE;

    // main loop
    while(true) {
MAIN_LOOP:
        switch(current_mode) {
        case IDLE: {
            // whenever we are in idle mode, set all pixels to black
            put_all(all_zero);
            int c = getchar();
            switch (c) {
                case 'r': {
                    printf("Entering RAW mode\n");
                    current_mode = RAW;
                    goto MAIN_LOOP;
                }
            }
        }
        case RAW: {
                int col = 0;
                for( int i = 0; i < 3; i++ ) {
                    int c = getchar_timeout_us(1000000);
                    if (c < 0) {
                        printf("Exiting RAW mode, returning to IDLE\n");
                        current_mode = IDLE;
                        pixel_count = 0; // reset pixel count as well, for next raw mode
                        memset(pixels, 0, sizeof pixels); // reset all pixels
                        goto MAIN_LOOP;
                    }
                    col |= (c << ((2-i) * 8));
                    printf("Received pixel %0.6x (RGB)\n", col);
                }
                col = rgb_to_grb(col);
                printf("Setting next pixel to %0.6x (GRB)\n", col);
                pixels[pixel_count++] = col;
                if (pixel_count == NUM_PIXELS) {
                    put_all(pixels);
                    pixel_count = 0;
                    sleep_ms(15); // next frame
                }
                break;
            }
        }
    }
    
    // unreachable (but required by C standard)
    return 0;
}