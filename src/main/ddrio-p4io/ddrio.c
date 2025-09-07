#include <windows.h>

#include <stdatomic.h>
#include <stdlib.h>

#include "bemanitools/ddrio.h"
#include "bemanitools/input.h"

#include "cconfig/cconfig-main.h"

#include "imports/avs.h"

#include "util/defs.h"
#include "util/log.h"

#include "config.h"

#include "aciodrv/device.h"
#include "aciodrv/mdxf.h"
#include "p4iodrv/device.h"

#include <stdio.h>

static struct p4iodrv_ctx *p4io_ctx;
static struct aciodrv_device_ctx *mdxf_device;

uint8_t light_buff[16] = {0};
uint8_t coin_buff[4] = {0};
uint32_t jamma[4] = {0};

struct p4io_bittrans {
    uint32_t p4io;
    uint32_t ddrio;
};

static const struct p4io_bittrans input_map[] = {
    {(1 << 0), 1 << DDR_P1_START},
    {(1 << 1), 1 << DDR_P1_MENU_UP},
    {(1 << 2), 1 << DDR_P2_MENU_DOWN},
    {(1 << 3), 1 << DDR_P1_MENU_LEFT},
    {(1 << 4), 1 << DDR_P1_MENU_RIGHT},

    {(1 << 8), 1 << DDR_P2_START},
    {(1 << 9), 1 << DDR_P2_MENU_UP},
    {(1 << 10), 1 << DDR_P1_MENU_DOWN},
    {(1 << 11), 1 << DDR_P2_MENU_LEFT},
    {(1 << 12), 1 << DDR_P2_MENU_RIGHT},

    {(1 << 24), 1 << DDR_COIN},
    {(1 << 25), 1 << DDR_SERVICE},
    {(1 << 28), 1 << DDR_TEST},

};

void ddr_io_set_loggers(
    log_formatter_t misc,
    log_formatter_t info,
    log_formatter_t warning,
    log_formatter_t fatal)
{
    log_to_external(misc, info, warning, fatal);
}

bool ddr_io_init(
    thread_create_t thread_create,
    thread_join_t thread_join,
    thread_destroy_t thread_destroy)
{
    log_warning("ddr_io_init");

    struct cconfig *config;
    struct p4io_mdxf_config config_mdxf;

    config = cconfig_init();
    ddrio_config_p4io_mdxf_init(config);

    if (!cconfig_main_config_init(
            config,
            "--ddrio-p4io-config",
            "ddrio-p4io.conf",
            "--help",
            "-h",
            "ddrio-p4io",
            CCONFIG_CMD_USAGE_OUT_STDOUT)) {
        cconfig_finit(config);
        exit(EXIT_FAILURE);
    }

    ddrio_config_p4io_mdxf_get(&config_mdxf, config);

    cconfig_finit(config);

    p4io_ctx = p4iodrv_open();

    if (!p4io_ctx) {
        log_warning("Could not open p4io");
        // return false;
    }

    mdxf_device = aciodrv_device_open_path(config_mdxf.port, config_mdxf.baud);

    if (!mdxf_device) {
        log_warning("Opening acio device failed");
        // return false;
    }

    int nodes = aciodrv_device_get_node_count(mdxf_device);

    if (nodes != 2) {
        log_warning("WARNING: only %d MDXF(s) found, expected 2", nodes);
    }

    for (int i = 0; i < nodes; i++) {
        if (!aciodrv_mdxf_init(mdxf_device, i)) {
            log_warning("Opening mdxf device %d failed", i);
            return false;
        }
    }

    return true;
}

uint32_t ddr_io_read_pad(void)
{
    uint32_t pad = 0;

    /* Sleep first: input is timestamped immediately AFTER the ioctl returns.

    Which is the right thing to do, for once. We sleep here because
    the game polls input in a tight loop. Can't complain, at there isn't
    an artificial limit on the poll frequency. */
    Sleep(1);

    // pull the state of the p4io & get the state of the mdxfs
    if (p4io_ctx) {
        memset(jamma, 0, sizeof(jamma));

        if (p4iodrv_read_jamma(p4io_ctx, jamma)) {
            // map the p4io correctly.
            for (int i = 0; i < lengthof(input_map); i++) {
                // for ddrio, all of the inputs are in the first 32bit word of
                // jamma.
                if (jamma[0] & input_map[i].p4io) {
                    pad |= input_map[i].ddrio;
                }
            }
        } else {
            log_warning("Could not read opened p4io");
        }
    }

    struct ac_io_mdxf_poll_in foot_p1 = {0};
    struct ac_io_mdxf_poll_in foot_p2 = {0};

    if (mdxf_device) {
        if (!aciodrv_mdxf_poll(mdxf_device, 0, &foot_p1)) {
            log_warning("Could not read from opened p1 mdxf");
        }

        if (!aciodrv_mdxf_poll(mdxf_device, 1, &foot_p2)) {
            log_warning("Could not read from opened p2 mdxf");
        }

        pad |= (foot_p1.panel.up > 0) ? (1 << DDR_P1_UP) : 0;
        pad |= (foot_p1.panel.down > 0) ? (1 << DDR_P1_DOWN) : 0;
        pad |= (foot_p1.panel.left > 0) ? (1 << DDR_P1_LEFT) : 0;
        pad |= (foot_p1.panel.right > 0) ? (1 << DDR_P1_RIGHT) : 0;

        pad |= (foot_p2.panel.up > 0) ? (1 << DDR_P2_UP) : 0;
        pad |= (foot_p2.panel.down > 0) ? (1 << DDR_P2_DOWN) : 0;
        pad |= (foot_p2.panel.left > 0) ? (1 << DDR_P2_LEFT) : 0;
        pad |= (foot_p2.panel.right > 0) ? (1 << DDR_P2_RIGHT) : 0;
    }

    return pad;
}

void ddr_io_set_lights_extio(uint32_t lights)
{
    // Python4/white cab setup does not have pad lights.

    // TODO: pull the NEON light out and map to the bass lights.

    if (p4io_ctx) {
        p4iodrv_cmd_portout(p4io_ctx, (uint8_t *) &light_buff);
    }
}

void ddr_io_set_lights_p3io(uint32_t lights)
{
    // TODO: map the marquee lights to the RGB of the p4io.

    if (p4io_ctx) {
        p4iodrv_cmd_portout(p4io_ctx, (uint8_t *) &light_buff);
    }
}

void ddr_io_set_lights_hdxs_panel(uint32_t lights)
{
    // TODO: Map the player button lights to the p4io menu lights.

    if (p4io_ctx) {
        p4iodrv_cmd_coinstock(p4io_ctx, (uint8_t *) &coin_buff);
        p4iodrv_cmd_portout(p4io_ctx, (uint8_t *) &light_buff);
    }
}

void ddr_io_set_lights_hdxs_rgb(uint8_t idx, uint8_t r, uint8_t g, uint8_t b)
{
    // TODO
}

void ddr_io_fini(void)
{
    if (mdxf_device) {
        aciodrv_device_close(mdxf_device);
    }

    if (p4io_ctx) {
        p4iodrv_close(p4io_ctx);
    }
}
