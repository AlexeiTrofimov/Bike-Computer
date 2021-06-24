#ifndef H_CYCLOCOMP_
#define H_CYCLOCOMP_

#include <stdbool.h>
#include "nimble/ble.h"
#include "modlog/modlog.h"
#ifdef __cplusplus
extern "C" {
#endif

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

float speedValue;

/** GATT server. */
#define GATT_SVR_SVC_ALERT_UUID               0x1811
#define GATT_SVR_CHR_SUP_NEW_ALERT_CAT_UUID   0x2A47
#define GATT_SVR_CHR_NEW_ALERT                0x2A46
#define GATT_SVR_CHR_SUP_UNR_ALERT_CAT_UUID   0x2A48
#define GATT_SVR_CHR_UNR_ALERT_STAT_UUID      0x2A45
#define GATT_SVR_CHR_ALERT_NOT_CTRL_PT        0x2A44

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_svr_init(void);
void notifyChange();

/** Misc. */
void print_bytes(const uint8_t *bytes, int len);
void print_addr(const void *addr);

/** Sensor reading tools */
void read_data(void);

#ifdef __cplusplus
}
#endif

#endif
