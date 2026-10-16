<!--
SPDX-FileCopyrightText: ElaadNL

SPDX-License-Identifier: Apache-2.0
-->

# Energy EVSE build (`build_matter_evse.sh`)

This file describes how to use the repository helper script `build_matter_evse.sh`, which wraps common **ESP-IDF** / **esp-matter** commands for the `examples/energy_evse` firmware.

> For the full simulator developer guide — endpoints, code walkthrough,
> commissioning, and end-to-end testing with a HEMS — see
> [`examples/energy_evse/README.md`](../examples/energy_evse/README.md). This
> page is the focused reference for the build script and serial console.

## Emulator-oriented firmware

This example is set up to exercise **charging logic and Matter EVSE flow** on a **software-emulated** charger: cable state, current limits, EV “drawing” power, faults, and meter values are driven through `HardwareControlInterface` and related code rather than a full production EVSE board. The build script and binary are intended for **development and bring-up** on ESP32-S3 (serial console, commissioning, and emulator commands), not as a one-size-fits-all production image. If you retarget to real hardware, you will need to wire those interfaces to your PHY/RELAY/ADC/NFC as appropriate.

## Serial console: `hw` emulator commands

With the flashed app running, open a serial session (for example `idf.py -p <PORT> monitor` from `examples/energy_evse/`). The [esp-matter console](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html#console-commands) is available; this project also registers a **`hw` command** used to manipulate the **emulated** hardware and charging inputs (implementation: `examples/energy_evse/main/driver/ev_charger/app_cmd.cpp` — `hw_cmd_handler`).

Syntax: `hw <subcommand> <argument>`

| Subcommand | Argument | Effect |
|------------|----------|--------|
| `cable` | `0` or `1` | `0` = cable not connected, `1` = cable connected (drives the emulated cable state for the charger state machine). |
| `limit` | mA (integer) | Sets the **charging current limit** for the active session path via `ChargerManager::setChargingSessionCurrentLimit` (value is read as a 64-bit string; applied as `int` mA). |
| `ev` | `0` or `1` | Toggles whether the emulated **EV is drawing** current (used by the logic that distinguishes charging vs suspended, etc.). |
| `fault` | code (0–255) | Sets the emulated **EVSE/charger fault** code (see `FaultStateEnum` in your cluster mapping); `0` is typically *no error*. |

Other top-level console commands in the same file (not part of `hw`, but useful for the same example): for example `sw` subcommands for factory reset, `device` / `info` to print charger and session text, and `nfc` helpers for test card UIDs. The built-in `help` in the monitor (if enabled) may list the registered names; the `help` string for `hw` in code can lag behind the handlers—treat the table above as the source of truth for **cable / limit / ev / fault**.

## Prerequisites

You need a working **ESP-IDF** and **esp-matter** environment (toolchain, `idf.py` on your `PATH`, and submodules). If you have not set this up yet, follow the official guide:

- [esp-matter — Get started](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/getting-started.html)

Typical setup on the machine where you build:

1. **ESP-IDF**: install a version that matches the esp-matter release you are using, then in each new shell run:
   - `source $IDF_PATH/export.sh` (or your OS-specific export command).

2. **This repo (esp-matter)**: from the **repository root** (where `build_matter_evse.sh` lives), run:
   - `git submodule update --init --recursive` (as needed for your branch).
   - `source export.sh` (esp-matter export so Matter paths and extra tools are configured).
   - Confirm `idf.py` works: `idf.py --version`.

3. **Fork patch on `connectedhomeip` (required for this example):** this fork does not carry local submodule commits. After submodules are updated, run **`./patches/apply_connectedhomeip_patches.sh`** once (or rely on `build_matter_evse.sh`, which runs the same step automatically). The first patch defers `MatterReportingAttributeChangeCallback` in `mode-base-server` to the system layer so attribute reporting is safe in the energy EVSE / emulator use case. If you use a different `connectedhomeip` commit than the one this fork pins, the patch may fail to apply; refresh or drop it as needed. After the patch is applied, `git status` may list the submodule as **modified**; that is expected—**do not** commit a new `connectedhomeip` pointer in the parent repo unless you are intentionally moving to a different upstream SHA.

4. **Hardware target**: the script is written for **ESP32-S3** (see [Configuration](#configuration) to change the flow if you use another SoC).

## What the script does

From the repo root, the script `cd`s into `examples/energy_evse/` and, depending on environment flags:

| Step | When it runs | Command (conceptually) |
|------|----------------|-------------------------|
| Set target | `SET_TARGET=1` | `idf.py set-target esp32s3` (with CMake policy as in the script) |
| Build | `BUILD=1` | `idf.py build` (see note below) |
| Flash app only | `FLASH_APP=1` | `idf.py app-flash` |

**Build output note:** with `BUILD=1`, the script runs `idf.py build` and **filters** the log to lines containing `warning`, `error`, or `Project build complete`, so you see less noise. For a full unfiltered build log, run from `examples/energy_evse/`: `idf.py build`.

**Flash note:** `app-flash` flashes the application only (not a full image erase/flash of all partitions). Use `idf.py -p <PORT> flash` or the full `erase-flash` flow from the [esp-matter documentation](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html) when you need a complete reflash or a different port.

## Build steps (typical)

Run all commands from the **repository root** (same directory as the script), after sourcing **IDF** and **esp-matter** `export` scripts.

### First-time (or new clone)

1. `source $IDF_PATH/export.sh`
2. `source ./export.sh` (from this repo root)
3. `git submodule update --init --recursive` (if you have not already)
4. `./patches/apply_connectedhomeip_patches.sh` (or skip if you will run `build_matter_evse.sh`, which applies the same patches automatically)

### Set chip target, then build

```bash
SET_TARGET=1 ./build_matter_evse.sh
BUILD=1 ./build_matter_evse.sh
```

You can combine them in one invocation:

```bash
SET_TARGET=1 BUILD=1 ./build_matter_evse.sh
```

### Flash the built application (optional)

With the board connected and a serial port available (e.g. `/dev/cu.usbserial-*` on macOS, `/dev/ttyUSB0` on Linux), set the port the usual **ESP-IDF** way, then run:

```bash
# Example: pick your port
export ESPPORT=/dev/cu.usbserial-XXXX
FLASH_APP=1 ./build_matter_evse.sh
```

(Or use `idf.py -p <PORT> app-flash` from `examples/energy_evse/`.)

### One-liner (set target + build + flash)

```bash
SET_TARGET=1 BUILD=1 FLASH_APP=1 ./build_matter_evse.sh
```

Use this only when you really want to run all three steps; you normally set the target only once per worktree (or when changing `sdkconfig`/target).

## Configuration

- **Default flags:** `SET_TARGET`, `BUILD`, and `FLASH_APP` default to `0`. Any non-`1` value leaves that step off.
- **SoC / target:** the script hard-codes `esp32s3` in the `set-target` step. For another target, run `idf.py set-target <target>` once from `examples/energy_evse/`, or edit the script to match your board.
- **Example app:** `sdkconfig` defaults and board-specific options may live in `examples/energy_evse/sdkconfig.defaults` and `examples/energy_evse/sdkconfig.defaults.esp32s3`. Adjust there or via `idf.py menuconfig` in that directory.

For general Matter development, commissioning, and flashing, see the main project **README** and the [esp-matter developing guide](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html).
