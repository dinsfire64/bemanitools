#include <string.h>

#include "cconfig/cconfig-util.h"

#include "unicorntail/config.h"

#include "security/mcode.h"

#include "util/log.h"
#include "util/net.h"

#define UNICORNTAIL_CONFIG_PORT_KEY "unicorntail.port"

#define UNICORNTAIL_CONFIG_SECURITY_MCODE_KEY "security.mcode"

#define UNICORNTAIL_CONFIG_EAMUSE_PCBID_KEY "eamuse.pcbid"
#define UNICORNTAIL_CONFIG_EAMUSE_EAMID_KEY "eamuse.eamid"

#define UNICORNTAIL_CONFIG_DEFAULT_PORT_VALUE "COM4"

#define UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_PCBID_VALUE security_id_default
#define UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_PCBID_VALUE_LEN \
    sizeof(security_id_default)
#define UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_EAMID_VALUE security_id_default
#define UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_EAMID_VALUE_LEN \
    sizeof(security_id_default)

static const struct security_mcode security_mcode_ddr_x = {
    .header = SECURITY_MCODE_HEADER,
    .unkn = SECURITY_MCODE_UNKN_Q,
    .game = SECURITY_MCODE_GAME_DDR_X,
    .region = SECURITY_MCODE_REGION_JAPAN,
    .cabinet = SECURITY_MCODE_CABINET_A,
    .revision = SECURITY_MCODE_REVISION_A,
};

void unicorntail_config_init(struct cconfig *config)
{
    char *tmp;

    cconfig_util_set_str(
        config,
        UNICORNTAIL_CONFIG_PORT_KEY,
        UNICORNTAIL_CONFIG_DEFAULT_PORT_VALUE,
        "unicorntail serial port. Physical COM port that all serial traffic "
        "with be redirected to.");

    tmp = security_mcode_to_str(&security_mcode_ddr_x);

    cconfig_util_set_str(
        config,
        UNICORNTAIL_CONFIG_SECURITY_MCODE_KEY,
        tmp,
        "Mcode of the game to run.");

    cconfig_util_set_data(
        config,
        UNICORNTAIL_CONFIG_EAMUSE_PCBID_KEY,
        (uint8_t *) &UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_PCBID_VALUE,
        UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_PCBID_VALUE_LEN,
        "PCBID");

    cconfig_util_set_data(
        config,
        UNICORNTAIL_CONFIG_EAMUSE_EAMID_KEY,
        (uint8_t *) &UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_EAMID_VALUE,
        UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_EAMID_VALUE_LEN,
        "EAMID");

    free(tmp);
}

void unicorntail_config_get(
    struct unicorntail_config *config_unicorntail, struct cconfig *config)
{
    char *tmp_default;
    char mcode[9];
    char *tmp;
    char *tmp2;

    if (!cconfig_util_get_str(
            config,
            UNICORNTAIL_CONFIG_PORT_KEY,
            config_unicorntail->port,
            sizeof(config_unicorntail->port) - 1,
            UNICORNTAIL_CONFIG_DEFAULT_PORT_VALUE)) {
        log_warning(
            "Invalid value for key '%s' specified, fallback "
            "to default '%s'",
            UNICORNTAIL_CONFIG_PORT_KEY,
            UNICORNTAIL_CONFIG_DEFAULT_PORT_VALUE);
    }

    tmp_default = security_mcode_to_str(&security_mcode_ddr_x);

    if (!cconfig_util_get_str(
            config,
            UNICORNTAIL_CONFIG_SECURITY_MCODE_KEY,
            mcode,
            sizeof(mcode) - 1,
            tmp_default)) {
        log_warning(
            "Invalid value for key '%s' specified, fallback "
            "to default '%s'",
            UNICORNTAIL_CONFIG_SECURITY_MCODE_KEY,
            tmp_default);
    }

    mcode[8] = '\0';

    if (!security_mcode_parse(mcode, &config_unicorntail->mcode)) {
        log_warning(
            "Invalid mcode '%s' specified, fallback to default '%s'",
            mcode,
            tmp_default);

        memcpy(
            &config_unicorntail->mcode,
            &security_mcode_ddr_x,
            sizeof(struct security_mcode));
    }

    if (!cconfig_util_get_data(
            config,
            UNICORNTAIL_CONFIG_EAMUSE_PCBID_KEY,
            (uint8_t *) &config_unicorntail->pcbid,
            UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_PCBID_VALUE_LEN,
            (uint8_t *) &UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_PCBID_VALUE)) {
        tmp = security_id_to_str(
            &UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_PCBID_VALUE, false);
        log_warning(
            "Invalid value for key '%s' specified, fallback "
            "to default",
            tmp);
        free(tmp);
    }

    if (!security_id_verify(&config_unicorntail->pcbid)) {
        tmp = security_id_to_str(
            &UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_PCBID_VALUE, false);
        tmp2 = security_id_to_str(&config_unicorntail->pcbid, false);
        log_warning(
            "PCBID verification of '%s' failed, fallback to default "
            "PCBID '%s'",
            tmp2,
            tmp);
        free(tmp);
        free(tmp2);
    }

    if (!cconfig_util_get_data(
            config,
            UNICORNTAIL_CONFIG_EAMUSE_EAMID_KEY,
            (uint8_t *) &config_unicorntail->eamid,
            UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_EAMID_VALUE_LEN,
            (uint8_t *) &UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_EAMID_VALUE)) {
        tmp = security_id_to_str(
            &UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_EAMID_VALUE, false);
        log_warning(
            "Invalid value for key '%s' specified, fallback "
            "to default",
            tmp);
        free(tmp);
    }

    if (!security_id_verify(&config_unicorntail->eamid)) {
        tmp = security_id_to_str(
            &UNICORNTAIL_CONFIG_EAMUSE_DEFAULT_EAMID_VALUE, false);
        tmp2 = security_id_to_str(&config_unicorntail->eamid, false);
        log_warning(
            "EAMID verification of '%s' failed, fallback to default "
            "EAMID '%s'",
            tmp2,
            tmp);
        free(tmp);
        free(tmp2);
    }

    free(tmp_default);
}
