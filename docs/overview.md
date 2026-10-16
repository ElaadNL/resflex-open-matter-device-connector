<!--
SPDX-FileCopyrightText: ElaadNL

SPDX-License-Identifier: Apache-2.0
-->

# Project Overview

This document provides background and scope for the Matter device connector.

![overview.jpg](overview.jpg)

---

## Purpose of This Repository

This repository contains the **device (FEID) side** of Matter interoperability:
EVSE simulator firmware built on Espressif's SDK for Matter, vendored here as a
hard fork.

It is the counterpart to
[`resflex-open-matter-hems-connector`](https://github.com/ElaadNL/resflex-open-matter-hems-connector),
which implements the HEMS side. The firmware here is what that connector is
developed and tested against.

Key goals:

- Provide a reference Matter EVSE implementation for HEMS interoperability testing
- Demonstrate the residential flexibility use cases over Matter 1.4
- Keep the vendored upstream tree traceable and correctly attributed
- Enable faster onboarding and reuse

For the build and flash workflow, see the [repository README](../README.md).

---

## Residential Flexibility

This software was developed as part of the ElaadNL
[Residential Flexibility](https://elaad.nl/en/projects/residential-flexibility/)
initiative, part of the Dutch National Grid Congestion Action Program. The
initiative aims to improve interoperability between Home Energy Management
Systems (HEMS) and flexible energy-intensive devices (FEIDs) such as:

- EV chargers
- Home batteries
- Heat pumps
- Solar inverters

Open-source connectors are developed for the following open protocols:

- S2 (EN 50491-12-2)
- Matter 1.4
- EEBUS (SHIP/SPINE)
- Modbus RTU/TCP (via a converter)
- OCPP

---

## System Context

The HEMS acts as the central coordinator. It receives external signals (such as
grid capacity limits or dynamic prices) and manages connected devices
accordingly. These devices include EV chargers, batteries, heat pumps, and
inverters.

Each connector enables secure and interoperable communication between the HEMS
and a specific type of device, using an open protocol.

This repository sits at the **device** end of that picture: it presents an EV
charger to a Matter-speaking HEMS, so the HEMS-side connector can be exercised
end to end without physical charging hardware.

---

## Relationship to Other Repositories

| Repository | Role |
| --- | --- |
| [`resflex-open-matter-hems-connector`](https://github.com/ElaadNL/resflex-open-matter-hems-connector) | HEMS side: Matter controller server |
| This repository | Device (FEID) side: EVSE simulator firmware |

Terminology follows [glossary.md](./glossary.md).
