// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace CT {
namespace Charger {

namespace UIUX {

void execCurrentStatus();

void handleStatus_Available();
void handleStatus_Preparing();
void handleStatus_Charging();
void handleStatus_Suspended_EVSE();
void handleStatus_Suspended_EV();
void handleStatus_Finishing();
void handleStatus_Unavailable();
void handleStatus_Faulted();

} // namespace UIUX

} // namespace Charger
} // namespace CT