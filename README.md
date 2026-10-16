<!--
SPDX-FileCopyrightText: ElaadNL

SPDX-License-Identifier: Apache-2.0
-->

# resflex-open-matter-device-connector

Matter EVSE-simulator firmware for the device (FEID) side of Matter
interoperability testing within the Residential Flexibility initiative of
ElaadNL and FAN, part of the Dutch National Grid Congestion Action Program.

This repository is the device-side counterpart to
[`resflex-open-matter-hems-connector`](https://github.com/ElaadNL/resflex-open-matter-hems-connector),
which implements the HEMS side. It carries the firmware that the HEMS connector
is developed and tested against.

## Disclaimer

This software is an open reference implementation, developed under the
coordination of ElaadNL as part of the
[Residential Flexibility](https://elaad.nl/en/projects/residential-flexibility/) initiative. It is not a finished,
certified, or end-user product. Before publication, the code was reviewed for
security by the European Network for Cyber Security (ENCS); that review is a
snapshot in time, not a guarantee. Anyone who integrates this software into a
product does so at their own risk and is responsible for its secure integration,
ongoing security testing, regulatory compliance, and any certification of that
product. The software is provided "as is", without warranty, under the Apache
License 2.0.

Read the full [disclaimer](DISCLAIMER.md) before using this software.

---

## What this repository contains

The [esp-matter](https://github.com/espressif/esp-matter) SDK, vendored as a
hard fork and extended with an **Energy EVSE simulator** example
(`examples/energy_evse`). It builds with ESP-IDF for ESP32-family targets.

| Path | Purpose |
| --- | --- |
| `examples/energy_evse/` | The EVSE simulator firmware — the primary deliverable |
| `components/` | esp-matter SDK components |
| `device_hal/` | Device hardware abstraction (LEDs, buttons, drivers) |
| `patches/` | Fork-only patches applied to the `connectedhomeip` submodule |
| `connectedhomeip/` | Matter SDK, pinned as a Git submodule |
| `docs/` | Upstream esp-matter docs plus ElaadNL project documentation |

Project background, use cases, and shared terminology are in
[`docs/index.md`](./docs/index.md). Provenance, attribution, and third-party
licenses are recorded in [`NOTICE`](./NOTICE).

---

## Requirements

- **ESP-IDF `v5.4.1`** — other versions are not tested against this tree
- A supported ESP32 target (the EVSE simulator is built for `esp32s3`)

---

## Getting started

### 1. Clone with submodules

```bash
git clone --recurse-submodules https://github.com/ElaadNL/resflex-open-matter-device-connector.git
cd resflex-open-matter-device-connector
```

For an existing clone:

```bash
git submodule update --init --recursive
```

### 2. Apply the required connectedhomeip fixes before setup

The Matter submodule layout has changed upstream, so the repository now needs
its fork-specific install fixes applied before the normal ESP-IDF setup steps.
These fixes are required for a clean local build on recent Matter snapshots:

- `connectedhomeip/connectedhomeip/scripts/setup/zap.json`
  using `patches/connectedhomeip/fix-matter-install.patch`
- `connectedhomeip/connectedhomeip/third_party/pigweed/repo/pw_env_setup/py/pw_env_setup/cipd_setup/arm.json`
  using `patches/connectedhomeip/fix-matter-install-2.patch`

Apply the repository's fork patch flow first, then continue with the normal
install/export steps:

```bash
./patches/apply_connectedhomeip_patches.sh
./install.sh
. ./export.sh
```

This sequence is **required and easy to miss**. If you skip the connectedhomeip
fixes, the Matter bootstrap/install step can fail or the build can break after
submodule updates.

> **Expected side effect:** the patches modify the `connectedhomeip` submodule's
> working tree, so `git status` will permanently show
> `connectedhomeip/connectedhomeip` as *modified*. That is normal. **Do not**
> commit a new submodule pointer unless you intend to move to a different
> upstream SHA.

### 3. Configure, build, and flash

```bash
SET_TARGET=1 BUILD=1 ./build_matter_evse.sh      # configure + build
FLASH_APP=1 ./build_matter_evse.sh               # flash
```

If the helper script reports that the target is not defined, configure the
ESP32-S3 target manually before running the build workflow:

```bash
idf.py set-target esp32s3 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

The full simulator developer guide — endpoints, code walkthrough,
commissioning, and end-to-end HEMS testing — is
[`examples/energy_evse/README.md`](./examples/energy_evse/README.md).
[`docs/firmware-guide.md`](./docs/firmware-guide.md) is the focused reference
for the `build_matter_evse.sh` helper and the serial console.

Upstream esp-matter's own documentation is not reproduced here; see
[`espressif/esp-matter`](https://github.com/espressif/esp-matter) for it.

### Host unit tests

```bash
./generate_unit_tests_coverage.sh
```

> Note: this script uses `xcrun llvm-cov gcov` and therefore runs on macOS only.

---

## Relationship to upstream

This repository is a **hard fork**, not a mirror: there is no upstream-sync
branch and no obligation to cherry-pick upstream changes.

| | |
| --- | --- |
| Original upstream | [`espressif/esp-matter`](https://github.com/espressif/esp-matter) (`release/v1.4`) |
| Intermediate fork | [`Computime-OSS/esp-matter`](https://github.com/Computime-OSS/esp-matter) (`evse-simulator-v1.4`) |
| Vendored from commit | `98ebf366d43836e0d37ba17ce46c150dcef964cb` |
| Matter SDK submodule | `8a70495a0578de0b1338199a670ac0a3d238ae3e` |

Espressif's internal GitLab CI and GitHub automation (JIRA sync, component-registry
publishing, Docker Hub pushes) were removed during vendoring, as they target
Espressif infrastructure and cannot run here.

---

## Reference documents

Quick links to the main EVSE example assets. Paths are relative to the repository root.

| Document | Description |
| --- | --- |
| [`examples/energy_evse/`](./examples/energy_evse/) | Primary deliverable — Matter EVSE simulator firmware for ESP32-S3 (clusters, charging state machine, UART simulation commands, and host unit tests) |
| [`examples/energy_evse/README.md`](./examples/energy_evse/README.md) | Developer guide for the example — endpoints, code map, commissioning, console commands, and end-to-end HEMS testing |
| [`build_matter_evse.sh`](./build_matter_evse.sh) | Helper to apply fork patches, set the `esp32s3` target, build, and flash (`SET_TARGET=1`, `BUILD=1`, `FLASH_APP=1`) |
| [`examples/energy_evse/check_matter_device.sh`](./examples/energy_evse/check_matter_device.sh) | Automated `chip-tool` checks of EVSE-related Matter cluster attributes on a commissioned node |
| [`generate_unit_tests_coverage.sh`](./generate_unit_tests_coverage.sh) | Builds and runs host unit tests under `examples/energy_evse/test/host_test`, then emits a coverage report in SonarQube XML format (macOS / `llvm-cov`) |
| [`examples/energy_evse/matter-evse-simulated-charged-meters.log`](./examples/energy_evse/matter-evse-simulated-charged-meters.log) | Captured ESP32 UART log from a simulated charging session (boot, meters, and session telemetry for comparison) |

---

## License

All content is licensed under the [Apache 2.0 License](./LICENSE).

The vendored esp-matter tree is Apache-2.0, copyright Espressif Systems
(Shanghai) PTE LTD, Project CHIP Authors, and esp-matter contributors. It also
carries MIT-licensed components (Unity, QRCode) and some CC0-1.0 files. See
[`NOTICE`](./NOTICE) for full attribution and [`REUSE.toml`](./REUSE.toml) for
per-path declarations.

---

## License Compliance (REUSE)

This repository follows the [REUSE Specification](https://reuse.software/) to ensure consistent and machine-readable license and copyright information.

To support this:

- A REUSE configuration is included in the repository
- Contributors are expected to use the provided pre-commit hook to validate compliance before committing
- The CI pipeline enforces REUSE compliance on all pull requests and is the authoritative check

Because the vendored upstream sources carry almost no per-file SPDX headers,
copyright and licensing are declared in bulk in [`REUSE.toml`](./REUSE.toml)
rather than by injecting headers into upstream files. This keeps the vendored
tree byte-identical to upstream. **When adding coverage for new files, extend
the annotations in `REUSE.toml` instead of editing upstream sources.**

If you want to run checks locally, the following projects are used:

- REUSE tool: [https://reuse.software/](https://reuse.software/)
- pre-commit framework: [https://pre-commit.com](https://pre-commit.com)

```bash
reuse lint
```

If a pull request fails the REUSE check in CI, it must be fixed before it can be merged.

---

## Questions or suggestions?

Please open an issue or contact the ElaadNL coordination team.
