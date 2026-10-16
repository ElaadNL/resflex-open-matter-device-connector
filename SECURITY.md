<!--
SPDX-FileCopyrightText: ElaadNL

SPDX-License-Identifier: Apache-2.0
-->

# Security Policy

[![Security Policy](https://img.shields.io/badge/security-disclosure-blue)](../../security/advisories)

This file provides guidance on how to responsibly report security vulnerabilities or concerns for this project.

---

## Reporting Security Issues

If you believe you have found a security vulnerability in this repository or any of the software developed under this project:

- **Do not open a public GitHub issue**
- Instead, report it privately, either through GitHub's private vulnerability
  reporting on this repository (**Security → Report a vulnerability**) or by
  email to the address below

### Contact

`foss@elaad.nl`

Your report will be acknowledged within 5 working days. We may reach out to you for clarification or to coordinate a fix.

---

## Disclosure Policy

We follow the principles of **Coordinated Vulnerability Disclosure (CVD)**. This means:

- Security issues will be validated and fixed responsibly
- Public disclosure will be made **only after** a fix is available or a timeline has been agreed
- Credit may be given to reporters upon request

---

## Scope

This policy applies to:

- The EVSE simulator firmware in this repository (`examples/energy_evse/`)
- ElaadNL-authored tooling and documentation here
- Other open-source connectors and tools developed under the Residential Flexibility initiative

Note that this repository *is* a simulator, intended for interoperability testing
rather than deployment on production charging hardware. Issues in the simulated
device behaviour are in scope; the simulator being unsuitable for production use
is by design, not a vulnerability.

---

## Out of Scope

- Security issues in the vendored upstream
  [esp-matter](https://github.com/espressif/esp-matter) tree or the
  `connectedhomeip` submodule — please report those to their upstream projects,
  as this repository is a hard fork and does not track upstream changes
- Security issues with other third-party dependencies (report upstream if possible)
- Social engineering, physical attacks, or theoretical issues without proof of impact

---

## Thank You

We appreciate responsible security research and thank all contributors who help improve the safety and reliability of the project.
