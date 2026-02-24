#include "cmsis_gcc.h"
#include "hardware/gpio.h"
#include "hardware/structs/m0plus.h"
#include "pico/stdlib.h"

#define SELECT_PIN_MASK (1 << 22 | 1 << 23 | 1 << 24 | 1 << 25)

// DIP switch setting for each app.
#define APP1_PIN_STATE (0b0001 << 22)
#define APP2_PIN_STATE (0b0010 << 22)
#define APP3_PIN_STATE (0b0011 << 22)
#define APP4_PIN_STATE (0b0100 << 22)
#define APP5_PIN_STATE (0b0101 << 22)
#define APP6_PIN_STATE (0b0110 << 22)

typedef void (*app_entry_t)(void);

void jump_to_app(uint32_t app_address) {
    // The first word at the app's address is the initial stack pointer.
    // The second word is the address of the reset handler (the entry point).
    uint32_t* app_vectors = (uint32_t*) app_address;
    uint32_t app_sp = app_vectors[0];
    app_entry_t app_entry = (app_entry_t) app_vectors[1];

    __disable_irq();

    // Set the Vector Table Offset Register (VTOR) to the app's vector table.
    ppb_hw->vtor = app_address;

    // Set the main stack pointer (MSP) to the app's stack pointer.
    __set_MSP(app_sp);

    __enable_irq();

    // Jump to the application's entry point.
    app_entry();

    // Never reached.
    while (1) {
        tight_loop_contents();
    }
}

int main() {
    gpio_init_mask(SELECT_PIN_MASK);
    gpio_set_dir_in_masked(SELECT_PIN_MASK);
    for (uint8_t i = 0; i < 32; i++) {
        if ((SELECT_PIN_MASK >> i) & 1) {
            gpio_pull_up(i);
        }
    }

    // Wait for the pull-ups to stabilize.
    sleep_ms(10);

    uint32_t target_app_address;

    uint32_t pin_state = (~gpio_get_all()) & SELECT_PIN_MASK;

    if (pin_state == APP1_PIN_STATE) {
        target_app_address = APP1_START_ADDRESS;
    } else if (pin_state == APP2_PIN_STATE) {
        target_app_address = APP2_START_ADDRESS;
    } else if (pin_state == APP3_PIN_STATE) {
        target_app_address = APP3_START_ADDRESS;
    } else if (pin_state == APP4_PIN_STATE) {
        target_app_address = APP4_START_ADDRESS;
    } else if (pin_state == APP5_PIN_STATE) {
        target_app_address = APP5_START_ADDRESS;
    } else if (pin_state == APP6_PIN_STATE) {
        target_app_address = APP6_START_ADDRESS;
    } else {
        target_app_address = APP1_START_ADDRESS;
    }

    for (uint8_t i = 0; i < 32; i++) {
        if ((SELECT_PIN_MASK >> i) & 1) {
            gpio_disable_pulls(i);
            gpio_deinit(i);
        }
    }

    jump_to_app(target_app_address);

    return 0;  // Never reached.
}
