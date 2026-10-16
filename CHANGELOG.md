<!--
SPDX-FileCopyrightText: ElaadNL

SPDX-License-Identifier: Apache-2.0
-->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

---

## [1.0.0] – 2026-10-19

Initial public release, developed under the ElaadNL
[Residential Flexibility](https://elaad.nl/en/projects/residential-flexibility/)
initiative.

### Added

- Matter Energy EVSE simulator firmware for ESP32-family targets
  (`examples/energy_evse/`), exposing the Energy EVSE cluster and related
  energy-management clusters for HEMS interoperability testing
- esp-matter SDK vendored as a hard fork; provenance and attribution in `NOTICE`
- Host-side unit tests for the EVSE driver logic
