// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "esp_sntp.h"

#include <array>
#include <cstring>
#include <string>
#include <sys/time.h>

namespace {
bool g_enabled = false;
sntp_sync_time_cb_t g_cb = nullptr;
std::array<std::string, 3> g_servers{};
sntp_sync_status_t g_sync_status = SNTP_SYNC_STATUS_RESET;
} // namespace

bool esp_sntp_enabled(void)
{
    return g_enabled;
}

void esp_sntp_setoperatingmode(int mode)
{
    (void)mode;
}

void esp_sntp_setservername(int idx, const char *server)
{
    if (idx < 0 || idx >= static_cast<int>(g_servers.size()) || server == nullptr) {
        return;
    }
    g_servers[static_cast<size_t>(idx)] = server;
}

void sntp_set_time_sync_notification_cb(sntp_sync_time_cb_t cb)
{
    g_cb = cb;
}

void esp_sntp_init(void)
{
    g_enabled = true;
}

void sntp_set_sync_status(sntp_sync_status_t status)
{
    g_sync_status = status;
}

sntp_sync_status_t unit_test_sntp_sync_status(void)
{
    return g_sync_status;
}

void unit_test_time_sync_fire_callback(int64_t sec, int32_t usec)
{
    if (g_cb == nullptr) {
        return;
    }
    timeval tv{};
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    g_cb(&tv);
}

const char *unit_test_time_sync_last_server(void)
{
    return g_servers[0].c_str();
}

const char *unit_test_time_sync_server_at(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(g_servers.size())) {
        return "";
    }
    return g_servers[static_cast<size_t>(idx)].c_str();
}

void unit_test_sntp_reset(void)
{
    g_enabled = false;
    g_cb = nullptr;
    g_servers = {};
    g_sync_status = SNTP_SYNC_STATUS_RESET;
}
