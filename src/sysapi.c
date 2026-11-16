#include "sysapi.h"
#include <string.h>
#include "libc/include/purec.h"

static void copy_text(char *dst, size_t cap, const char *src, size_t src_cap)
{
    size_t n = 0;
    while (n + 1 < cap && n < src_cap && src[n]) {
        dst[n] = src[n];
        n++;
    }
    dst[n] = 0;
}

bool sa_cpu_info(struct sa_cpu *out)
{
    struct cpu_monitor_info info;
    memset(out, 0, sizeof(*out));
    if (!pc_cpu_info(&info))
        return false;
    copy_text(out->name, sizeof(out->name), info.name, sizeof(info.name));
    out->cores = info.logical_processors;
    out->usage = info.usage_percent;
    out->freq_hz = info.frequency_hz;
    out->uptime_ms = info.uptime_ms;
    return true;
}

bool sa_memory_info(struct sa_mem *out)
{
    struct memory_monitor_info info;
    memset(out, 0, sizeof(*out));
    if (!pc_memory_info(&info))
        return false;
    out->total = info.total_bytes;
    out->used = info.used_bytes;
    out->available = info.available_bytes;
    return true;
}

int sa_wifi_scan(void)
{
    return pc_wifi_scan();
}

int sa_wifi_list(struct sa_wifi_net *out, int capacity)
{
    static struct wifi_network_info list[64];
    int n = pc_wifi_list(list, 64);
    if (n < 0)
        return n;
    if (n > capacity)
        n = capacity;
    for (int i = 0; i < n; i++) {
        copy_text(out[i].ssid, sizeof(out[i].ssid), list[i].ssid, sizeof(list[i].ssid));
        out[i].rssi = list[i].rssi;
        out[i].channel = list[i].channel;
        out[i].security = list[i].security;
    }
    return n;
}

int sa_wifi_connect(const char *ssid, const char *password)
{
    return pc_wifi_connect(ssid, password ? password : "");
}

int sa_wifi_disconnect(void)
{
    return pc_wifi_disconnect();
}

bool sa_wifi_status(struct sa_wifi_status *out)
{
    struct wifi_status_info info;
    memset(out, 0, sizeof(*out));
    if (!pc_wifi_status(&info))
        return false;
    out->device = info.has_device != 0;
    out->connected = info.connected != 0;
    copy_text(out->ssid, sizeof(out->ssid), info.ssid, sizeof(info.ssid));
    out->rssi = info.rssi;
    out->channel = info.channel;
    out->security = info.security;
    out->ip = info.ip_address;
    copy_text(out->iface, sizeof(out->iface), info.interface_name, sizeof(info.interface_name));
    out->last_error = info.last_error;
    return true;
}

int sa_ifaces(struct sa_iface *out, int capacity)
{
    static struct net_if_info list[16];
    int n = pc_net_if_list(list, 16);
    if (n < 0)
        return n;
    if (n > capacity)
        n = capacity;
    for (int i = 0; i < n; i++) {
        copy_text(out[i].name, sizeof(out[i].name), list[i].name, sizeof(list[i].name));
        out[i].link = list[i].link_up != 0;
        out[i].has_ip = list[i].has_ip != 0;
        out[i].ip = list[i].ip_address;
        out[i].gateway = list[i].gateway;
        out[i].dns = list[i].dns_server;
        memcpy(out[i].mac, list[i].mac, 6);
        out[i].rx_bytes = list[i].rx_bytes;
        out[i].tx_bytes = list[i].tx_bytes;
    }
    return n;
}

bool sa_audio(struct sa_audio *out)
{
    struct audio_status st;
    memset(out, 0, sizeof(*out));
    if (!pc_audio_get_status(&st))
        return false;
    out->volume = st.volume;
    out->muted = st.muted != 0;
    out->outputs = st.output_device_count;
    out->selected = st.selected_output_device;
    out->backend = st.backend;
    out->pcm_ready = st.pcm_ready != 0;
    out->codec = st.hda_codec;
    out->dac = st.hda_dac_node;
    out->pin = st.hda_pin_node;
    return true;
}

void sa_audio_set_volume(uint32_t volume)
{
    pc_audio_set_volume(volume > 100 ? 100 : volume);
}

void sa_audio_set_muted(bool muted)
{
    pc_audio_set_muted(muted);
}

bool sa_audio_select(uint32_t index)
{
    return pc_audio_select_output(index);
}

void sa_audio_test(void)
{
    pc_audio_play_test();
}

int sa_audio_tone(uint32_t hz, uint32_t ms)
{
    return pc_audio_play_tone(hz, ms);
}

int sa_disks(struct sa_disk *out, int capacity)
{
    static struct storage_device_info list[16];
    int n = pc_list_disks(list, 16);
    if (n < 0)
        return n;
    if (n > capacity)
        n = capacity;
    for (int i = 0; i < n; i++) {
        copy_text(out[i].name, sizeof(out[i].name), list[i].name, sizeof(list[i].name));
        copy_text(out[i].model, sizeof(out[i].model), list[i].model, sizeof(list[i].model));
        out[i].sectors = list[i].sector_count;
        out[i].sector_size = list[i].sector_size;
        out[i].transport = list[i].transport;
        out[i].writable = list[i].writable != 0;
        out[i].operational = list[i].operational != 0;
    }
    return n;
}

bool sa_fs_info(struct sa_fs *out)
{
    memset(out, 0, sizeof(*out));
    pc_get_root_device(out->root, sizeof(out->root));
    pc_get_fs_type(out->type, sizeof(out->type));
    struct ext2_super_info sb;
    if (pc_ext2_super(&sb) < 0 || !sb.block_size)
        return false;
    out->block_size = sb.block_size;
    out->total_bytes = (uint64_t)sb.total_blocks * sb.block_size;
    out->free_bytes = (uint64_t)sb.free_blocks * sb.block_size;
    out->groups = sb.groups_count;
    out->inodes_total = sb.total_inodes;
    out->inodes_free = sb.free_inodes;
    return true;
}

bool sa_time(struct sa_time *out)
{
    struct time_info info;
    memset(out, 0, sizeof(*out));
    if (pc_time_get(&info) < 0)
        return false;
    out->unix_seconds = info.unix_seconds;
    out->tz_minutes = info.tz_minutes;
    out->uptime_ms = info.uptime_ms;
    out->source = info.source;
    return true;
}

int sa_zone_get(char *out, uint32_t capacity)
{
    return pc_time_zone_get(out, capacity);
}

int sa_zone_set(const char *name)
{
    return pc_time_zone_set(name);
}

bool sa_battery(struct sa_battery *out)
{
    struct battery_info info;
    memset(out, 0, sizeof(*out));
    memset(&info, 0, sizeof(info));
    if (pc_syscall(SYS_BATTERY_INFO, (uint64_t)(uintptr_t)&info, 0, 0) < 0 || !info.present)
        return false;
    out->present = true;
    out->percent = info.percent > 100 ? 100 : info.percent;
    out->charging = info.charging != 0;
    out->minutes = info.remaining_minutes;
    copy_text(out->status, sizeof(out->status), info.status_text, sizeof(info.status_text));
    return true;
}

bool sa_thermal(struct sa_thermal *out)
{
    struct thermal_info info;
    memset(out, 0, sizeof(*out));
    memset(&info, 0, sizeof(info));
    if (pc_syscall(SYS_THERMAL_INFO, (uint64_t)(uintptr_t)&info, 0, 0) < 0 || !info.available)
        return false;
    out->available = true;
    out->deci_c = info.temperature_deci_c;
    out->fan_rpm = info.fan_rpm;
    copy_text(out->zone, sizeof(out->zone), info.thermal_zone, sizeof(info.thermal_zone));
    return true;
}

bool sa_user(char *name, uint32_t capacity, uint32_t *uid)
{
    struct user_info info;
    int32_t id = pc_getuid();
    if (uid)
        *uid = (uint32_t)id;
    if (pc_user_lookup(0, (uint32_t)id, &info) < 0) {
        copy_text(name, capacity, id == 0 ? "root" : "user", 8);
        return false;
    }
    copy_text(name, capacity, info.name, sizeof(info.name));
    return true;
}

void sa_power(bool restart)
{
    int32_t pid = pc_exec_with_args("/bin/systemctl", restart ? "reboot" : "poweroff");
    if (pid >= 0) {
        int32_t status = 0;
        pc_wait(pid, &status, false);
        pc_sleep(3000);
    }
    if (restart)
        pc_reboot();
    pc_shutdown();
}
