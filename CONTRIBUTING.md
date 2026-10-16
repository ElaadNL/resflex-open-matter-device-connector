<!--
SPDX-FileCopyrightText: ElaadNL

SPDX-License-Identifier: Apache-2.0
-->

# Contributing Guide

Thank you for your interest in contributing to this project.

This repository contains the Matter EVSE simulator firmware for the ElaadNL
Residential Flexibility initiative: the device (FEID) side of Matter
interoperability testing.

---

## Scope of Contributions

You can contribute to this repository by:

- Improving the EVSE simulator firmware under `examples/energy_evse/`
- Adding or extending host unit tests under `examples/energy_evse/test/`
- Improving documentation, terminology, or style compliance
- Reporting inconsistencies, bugs, or unclear language

### Working with the vendored upstream tree

Most of this repository is [esp-matter](https://github.com/espressif/esp-matter),
vendored as a hard fork and kept **byte-identical to upstream** so it can still be
diffed against it. Two rules follow from that:

- **Do not modify upstream files** outside `examples/energy_evse/` unless the change
  is genuinely necessary. Prefer adding code under `examples/energy_evse/`.
- **Do not add SPDX headers to upstream files.** Licensing for the vendored tree is
  declared in [`REUSE.toml`](./REUSE.toml); extend the annotations there instead.
  ElaadNL-authored files carry in-file SPDX headers.

Tooling is scoped to match: `clang-format` and `markdownlint` only run over
ElaadNL-authored paths, so a full `pre-commit run --all-files` will not rewrite
vendored code.

### Before you build

The firmware needs ESP-IDF `v5.4.1`, the `connectedhomeip` submodule, and the
fork patches applied. See the [README](./README.md) — the
`./patches/apply_connectedhomeip_patches.sh` step is easy to miss and does not
fail loudly if skipped.

---

## How to Contribute

1. Fork the repository
2. Create a branch for your changes:

   ```bash
   git checkout -b docs/my-suggestion
   ```

3. Make your changes and commit with a clear message
4. Open a Pull Request against the `main` branch
5. A maintainer will review and respond to your contribution

---

## Review and Merge Process

All changes are reviewed by ElaadNL maintainers.

Pull requests are reviewed based on:

- Correctness and clarity of the change
- Consistency with the documentation style guide
- Alignment with project-wide terminology and structure
- REUSE compliance (enforced in CI) and respect for the vendored upstream tree

---

## Branch Naming Convention

- `feat/<feature>` – new firmware functionality
- `fix/<description>` – bug fixes and small corrections
- `chore/<task>` – tooling, CI, licensing, and maintenance
- `docs/<topic>` – documentation changes

---

## Code of Conduct

Please keep all contributions respectful and constructive.  
We encourage discussion, collaboration, and shared ownership of quality documentation.

---

## Contributor License Agreement

No CLA is required for contributions to this repository at this time.  
If that changes, it will be clearly documented here and in the pull request flow.

---

## Questions?

Open an issue if you're not sure whether something belongs here or in a protocol-specific repo.  
You can also contact the ElaadNL coordination team directly if needed.
