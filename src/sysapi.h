#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sa_cpu {
    char name[64];
    uint32_t cores;
    uint32_t usage;
    uint64_t freq_hz;
    uint64_t uptime_ms;
};

struct sa_mem {
    uint64_t total;
    uint64_t used;
    uint64_t available;
};

struct sa_wifi_net {
    char ssid[33];
    int rssi;
    int channel;
    int security;
};

struct sa_wifi_status {
    bool device;
    bool connected;
    char ssid[33];
    int rssi;
    int channel;
    int security;
    uint32_t ip;
    char iface[16];
    int last_error;
};

struct sa_iface {
    char name[16];
    bool link;
    bool has_ip;
    uint32_t ip;
    uint32_t gateway;
    uint32_t dns;
    uint8_t mac[6];
    uint64_t rx_bytes;
    uint64_t tx_bytes;
};

struct sa_audio {
    uint32_t volume;
    bool muted;
    uint32_t outputs;
    uint32_t selected;
    uint32_t backend;
    bool pcm_ready;
    uint32_t codec;
    uint32_t dac;
    uint32_t pin;
};

struct sa_disk {
    char name[16];
    char model[48];
    uint64_t sectors;
    uint32_t sector_size;
    int transport;
    bool writable;
    bool operational;
};

struct sa_fs {
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint32_t block_size;
    uint32_t groups;
    uint32_t inodes_total;
    uint32_t inodes_free;
    char root[32];
    char type[16];
};

struct sa_time {
    int64_t unix_seconds;
    int32_t tz_minutes;
    uint64_t uptime_ms;
    uint32_t source;
};

struct sa_battery {
    bool present;
    uint32_t percent;
    bool charging;
    uint32_t minutes;
    char status[32];
};

struct sa_thermal {
    bool available;
    int32_t deci_c;
    uint32_t fan_rpm;
    char zone[32];
};

enum { SA_SEC_OPEN = 0, SA_SEC_WEP, SA_SEC_WPA2, SA_SEC_WPA3, SA_SEC_WPA2_WPA3 };
enum { SA_TRANSPORT_ATA = 1, SA_TRANSPORT_AHCI, SA_TRANSPORT_USB, SA_TRANSPORT_USB_EHCI };

bool sa_cpu_info(struct sa_cpu *out);
bool sa_memory_info(struct sa_mem *out);
int sa_wifi_scan(void);
int sa_wifi_list(struct sa_wifi_net *out, int capacity);
int sa_wifi_connect(const char *ssid, const char *password);
int sa_wifi_disconnect(void);
bool sa_wifi_status(struct sa_wifi_status *out);
int sa_ifaces(struct sa_iface *out, int capacity);
bool sa_audio(struct sa_audio *out);
void sa_audio_set_volume(uint32_t volume);
void sa_audio_set_muted(bool muted);
bool sa_audio_select(uint32_t index);
void sa_audio_test(void);
int sa_audio_tone(uint32_t hz, uint32_t ms);
int sa_disks(struct sa_disk *out, int capacity);
bool sa_fs_info(struct sa_fs *out);
bool sa_time(struct sa_time *out);
int sa_zone_get(char *out, uint32_t capacity);
int sa_zone_set(const char *name);
bool sa_battery(struct sa_battery *out);
bool sa_thermal(struct sa_thermal *out);
bool sa_user(char *name, uint32_t capacity, uint32_t *uid);
void sa_power(bool restart);

#ifdef __cplusplus
}
#endif
