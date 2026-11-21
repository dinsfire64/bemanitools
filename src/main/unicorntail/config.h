#ifndef UNICORNTAIL_CONFIG_SECURITY_H
#define UNICORNTAIL_CONFIG_SECURITY_H

#include "cconfig/cconfig.h"

#include "security/id.h"

#include "security/mcode.h"

#include "util/net.h"

/**
 * Struct holding configuration values for security related items.
 */
struct unicorntail_config {
    char port[64];
    struct security_mcode mcode;
    struct security_id pcbid;
    struct security_id eamid;
};

/**
 * Initialize a cconfig structure with the basic structure and default values
 * of this configuration.
 */
void unicorntail_config_init(struct cconfig *config);

/**
 * Read the module specific config struct values from the provided cconfig
 * struct.
 *
 * @param config_unicorntail Target module specific struct to read configuration
 *                        values to.
 * @param config cconfig struct holding the intermediate data to read from.
 */
void unicorntail_config_get(
    struct unicorntail_config *config_unicorntail, struct cconfig *config);

#endif