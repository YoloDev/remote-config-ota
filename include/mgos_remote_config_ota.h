#ifndef MGOS_REMOTE_CONFIG_OTA
#define MGOS_REMOTE_CONFIG_OTA

#include "mgos_remote_config.h"
#include "mgos_updater_common.h"

#define MGOS_REMOTE_CONFIG_OTA_BASE MGOS_EVENT_BASE('R', 'O', 'T')

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool mgos_remote_config_ota_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_REMOTE_CONFIG_OTA */
