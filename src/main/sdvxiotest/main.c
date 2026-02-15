#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include "bemanitools/sdvxio.h"

#include "util/log.h"
#include "util/thread.h"

static uint16_t
check_assign_gpio(uint16_t input, size_t idx_in, size_t gpio_out)
{
    if ((input >> idx_in) & 1) {
        return 1 << gpio_out;
    }

    return 0;
}

static void _all_lights_off_shutdown()
{
    // max of 18 channels
    for (int i = 0; i < 18; i++) {
        sdvx_io_set_pwm_light(i, 0x00);
    }

    sdvx_io_set_gpio_lights(0);

    // push it a few times to make sure.
    for (uint8_t i = 0; i < 3; i++) {
        sdvx_io_write_output();
    }
}

/**
 * Tool to test your implementations of sdvxio.
 */
int main(int argc, char **argv)
{
    log_to_writer(log_writer_stdout, NULL);

    sdvx_io_set_loggers(
        log_impl_misc, log_impl_info, log_impl_warning, log_impl_fatal);

    if (!sdvx_io_init(crt_thread_create, crt_thread_join, crt_thread_destroy)) {
        printf("Initializing sdvxio failed\n");
        return -1;
    }

    printf(">>> Initializing sdvxio successful, press enter to continue <<<\n");

    if (getchar() != '\n') {
        return 0;
    }

    /* inputs */
    uint8_t input_gpio_sys = 0;
    uint16_t input_gpio[2] = {0};
    uint16_t spinner_pos[2] = {0};

    /* outputs */
    uint32_t gpio_lights = 0;
    uint8_t wingUpper[3] = {0};
    uint8_t wingLower[3] = {0};
    uint8_t woofer[3] = {0};
    uint8_t controller[3] = {0};

    bool loop = true;
    uint8_t cnt = 0;
    bool all_on = false;
    while (loop) {
        /* get inputs */
        if (!sdvx_io_read_input()) {
            printf("ERROR: reading inputs failed\n");
            return -4;
        }

        input_gpio_sys = sdvx_io_get_input_gpio_sys();

        input_gpio[0] = sdvx_io_get_input_gpio(0);
        input_gpio[1] = sdvx_io_get_input_gpio(1);

        spinner_pos[0] = sdvx_io_get_spinner_pos(0);
        spinner_pos[1] = sdvx_io_get_spinner_pos(1);

        system("cls");
        printf(
            "%02x %04x %04x %04x %04x\n",
            input_gpio_sys,
            input_gpio[0],
            input_gpio[1],
            spinner_pos[0],
            spinner_pos[1]);

        /* set outputs */
        if (all_on) {
            memset(wingUpper, 0xFF, sizeof(wingUpper));
            memset(wingLower, 0xFF, sizeof(wingLower));
            memset(woofer, 0xFF, sizeof(woofer));
            memset(controller, 0xFF, sizeof(controller));
            gpio_lights = UINT32_MAX;
        }

        /* light up keys when pressed */
        if (!all_on) {
            gpio_lights = 0;
            
            // reactive lighting for buttons
            gpio_lights |= check_assign_gpio(
                input_gpio[0], SDVX_IO_IN_GPIO_0_START, SDVX_IO_OUT_GPIO_START);
            gpio_lights |= check_assign_gpio(
                input_gpio[0], SDVX_IO_IN_GPIO_0_A, SDVX_IO_OUT_GPIO_A);
            gpio_lights |= check_assign_gpio(
                input_gpio[0], SDVX_IO_IN_GPIO_0_B, SDVX_IO_OUT_GPIO_B);
            gpio_lights |= check_assign_gpio(
                input_gpio[0], SDVX_IO_IN_GPIO_0_C, SDVX_IO_OUT_GPIO_C);
            gpio_lights |= check_assign_gpio(
                input_gpio[1], SDVX_IO_IN_GPIO_1_D, SDVX_IO_OUT_GPIO_D);
            gpio_lights |= check_assign_gpio(
                input_gpio[1], SDVX_IO_IN_GPIO_1_FX_L, SDVX_IO_OUT_GPIO_FX_L);
            gpio_lights |= check_assign_gpio(
                input_gpio[1], SDVX_IO_IN_GPIO_1_FX_R, SDVX_IO_OUT_GPIO_FX_R);
        } else {
            // when button is pressed, disable "all on"
            if (input_gpio[0] > 0 || input_gpio[1] > 0) {
                all_on = false;

                gpio_lights = 0;
                memset(wingUpper, 0x00, sizeof(wingUpper));
                memset(wingLower, 0x00, sizeof(wingLower));
                memset(woofer, 0x00, sizeof(woofer));
                memset(controller, 0x00, sizeof(controller));
            }
        }

        // push the lighting state.
        sdvx_io_set_pwm_light(0x00, wingUpper[0]);
        sdvx_io_set_pwm_light(0x01, wingUpper[1]);
        sdvx_io_set_pwm_light(0x02, wingUpper[2]);

        sdvx_io_set_pwm_light(0x03, wingUpper[0]);
        sdvx_io_set_pwm_light(0x04, wingUpper[1]);
        sdvx_io_set_pwm_light(0x05, wingUpper[2]);

        sdvx_io_set_pwm_light(0x06, wingLower[0]);
        sdvx_io_set_pwm_light(0x07, wingLower[1]);
        sdvx_io_set_pwm_light(0x08, wingLower[2]);

        sdvx_io_set_pwm_light(0x09, wingLower[0]);
        sdvx_io_set_pwm_light(0x0A, wingLower[1]);
        sdvx_io_set_pwm_light(0x0B, wingLower[2]);

        sdvx_io_set_pwm_light(0x0C, woofer[0]);
        sdvx_io_set_pwm_light(0x0D, woofer[1]);
        sdvx_io_set_pwm_light(0x0E, woofer[2]);

        sdvx_io_set_pwm_light(0x0F, controller[0]);
        sdvx_io_set_pwm_light(0x10, controller[1]);
        sdvx_io_set_pwm_light(0x11, controller[2]);

        sdvx_io_set_gpio_lights(gpio_lights);

        if (!sdvx_io_write_output()) {
            printf("ERROR: writing lights failed\n");
            return -4;
        }

        /* avoid CPU banging */
        Sleep(5);
        ++cnt;

        /* process menu */
        if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
            system("cls");
            Sleep(5);
            printf(
                "Menu options:\n"
                "  0: Exit menu and continue loop\n"
                "  1: Exit\n"
                "  2: Set all outputs on\n"
                "  3: Clear all outputs\n");

            char c = getchar();

            switch (c) {
                case '1': {
                    loop = false;
                    break;
                }

                case '2': {
                    all_on = true;
                    break;
                }

                case '3': {
                    all_on = false;
                    break;
                }

                case '0':
                default:
                    break;
            }
        }
    }

    system("cls");

    _all_lights_off_shutdown();

    sdvx_io_fini();

    return 0;
}