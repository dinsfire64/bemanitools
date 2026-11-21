#include <windows.h>

#include <stdbool.h>

#include "hook/iohook.h"

#include "hooklib/app.h"

#include "imports/avs.h"

#include "unicorntail/p3io.h"
#include "unicorntail/usbmem.h"

#include "cconfig/cconfig-main.h"
#include "unicorntail/config.h"

#include "util/defs.h"
#include "util/log.h"

#include "security/rp-sign-key.h"

static struct unicorntail_config _unicorntail_config;

static bool my_dll_entry_init(char *sidcode, struct property_node *param);
static bool my_dll_entry_main(void);

static bool my_dll_entry_init(char *sidcode, struct property_node *param)
{
    log_info("--- Begin unicorntail dll_entry_init ---");

    iohook_push_handler(p3io_filter_dispatch_irp);
    iohook_push_handler(usbmem_dispatch_irp);

    p3io_filter_init("COM4");
    usbmem_init();

    log_info("--- End unicorntail dll_entry_init ---");

    return app_hook_invoke_init(sidcode, param);
}

static bool my_dll_entry_main(void)
{
    bool result;

    result = app_hook_invoke_main();

    usbmem_fini();
    p3io_filter_fini();

    return result;
}

static void
_unicorntail_config_init(struct unicorntail_config *config_unicorntail_security)
{
    struct cconfig *config;

    config = cconfig_init();

    unicorntail_config_init(config);

    if (!cconfig_main_config_init(
            config,
            "--unicorntail-config",
            "unicorntail.conf",
            "--help",
            "-h",
            "unicorntail",
            CCONFIG_CMD_USAGE_OUT_STDOUT)) {
        cconfig_finit(config);
        exit(EXIT_FAILURE);
    }

    unicorntail_config_get(config_unicorntail_security, config);

    cconfig_finit(config);
}

BOOL WINAPI DllMain(HMODULE self, DWORD reason, void *ctx)
{
    if (reason != DLL_PROCESS_ATTACH) {
        goto end;
    }

    // log_to_external(log_body_misc, log_body_info, log_body_warning,
    // log_body_fatal);
    
    // app_hook_init(my_dll_entry_init, my_dll_entry_main);

    log_to_writer(log_writer_debug, NULL);

    log_info("--- Begin unicorntail dll_entry_init ---");

    _unicorntail_config_init(&_unicorntail_config);

    iohook_push_handler(p3io_filter_dispatch_irp);
    iohook_push_handler(usbmem_dispatch_irp);

    p3io_filter_init_with_plugs(
        _unicorntail_config.port,
        &_unicorntail_config.mcode,
        &_unicorntail_config.pcbid,
        &_unicorntail_config.eamid,
#if AVS_VERSION >= 1002
        &security_rp_sign_key_black_ddrx2,
#else
        &security_rp_sign_key_black_ddrx,
#endif
        &security_rp_sign_key_white_eamuse);

    usbmem_init();

    log_info("--- End unicorntail dll_entry_init ---");

end:
    return TRUE;
}
