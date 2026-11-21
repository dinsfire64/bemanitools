#ifndef UNICORNTAIL_P3IO_H
#define UNICORNTAIL_P3IO_H

#include "hook/iohook.h"

#include "security/mcode.h"
#include "security/rp3.h"

void p3io_filter_init(const char *port);
void p3io_filter_init_with_plugs(
    const char *port,
    const struct security_mcode *mcode,
    const struct security_id *pcbid,
    const struct security_id *eamid,
    const struct security_rp_sign_key *black_sign_key,
    const struct security_rp_sign_key *white_sign_key);
void p3io_filter_fini(void);
HRESULT p3io_filter_dispatch_irp(struct irp *irp);

#endif
