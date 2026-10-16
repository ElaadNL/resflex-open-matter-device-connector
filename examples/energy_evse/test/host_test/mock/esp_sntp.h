// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

#define SNTP_OPMODE_POLL 0

struct timeval;

typedef void (*sntp_sync_time_cb_t)(struct timeval *tv);

bool esp_sntp_enabled(void);
void esp_sntp_setoperatingmode(int mode);
void esp_sntp_setservername(int idx, const char *server);
void sntp_set_time_sync_notification_cb(sntp_sync_time_cb_t cb);
void esp_sntp_init(void);

enum sntp_sync_status_t {
    SNTP_SYNC_STATUS_RESET = 0,
    SNTP_SYNC_STATUS_COMPLETED,
};

void sntp_set_sync_status(sntp_sync_status_t status);

sntp_sync_status_t unit_test_sntp_sync_status(void);

void unit_test_time_sync_fire_callback(int64_t sec, int32_t usec);
const char *unit_test_time_sync_last_server(void);
const char *unit_test_time_sync_server_at(int idx);
void unit_test_sntp_reset(void);
