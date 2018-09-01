#include "mgos_remote_config_ota.h"
#include "common/cs_dbg.h"
#include "frozen.h"
#include "mgos_sys_config.h"

// #define OTA_VERSION_FILE "ota_version.json"
#define OTA_CONFIG_KEY ".ota"

struct remote_ota_config {
  char *version;
  uint32_t crc32;
  char *uri;
};

static const void *get_ota_config(void *store) { return store; }

static bool update_ota_config(void *store, const struct json_token *token,
                              const char *path) {
  struct remote_ota_config *data = (struct remote_ota_config *)store;
  switch (token->type) {
    case JSON_TYPE_OBJECT_END: {
      char *uri;
      char *crcHex;
      char *version;
      if (json_scanf(token->ptr, token->len, "{uri: %Q, crc: %Q, ver: %Q}",
                     &uri, &crcHex, &version) == 3) {
        uint32_t crc = strtol(crcHex, NULL, 16);
        free(crcHex);

        if (strcmp(uri, data->uri) == 0 &&
            strcmp(version, data->version) == 0 && crc == data->crc32) {
          free(uri);
          free(version);
          return false;
        }

        free(data->version);
        free(data->uri);
        data->version = version;
        data->uri = uri;
        data->crc32 = crc;
        return true;
      }

      LOG(LL_WARN,
          ("%s: expected ota object with uri, crc and ver, but was: %.*s", path,
           token->len, token->ptr));
      return false;
    }

    default: {
      LOG(LL_WARN,
          ("%s: expected object, but was: %.*s", path, token->len, token->ptr));
      return false;
    }
  }
}

struct mgos_remote_config_data mgos_remote_config_data_ota_config() {
  struct remote_ota_config *data = malloc(sizeof(struct remote_ota_config));
  data->version = NULL;
  data->crc32 = 0;
  data->uri = NULL;

  struct mgos_remote_config_data ret = {
      .data = data, .update = update_ota_config, .get = get_ota_config};

  return ret;
}

void mgos_remote_config_update_ev(int ev, void *ev_data, void *userdata) {
  struct mgos_remote_config_update *update =
      (struct mgos_remote_config_update *)ev_data;
  if (strcmp(update->path, OTA_CONFIG_KEY) == 0) {
    struct remote_ota_config *data = (struct remote_ota_config *)update->value;
    LOG(LL_INFO, ("Trigger new update to version: %s", data->version));
    // TODO: Trigger some update event that starts the download
  }

  (void)ev;
  (void)userdata;
}

bool mgos_remote_config_ota_init(void) {
  if (!mgos_sys_config_get_rcfg_enable()) {
    return true;
  }

  if (!mgos_sys_config_get_rcfg_ota()) {
    return true;
  }

  struct mgos_remote_config_prop remote_props[1] = {
      {.path = OTA_CONFIG_KEY, .data = mgos_remote_config_data_ota_config()}};
  mgos_remote_config_register(remote_props, ARRAY_SIZE(remote_props));

  mgos_event_add_handler(MGOS_REMOTE_CONFIG_UPDATE,
                         mgos_remote_config_update_ev, NULL);
  return true;
}