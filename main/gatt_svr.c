
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "cyclocomp.h"


uint16_t char_val = 0;

/* 19172c72-f781-4efa-a5ab-5158872be9c8 */
static const ble_uuid128_t gatt_svr_svc_cycl_comp_uuid =
    BLE_UUID128_INIT(0xc8, 0xe9, 0x2b, 0x87, 0x58, 0x51, 0xab, 0xa5,
                     0xfa, 0x4e, 0x81, 0xf7, 0x72, 0x2c, 0x17, 0x19);

/* 9bbff0b4-d472-48b3-8ca3-70de95ee4bd7 */
static const ble_uuid128_t gatt_svr_chr_sensor_value_uuid =
    BLE_UUID128_INIT(0xd7, 0x4b, 0xee, 0x95, 0xde, 0x70, 0xa3, 0x8c,
                     0xb3, 0x48, 0x72, 0xd4, 0xb4, 0xf0, 0xbf, 0x9b);

/* 9bbff0b4-d472-48b3-8ca3-70de95ee4bd8 */
static const ble_uuid128_t gatt_svr_chr_write_value_uuid =
    BLE_UUID128_INIT(0xd8, 0x4b, 0xee, 0x95, 0xde, 0x70, 0xa3, 0x8c,
                     0xb3, 0x48, 0x72, 0xd4, 0xb4, 0xf0, 0xbf, 0x9b);

static int
gatt_svr_chr_access_sensor_rd(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt,
                             void *arg);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /*** Service: Cyclocomputer */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_cycl_comp_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                /*** Characteristic: Recieve bike specs */
                .uuid = &gatt_svr_chr_write_value_uuid.u,
                .access_cb = gatt_svr_chr_access_sensor_rd,
                .flags = BLE_GATT_CHR_F_WRITE
            },
            {
                /*** Characteristic: Sensor value. */
                .uuid = &gatt_svr_chr_sensor_value_uuid.u,
                .access_cb = gatt_svr_chr_access_sensor_rd,
                .flags = BLE_GATT_CHR_F_NOTIFY
            },
            {
                0, /* No more characteristics in this service. */
            }},
    },

    {
        0, /* No more services. */
    },
};

static int
gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                   void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);

    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

static int
gatt_svr_chr_access_sensor_rd(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt,
                             void *arg)
{
    const ble_uuid_t *uuid;
    float num;
    int rc;

    uuid = ctxt->chr->uuid;

    /* Determine which characteristic is being accessed by examining its
     * 128-bit UUID.
     */

    if (ble_uuid_cmp(uuid, &gatt_svr_chr_sensor_value_uuid.u) == 0) {
        assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);

        num = speedValue;
        
        rc = os_mbuf_append(ctxt->om, &num, sizeof num);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (ble_uuid_cmp(uuid, &gatt_svr_chr_write_value_uuid.u) == 0) {
        rc = gatt_svr_chr_write(ctxt->om,
                            sizeof wheel_circumference,
                            sizeof wheel_circumference,
                            &wheel_circumference, NULL);
    return rc;
    }

    /* Unknown characteristic; the nimble stack should not have called this
     * function.
     */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(DEBUG, "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
                    char_val = ctxt->chr.val_handle;
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

void notify_change(){
    ble_gatts_chr_updated(char_val);
}

int
gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
