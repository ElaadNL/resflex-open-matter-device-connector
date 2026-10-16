<!--
SPDX-FileCopyrightText: ElaadNL

SPDX-License-Identifier: Apache-2.0
-->

# Vulnerabilities

Known security disclosures for this project.

## Dependencies

### Not fixed

| Severity | Advisory                                                       | Component           | Notes                                                                                                                                                                                                                                                           |
| -------- | -------------------------------------------------------------- | ------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Medium   | [CVE-2025-66442](https://osv.dev/vulnerability/CVE-2025-66442) | mbed_tls 3.6.6      | Compiler-induced timing side channel (RSA / CBC/ECB) with LLVM `select-optimize`. Brought in via ESP-IDF; fix requires a newer Mbed TLS / IDF release.                                                                                                          |
| Medium   | [CVE-2023-52160](https://osv.dev/vulnerability/CVE-2023-52160) | wpa_supplicant 2.10 | PEAP Phase-2 authentication bypass when the client does not verify the network TLS certificate. Shipped via ESP-IDF; Espressif has published IDF PEAP Phase-2 auth updates. Residual risk depends on enterprise PEAP use and certificate verification settings. |

### False positives

| Severity | Advisory                                                                                                                                                                    | Component       | Notes                                                                                                                                                                                                                                                           |
| -------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Critical | [CVE-2024-30949](https://osv.dev/vulnerability/CVE-2024-30949)                                                                                                              | newlib 4.3.0    | Espressif documents this as **not applicable** to ESP-IDF (RISC-V libgloss `gettimeofday`; ESP-IDF does not link that path). See [ESP-IDF security vulnerabilities](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/vulnerabilities.html). |
| High     | [CVE-2024-28115](https://osv.dev/vulnerability/CVE-2024-28115) / [GHSA-xcv7-v92w-gq6r](https://github.com/FreeRTOS/FreeRTOS-Kernel/security/advisories/GHSA-xcv7-v92w-gq6r) | freertos 10.5.1 | Affects FreeRTOS Kernel ARMv7-M MPU ports and ARMv8-M ports with MPU enabled. This firmware targets **esp32s3 (Xtensa)**, so the advisory scope does not apply.                                                                                                 |
