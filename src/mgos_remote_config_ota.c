#include "mgos_remote_config_ota.h"
#include "common/cs_dbg.h"
#include "frozen.h"
#include "mgos_sys_config.h"
#include "mgos_yolodev_ota.h"

// #define OTA_VERSION_FILE "ota_version.json"
#define OTA_CONFIG_KEY ".ota"

struct remote_ota_config {
  char *version;
  uint32_t crc32;
  char *uri;
};

static const void *get_ota_config(void *store) { return store; }

static int safe_strcmp(const char *left, const char *right) {
  if (left == right) {
    return 0;
  }

  if (left == NULL) {
    return -1;
  }

  if (right == NULL) {
    return 1;
  }

  return strcmp(left, right);
}

static uint8_t hex_char_to_int(char c, const char *str) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  LOG(LL_WARN, ("Invalid crc char in string: %s", str));
  return 0;
}

static uint32_t hext_to_int(const char *str) {
  // These should always have length 8
  char c0, c1, c2, c3, c4, c5, c6, c7;
  c0 = hex_char_to_int(str[0], str);
  c1 = hex_char_to_int(str[1], str);
  c2 = hex_char_to_int(str[2], str);
  c3 = hex_char_to_int(str[3], str);
  c4 = hex_char_to_int(str[4], str);
  c5 = hex_char_to_int(str[5], str);
  c6 = hex_char_to_int(str[6], str);
  c7 = hex_char_to_int(str[7], str);

  uint32_t ret = 0;
  ret += c0;
  ret = ret << 4;
  ret += c1;
  ret = ret << 4;
  ret += c2;
  ret = ret << 4;
  ret += c3;
  ret = ret << 4;
  ret += c4;
  ret = ret << 4;
  ret += c5;
  ret = ret << 4;
  ret += c6;
  ret = ret << 4;
  ret += c7;

  return ret;
}

static bool update_ota_config(void *store, const struct json_token *token,
                              const char *path) {
  struct remote_ota_config *data = (struct remote_ota_config *)store;
  switch (token->type) {
    case JSON_TYPE_OBJECT_END: {
      char *uri = NULL;
      char *crcHex = NULL;
      char *version = NULL;
      if (json_scanf(token->ptr, token->len, "{uri: %Q, crc: %Q, ver: %Q}",
                     &uri, &crcHex, &version) == 3) {
        uint32_t crc = hext_to_int(crcHex);
        LOG(LL_DEBUG, ("CRC: %s parsed as %x", crcHex, crc));
        free(crcHex);

        if (safe_strcmp(uri, data->uri) == 0 &&
            safe_strcmp(version, data->version) == 0 && crc == data->crc32) {
          LOG(LL_INFO,
              ("OTA config was the same as previous config, ignoring"));
          free(uri);
          free(version);
          return false;
        }

        LOG(LL_DEBUG, ("New ota config received, storing"));
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
  if (safe_strcmp(update->path, OTA_CONFIG_KEY) == 0) {
    struct remote_ota_config *data = (struct remote_ota_config *)update->value;
    yolodev_request_ota(data->uri, data->crc32);
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
