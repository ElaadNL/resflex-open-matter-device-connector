<!--
SPDX-FileCopyrightText: ElaadNL

SPDX-License-Identifier: Apache-2.0
-->

# Matter EVSE Device Simulator — Developer Guide

Welcome! This guide walks you through our **Matter EVSE device simulator** — an ESP32-S3 reference implementation built with ESP-Matter. It is designed to help you commission, test, and integrate with HEMS controllers quickly, whether you are new to Matter or already familiar with energy clusters.

The firmware simulates an EV supply equipment (EVSE) node and exposes the Energy EVSE cluster (`0x0099`) together with related energy-management clusters for telemetry and control. We have kept the structure approachable so you can explore, validate, and extend it at your own pace.

> **A note on this demo build**  
> Provisioning credentials (DAC, passcode, discriminator) are intended for **development and evaluation only**. They use the standard ESP-Matter test values (see §3) and live in the factory (`fctry`) NVS partition. For your own deployments, please regenerate credentials using `mfg_tool` and update VID/PID as needed. We are happy to support production hardening as requirements evolve.

---

## 1. Functional Overview

### What this firmware does

| Layer | Role |
| ----- | ---- |
| **Matter stack** | Secure commissioning (PASE/CASE), attribute reporting, cluster commands |
| **EVSE simulator** | State machine for cable plug, EV demand, charging sessions, and faults |
| **Energy clusters** | Telemetry and control paths for HEMS and controller integration |

### Matter endpoints (created at runtime)

Endpoint IDs are assigned at boot. In a typical build they map as follows:

| Endpoint | Device type / role | Key clusters | Cluster ID |
| -------- | ------------------ | ------------ | ---------- |
| 0 | Root Node | Descriptor, Basic Information, … | — |
| 1 | Energy EVSE | Energy EVSE, Energy EVSE Mode | `0x0099` |
| 2 | Power Source | Power Source (wired) | `0x002F` |
| 3 | Electrical Sensor | Power Topology, Electrical Power Measurement, Electrical Energy Measurement | `0x0090`, `0x0091` |
| 4 | Device Energy Management | Device Energy Management, DEM Mode | `0x0098` |

### Device identity (runtime)

| Field | Value / source |
| ----- | -------------- |
| Vendor name | `ElaadNL` |
| Product name | `Matter EVSE` |
| Hardware version | `EVSE-SIM-1.0` |
| Serial number | `EVSE-SIM-XXXXXX` (derived from Wi-Fi MAC) |
| Vendor ID / Product ID | `menuconfig` → **CHIP Device Layer → Device Identification Options** (defaults: `0xFFF1` / `0x8000`) |

These values are set in `main/driver/ev_charger/ESP32DeviceInstanceInfoProvider.cpp` and can be customised for your product.

### Charging simulator defaults

| Parameter | Default |
| --------- | ------- |
| HW max current | 32 A (32 000 mA) |
| Min charge current | 6 A |
| Nominal voltage | 230 V |
| Phases | 1 |

Defaults are defined in `chargerManager.cpp` → `chargerInitialConfigure()`.

### High-level data flow

```text
UART commands (app_cmd.cpp)
        ↓
HardwareControlInterface  ← emulated cable / meter / fault
        ↓
ChargerManager            ← state machine (AVAILABLE → CHARGING → …)
        ↓
MatterManager + delegates ← Matter attributes, events, telemetry
        ↓
Matter controller (chip-tool / HEMS)
```

---

## 2. Where to Start in the Code

If you are new to this project, we suggest beginning with **`main/app_main.cpp`** — it shows the startup sequence in a few lines:

```cpp
CT::Charger::HardwareControlInterface::Instance().init();
CT::Charger::ChargerManager::Controller();
CT::Charger::MatterManager::GetInstance().Init();
esp_charger_commands_register();
```

| File | What it does |
| ---- | ------------ |
| `main/app_main.cpp` | Application entry point and init order |
| `main/driver/ev_charger/ESP32DeviceInstanceInfoProvider.cpp` | Vendor/product name, serial number, hardware version |
| `main/driver/ev_charger/matterManager.cpp` | Matter node, endpoints, cluster wiring, telemetry |
| `main/driver/ev_charger/EnergyEvseDelegateImpl.cpp` | EVSE commands: `SetTargets`, `EnableCharging`, `DisableCharging`, `EnableDischarging` |
| `main/driver/ev_charger/ElectricalPowerMeasurementDelegate.cpp` | Power, voltage, and current telemetry |
| `main/driver/ev_charger/DeviceEnergyManagementDelegateImpl.cpp` | Device Energy Management cluster |
| `main/driver/ev_charger/chargerManager.cpp` | Charger state machine and session logic |
| `main/driver/ev_charger/hardwareControlInterface.cpp` | Emulated hardware: cable, meter, faults |
| `main/driver/ev_charger/app_cmd.cpp` | UART commands for simulation and debugging |

### Matter endpoint creation

From `matterManager.cpp`:

```cpp
endpoint::energy_evse::create(...);              // EVSE + mode delegate
endpoint::power_source_device::create(...);      // Power source
endpoint::electrical_sensor::create(...);        // EPM + power topology
endpoint::device_energy_management::create(...); // DEM
```

### Device info example

From `ESP32DeviceInstanceInfoProvider.cpp`:

```cpp
constexpr const char kDeviceVendorName[]            = "ElaadNL";
constexpr const char kDeviceProductName[]           = "Matter EVSE";
constexpr const char kDeviceHardwareVersionString[] = "EVSE-SIM-1.0";
```

To change VID/PID, use `idf.py menuconfig` and keep `mfg_tool` / Certification Declaration aligned.

---

## 3. Build & Flash

### Prerequisites

- ESP-IDF and ESP-Matter environment — see the [ESP-Matter developing guide](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html)
- New to ESP-Matter on ESP32? The [ESP-Matter introduction](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/introduction.html) gives a helpful overview of platforms, SDK components, and tooling
- Target board: **ESP32-S3** with 16 MB flash (`sdkconfig.defaults`)

### Build steps

```bash
cd examples/energy_evse
export ESP_MATTER_PATH=/path/to/esp-matter-ct-fork
idf.py set-target esp32s3 build
idf.py -p /dev/tty.usbserial-XXXX flash monitor
```

> For the repository's `build_matter_evse.sh` helper (which wraps set-target /
> build / flash and applies the fork patches) and the `hw` serial-console
> commands, see [`docs/firmware-guide.md`](../../docs/firmware-guide.md).

### Factory / provisioning data

Commissioning data (passcode, discriminator, DAC, SPAKE2+ verifier) is stored in the **`fctry`** NVS partition (`partitions.csv`).

For development, generate credentials with [esp-matter-mfg-tool](https://pypi.org/project/esp-matter-mfg-tool/) and flash the `fctry` binary. Your actual values appear in `out/<vid_pid>/<node-id>/<node-id>-onb_codes.csv` after running `mfg_tool`.

#### Demo commissioning credentials (this build)

This example uses the **standard ESP-Matter test credentials** unless you have regenerated the `fctry` partition. You can commission via BLE using the passcode and discriminator below, or scan the demo QR code with a Matter controller app. See [ESP-Matter setup (QR code)](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html#esp-matter-setup) for further details.

![Matter demo QR code — passcode 20202021, discriminator 3840](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/_images/matter_qrcode_20202021_3840.png)

| Field | Value |
| ----- | ----- |
| Version | `0` |
| Vendor ID | `65521` (`0xFFF1`) |
| Product ID | `32768` (`0x8000`) |
| Custom flow | `0` (STANDARD) |
| Discovery bitmask | `0x02` (BLE) |
| Long discriminator | `3840` (`0xF00`) |
| Passcode | `20202021` |

These match the boot log line `vendorID=65521 productID=32768 discriminator=3840` and the chip-tool example in §4.2. For production, replace all of the above via `mfg_tool` and update VID/PID in `menuconfig`.

---

## 4. Test Instructions

This section follows a simple path: power on → commission → simulate charging → validate clusters. Each step builds on the previous one.

### 4.1 Power on the device

1. Flash the firmware and open the serial monitor: `idf.py monitor`
2. Look for: NVS init → charger init → Matter stack start → **Commissioning window opened**

You are ready to pair once the commissioning window is open.

### 4.2 Commission with a Matter controller

#### macOS + BLE-WiFi (chip-tool)

1. Install the **Matter Developer Profile** on your Mac and iPhone/iPad — [Apple Matter guide](https://github.com/project-chip/connectedhomeip/blob/master/docs/guides/darwin.md#profile-installation)
2. Build chip-tool on macOS
3. Commission over BLE-WiFi:

   ```bash
   chip-tool pairing ble-wifi <NODE_ID> <WIFI_SSID> <WIFI_PASS> <PASSCODE> <DISCRIMINATOR>
   # Example:
   chip-tool pairing ble-wifi 0x7283 MyWiFi MyPassword 20202021 3840
   ```

4. Confirm the EVSE endpoint (usually `1`):

   ```bash
   chip-tool descriptor read device-type-list <NODE_ID> 0
   chip-tool descriptor read device-type-list <NODE_ID> 1
   ```

#### Useful chip-tool commands

```bash
# Read state
chip-tool energyevse read state <NODE_ID> <ENDPOINT>
chip-tool energyevse read supply-state <NODE_ID> <ENDPOINT>

# Enable / disable charging
chip-tool energyevse enable-charging null 6000 32000 <NODE_ID> <ENDPOINT> --timedInteractionTimeoutMs 3000
chip-tool energyevse disable <NODE_ID> <ENDPOINT> --timedInteractionTimeoutMs 3000

# Read limits and mode
chip-tool energyevse read maximum-charge-current <NODE_ID> <ENDPOINT>
chip-tool energyevsemode read current-mode <NODE_ID> <ENDPOINT>
```

For more commands, see the [chip-tool guide](https://project-chip.github.io/connectedhomeip-doc/development_controllers/chip-tool/chip_tool_guide.html).

### 4.3 Simulate charging via UART

After boot, the ESP console accepts `hw` and `sw` commands (defined in `app_cmd.cpp`). These let you mimic hardware events without physical EVSE equipment — helpful for first-time bring-up.

#### Hardware emulator (`hw`)

| Command | Effect |
| ------- | ------ |
| `hw cable 1` | Plug in the charging gun |
| `hw cable 0` | Unplug the charging gun |
| `hw ev 1` | EV starts requesting energy/current |
| `hw ev 0` | EV stops drawing |
| `hw limit <mA>` | Set current limit (e.g. `hw limit 16000`) |
| `hw fault <code>` | Inject a fault (`0` clears the fault) |

#### Software helpers (`sw`)

| Command | Effect |
| ------- | ------ |
| `sw device` | Print charger configuration |
| `sw info` | Print active session information |
| `sw current <mA>` | Set session current limit |
| `sw factoryreset` | Matter factory reset |
| `sw reboot` | Reboot the device |

### 4.4 Typical charging session (step by step)

```text
1. Power on device                    → Matter stack up, commissioning window open
2. Commission with chip-tool          → "Commissioning complete"
3. chip-tool energyevse enable-charging ...
4. hw cable 1                         → NotPluggedIn → PluggedInNoDemand
5. hw ev 1                            → PluggedInCharging
6. Telemetry updates automatically    → EPM and session energy attributes
7. hw ev 0                            → PluggedInNoDemand (suspended)
8. hw cable 0                         → Session ends, NotPluggedIn
```

### 4.5 Acceptance checklist

These attributes are the main ones to verify for HEMS integration:

| Attribute | Cluster |
| --------- | ------- |
| `State`, `SupplyState` | Energy EVSE (`0x0099`) |
| `NextChargeRequiredEnergy`, `NextChargeTargetSoC` | Energy EVSE |
| `SessionEnergyCharged`, `SessionEnergyDischarged` | Energy EVSE |
| Active power / voltage / current | Electrical Power Measurement (`0x0090`) |

### 4.6 End-to-end with HEMS

Once commissioned, any Matter controller on the same fabric can interact with the clusters above. Point your HEMS implementation at the commissioned **node ID** and **EVSE endpoint** (typically endpoint `1`).

### 4.7 Cluster validation script

We provide `check_matter_device.sh` to read all clusters exposed by this build in one pass. It is a convenient sanity check after flashing or changing firmware.

```bash
./check_matter_device.sh ./out/chip-tool/chip-tool <NODE_ID>
# Example:
./check_matter_device.sh ./out/chip-tool/chip-tool 0x7283
```

**Before you run it:** the device should be commissioned and reachable; chip-tool should be built at the path you pass as the first argument.

**What it checks:**

- `server-list` on endpoints 0–4 (all expected clusters present)
- Attribute reads on: `basicinformation`, `energyevse`, `energyevsemode`, `powersource`, `powertopology`, `electricalpowermeasurement`, `electricalenergymeasurement`, `deviceenergymanagement`, `deviceenergymanagementmode`

> Some attributes show `Unsupported` in chip-tool — that is normal for this configuration. See §4.8 for a full explanation.

### 4.8 Optional features and `Unsupported` attributes

The validation script reads **every** attribute in the Matter specification. This demo implements the clusters and attributes needed for EVSE charging simulation and HEMS hand-off. Attributes marked `Unsupported` usually mean a **feature is not enabled in this build**, not that something is broken.

All clusters are present (see `server-list` from the validation script). The difference is which optional attributes are exposed.

| Cluster | Examples not enabled in this build | Why |
| ------- | ---------------------------------- | --- |
| **Basic Information** (EP 0) | `product-url`, `product-label`, `product-appearance` | Optional fields not implemented in `ESP32DeviceInstanceInfoProvider.cpp` yet |
| **Energy EVSE** (EP 1) | `discharging-enabled-until`, `maximum-discharge-current`, `session-energy-discharged` | V2X/discharging is off (`kSupportDischargingV2x = false` in `matterManager.h`) |
| **Energy EVSE** (EP 1) | `randomization-delay-window`, `approximate-evefficiency`, `vehicle-id` | Optional attributes — can be added when needed |
| **Power Source** (EP 2) | All `bat-*` attributes | Wired-only configuration; battery features not enabled |
| **Power Source** (EP 2) | `wired-assessed-*`, `wired-present`, `active-wired-faults` | Subset of wired attributes populated for this demo |
| **Power Topology** (EP 3) | `active-endpoints`, `electrical-circuit-nodes` | Minimal topology — `available-endpoints` is set |
| **Electrical Power Measurement** (EP 3) | `reactive-*`, `apparent-*`, `rms*`, harmonics | AC active measurements only (voltage, current, power, frequency) |
| **Electrical Energy Measurement** (EP 3) | `cumulative-energy-exported`, `periodic-energy-*` | Import-only metering (no V2X export in this build) |
| **DEM Mode** (EP 4) | `supported-modes` read | Mode table not fully populated yet |

#### `OK` with `null` — idle state, not an error

Some attributes read successfully but return `null` when nothing is active:

- **Schedule attributes** (`next-charge-*`) — no `SetTargets` schedule configured
- **Session attributes** (`session-id`, `session-energy-charged`, …) — no session running during the cluster check
- **Meter values** (`voltage`, `active-power`, …) — device idle; values appear once charging starts (§5)

#### Extending the build

The codebase is structured so features can be switched on as your product requirements grow:

| Capability | Where to enable |
| ---------- | --------------- |
| V2X / discharging | `kSupportDischargingV2x` in `matterManager.h` + `EnergyEvseDelegateImpl.cpp` |
| Battery power source | `Feature::kBattery` in `PowerSourceDelegate::AddCustomFeatures()` |
| Reactive / apparent power | `ElectricalPowerMeasurementDelegate.cpp` |
| DEM supported modes | `DeviceEnergyManagementDelegateImpl.cpp` |
| Additional Basic Information fields | `ESP32DeviceInstanceInfoProvider.cpp` |

We plan to expand optional cluster coverage based on integration feedback. If you need a specific attribute for your HEMS use case, the paths above are the intended extension points.

---

## 5. Sample Logs

A complete device-side capture is included: [`matter-evse-simulated-charged-meters.log`](matter-evse-simulated-charged-meters.log).  
It covers boot through commissioning, a charging session, telemetry, and controller commands. Controller (chip-tool) output is not included — only ESP32 UART logs.

### Log phases at a glance

| Phase | What happens | What to look for |
| ----- | ------------ | ---------------- |
| **1. Boot → provisioning** | Cold boot, window opens | `Commissioning window opened`, `discriminator=3840` |
| **2. Provisioned** | BLE-WiFi pairing | `Commissioning completed successfully` → `Commissioning complete` |
| **3. Charging session** | `hw cable 1`, session starts | `[ AVAILABLE ] >>> [ CHARGING ]`, `starting session at time …` |
| **4. Telemetry** | Meter updates | `mSessionEnergyCharged N`, `Charging Session Info`, `IM:ReportData` |
| **5. Controller commands** | chip-tool EVSE + meter reads | See §5.1–§5.3 |

### 5.1 Disable charging

Command: `chip-tool energyevse disable 0x7283 1`

```text
EnergyEvseDelegate::Disable()
ChargingEnabledUntil updated to 0
MaximumChargeCurrent updated to 0
SupplyState updated to 0
ComputeChargingSchedule: Not plugged in or charging disabled
```

### 5.2 Enable charging

Command: `chip-tool energyevse enable-charging null 6000 32000 0x7283 1`

```text
EnableCharging CMD received: chargingEnabledUntil=null, minCC=6000, maxCC=32000
EnergyEvseDelegate::EnableCharging()
Charging enabled indefinitely
MaximumChargeCurrent updated to 32000
SupplyState updated to 1
```

### 5.3 Meter read & write

During charging, the device sends telemetry to subscribed controllers (`IM:ReportData` approximately every 5 seconds). Key attributes to read:

- **EP 3** — `voltage`, `active-current`, `active-power`, `cumulative-energy-imported`
- **EP 1** — `session-energy-charged`, `session-duration`

Live UART output while charging (`sw info` or automatic session logs):

```text
Charging Session Info:
    Session ID:       2
    Time Elapsed:       10.00 sec
    Energy Delivered:   0.02 Wh
    Current:            32.1 A
    Voltage:            230.0 V
```

### Capture your own logs

```bash
idf.py -p /dev/tty.usbserial-XXXX monitor | tee matter-evse-simulated-charged-meters.log
```

Run the steps in §4.4, then `check_matter_device.sh` (§4.7) to confirm cluster reads.

---

## 6. Configuration Quick Reference

| Item | Location |
| ---- | -------- |
| Target / flash / BLE | `sdkconfig.defaults`, `sdkconfig.defaults.esp32s3` |
| Factory partition | `partitions.csv` → `fctry` |
| VID / PID | `menuconfig` → CHIP Device Layer |
| Vendor / product strings | `ESP32DeviceInstanceInfoProvider.cpp` |
| Max current / phases | `chargerManager.cpp` |
| Cluster features (RFID, SoC, V2X) | `EnergyEvseDelegateImpl.cpp` |
| V2X discharge flag | `matterManager.h` → `kSupportDischargingV2x` |
| Commissioning window timeout | `matterManager.cpp` → 300 s |
| Cluster test script | `check_matter_device.sh` |
| Sample device log | `matter-evse-simulated-charged-meters.log` |

---

## 7. Troubleshooting

| Symptom | What to try |
| ------- | ----------- |
| Commissioning fails | Confirm passcode/discriminator match the `fctry` partition; on macOS, install the Matter Developer Profile |
| No telemetry | Run `hw ev 1` and enable charging via chip-tool first |
| Unsure of endpoint ID | `chip-tool descriptor read device-type-list <NODE_ID> <EP>` for each endpoint |
| Stale fabric after re-flash | `sw factoryreset` or `matter esp factoryreset`; re-flash `fctry` if needed |
| `Unsupported` in cluster check | Expected for unconfigured optional features — see §4.8 |
| Time-based targets look wrong | Ensure Wi-Fi is up; prefer Matter Time Sync (`CONFIG_ENABLE_MATTER_AUTHENTICATED_TIME_SYNC=y`) or disclosed SNTP servers (`CONFIG_ENABLE_SNTP_TIME_SYNC=y`, `CONFIG_NTP_SERVER_*`) |

---

## 8. Roadmap & Support

This simulator is an active reference implementation. Current scope covers:

- Energy EVSE cluster with charging control and session reporting
- Electrical power and energy measurement (import path)
- Device Energy Management cluster (baseline)
- UART-based hardware emulation for lab and CI testing

**Planned / on request** (depends on product and certification priorities):

- V2X and bidirectional energy attributes
- Full DEM mode table and forecast reporting
- Additional Power Source and Power Topology features
- Production credential and OTA workflows

We welcome feedback from HEMS integrators and Matter developers. If you hit a gap in cluster coverage or need guidance on extending a delegate, the code paths in §4.8 and §6 are the recommended starting points — and we are open to supporting further integration work as your programme matures.

---

## References

- [ESP-Matter introduction (ESP32)](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/introduction.html) — platforms, SDK overview, and tooling
- [ESP-Matter developing guide](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html)
- [ESP-Matter setup & demo QR code](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html#esp-matter-setup) — default commissioning credentials and onboarding
- [Matter on Apple devices](https://github.com/project-chip/connectedhomeip/blob/master/docs/guides/darwin.md)
- [chip-tool guide](https://project-chip.github.io/connectedhomeip-doc/development_controllers/chip-tool/chip_tool_guide.html)
- Matter 1.4.1 Energy EVSE cluster: `0x0099`
