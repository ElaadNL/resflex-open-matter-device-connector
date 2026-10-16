<!--
SPDX-FileCopyrightText: ElaadNL

SPDX-License-Identifier: Apache-2.0
-->

# Support

This file explains what kind of support is available for this repository and how to get help if needed.

---

## Scope of Support

This repository is part of the ElaadNL Residential Flexibility initiative and contains the Matter
EVSE simulator firmware: the device (FEID) side of Matter interoperability testing.

Support in this context means:

- Help building, flashing, and commissioning the EVSE simulator
- Questions about the Matter clusters the simulator implements
- Clarifications on the vendored esp-matter tree, its fork patches, and its licensing
- Suggestions on documentation, naming, and consistency

---

## Where to Ask Questions

- For questions, bugs, or suggestions: open a GitHub issue
- For HEMS-side questions: see
  [`resflex-open-matter-hems-connector`](https://github.com/ElaadNL/resflex-open-matter-hems-connector)

Before opening a build issue, please confirm you are on ESP-IDF `v5.4.1`, that
submodules are initialised, and that `./patches/apply_connectedhomeip_patches.sh`
has been run — see the [README](./README.md).

---

## What Is Not Supported

This repository does not provide:

- Support for upstream [esp-matter](https://github.com/espressif/esp-matter) itself
  — report those upstream, since this is a hard fork and does not track upstream changes
- Help with proprietary product integration
- Certification of any device as Matter compliant (see `NOTICE-esp-matter`)
- Operational support or service level guarantees

---

## Notes

Providing light, community-based support does not prevent any party from offering or selling commercial support separately.
