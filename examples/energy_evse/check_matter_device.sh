#!/bin/bash
# SPDX-FileCopyrightText: ElaadNL
# SPDX-License-Identifier: Apache-2.0

# Global Configuration
# --- Inputs ---
# $1 is the first argument (CHIP_TOOL path)
# $2 is the second argument (NODE_ID)
CHIP_TOOL="$1"
NODE_ID="$2"

# NODE_ID="0x7283"
# CHIP_TOOL="./out/chip-tool/chip-tool"

# --- Validation ---

# Check if CHIP_TOOL argument is empty
if [[ -z "$CHIP_TOOL" ]]; then
    echo -e "\033[31mError: Path to chip-tool (1st argument) is missing.\033[0m"
    echo "Usage: $0 <path_to_tool> <node_id>"
    exit 1
fi

# Check if NODE_ID argument is empty
if [[ -z "$NODE_ID" ]]; then
    echo -e "\033[31mError: NODE_ID (2nd argument) is missing.\033[0m"
    echo "Usage: $0 <path_to_tool> <node_id>"
    exit 1
fi

# Check if the tool actually exists and is executable
if [[ ! -x "$CHIP_TOOL" ]]; then
    echo -e "\033[31mError: chip-tool not found or not executable at: $CHIP_TOOL\033[0m"
    exit 1
fi

# --- Pre-flight Check (Fixed for Color) ---
if [[ ! -x "$CHIP_TOOL" ]]; then
    # Added -e here to enable color interpretation
    echo -e "\033[31mError: chip-tool not found at $CHIP_TOOL\033[0m"
    echo "Please ensure you are running this script from the CHIP SDK root directory."
    exit 1
fi

# --- The Core Testing Function ---
run_chip_test() {
	local CLUSTER=$1
	local OPERATION=$2
	local EP=$3
	shift 3
	local ATTRIBUTES=("$@")

	echo "---------------------------------------------------------------"
	echo " Testing: $CLUSTER | Operation: $OPERATION | EP: $EP"
	echo "---------------------------------------------------------------"
	printf "%-30s | %-10s\n" "Attribute" "Result"
	echo "---------------------------------------------------------------"

	for ATTR in "${ATTRIBUTES[@]}"; do
		# Execute the command
		RESULT=$($CHIP_TOOL "$CLUSTER" "$OPERATION" "$ATTR" "$NODE_ID" "$EP" 2>&1)

		if echo "$RESULT" | grep -q "UNSUPPORTED_ATTRIBUTE"; then
			printf "\e[31m%-30s | Unsupported\e[0m\n" "$ATTR"
		elif echo "$RESULT" | grep -q "TOO] "; then
			# Clean up the value for display (takes the last line)
			VALUE=$(echo "$RESULT" | grep "TOO]" | tail -n 1 | sed 's/.*: //')

			# Truncation: If the value is longer than 80 characters, trim it
			if [ ${#VALUE} -gt 80 ]; then
				VALUE="${VALUE:0:80}..."
			fi

			printf "\e[32m%-30s | OK (Value: %s)\e[0m\n" "$ATTR" "$VALUE"
		else
			printf "\e[33m%-30s | Failed/Error\e[0m\n" "$ATTR"
		fi
	done
	echo -e "---------------------------------------------------------------\n"
}

chip_read() {
    local ATTR=$1
    local NODE=$2
    local EP=$3
    local VERBOSE=$4

	echo "---------------------------------------------------------------"
	echo " Testing: $ATTR | Endpoint: $EP"
	echo "---------------------------------------------------------------"

    # Fix: macOS-compatible way to convert hyphen-case to PascalCase
    # Example: device-type-list -> DeviceTypeList
    local FILTER=$(echo "$ATTR" | perl -pe 's/(^|-)(\w)/\U$2/g')

    if [[ "$VERBOSE" == "-v" ]]; then
        $CHIP_TOOL descriptor read "$ATTR" "$NODE" "$EP"
    else
        # Use stdbuf or just standard pipes
        $CHIP_TOOL descriptor read "$ATTR" "$NODE" "$EP" | \
        grep -E "\[TOO\]|Msg TX.*StandaloneAck" | \
        sed 's/.*\] //' | \
        sed -n "/$FILTER/,/StandaloneAck/{ /StandaloneAck/d; p; }"
    fi
}

# ===============================================================
# TEST : Read descriptors for different endpoints
# ===============================================================
chip_read "server-list" "$NODE_ID" "0"
chip_read "server-list" "$NODE_ID" "1"
chip_read "server-list" "$NODE_ID" "2"
chip_read "server-list" "$NODE_ID" "3"
chip_read "server-list" "$NODE_ID" "4"

# ===============================================================
# TEST : BasicInformation
# ===============================================================
EVSE_ATTRS=(
	"data-model-revision" "vendor-name" "vendor-id" "product-name" "product-id" "node-label" "location" "hardware-version" "hardware-version-string" "software-version" "software-version-string" "manufacturing-date" "part-number" "product-url" "product-label" "serial-number" "local-config-disabled" "reachable" "unique-id" "capability-minima" "product-appearance" "specification-version" "max-paths-per-invoke" "configuration-version" 
)
run_chip_test "basicinformation" "read" "0" "${EVSE_ATTRS[@]}"

# ===============================================================
# TEST : EnergyEVSE
# ===============================================================
EVSE_ATTRS=(
	"state" "supply-state" "fault-state" "charging-enabled-until"
	"discharging-enabled-until" "circuit-capacity" "minimum-charge-current"
	"maximum-charge-current" "maximum-discharge-current" "user-maximum-charge-current"
	"randomization-delay-window" "next-charge-start-time" "next-charge-target-time"
	"next-charge-required-energy" "next-charge-target-so-c" "approximate-evefficiency"
	"state-of-charge" "battery-capacity" "vehicle-id" "session-id"
	"session-duration" "session-energy-charged" "session-energy-discharged"
)
run_chip_test "energyevse" "read" "1" "${EVSE_ATTRS[@]}"

# ===============================================================
# TEST : EnergyEVSE Mode
# ===============================================================
MODE_ATTRS=("supported-modes" "current-mode")
run_chip_test "energyevsemode" "read" "1" "${MODE_ATTRS[@]}"

# ===============================================================
# TEST : PowerSource
# ===============================================================
MODE_ATTRS=("status" "order" "description" "wired-assessed-input-voltage" "wired-assessed-input-frequency" "wired-current-type" "wired-assessed-current" "wired-nominal-voltage" "wired-maximum-current" "wired-present" "active-wired-faults" "bat-voltage" "bat-percent-remaining" "bat-time-remaining" "bat-charge-level" "bat-replacement-needed" "bat-replaceability" "bat-present" "active-bat-faults" "bat-replacement-description" "bat-common-designation" "bat-ansidesignation" "bat-iecdesignation" "bat-approved-chemistry" "bat-capacity" "bat-quantity" "bat-charge-state" "bat-time-to-full-charge" "bat-functional-while-charging" "bat-charging-current" "active-bat-charge-faults" "endpoint-list")
run_chip_test "powersource" "read" "2" "${MODE_ATTRS[@]}"

# ===============================================================
# TEST : PowerTopology
# ===============================================================
MODE_ATTRS=("available-endpoints" "active-endpoints" "electrical-circuit-nodes")
run_chip_test "powertopology" "read" "3" "${MODE_ATTRS[@]}"

# ===============================================================
# TEST : ElectricalPowerMeasurement
# ===============================================================
MODE_ATTRS=("power-mode" "number-of-measurement-types" "accuracy" "ranges" "voltage" "active-current" "reactive-current" "apparent-current" "active-power" "reactive-power" "apparent-power" "rmsvoltage" "rmscurrent" "rmspower" "frequency" "harmonic-currents" "harmonic-phases" "power-factor" "neutral-current")
run_chip_test "electricalpowermeasurement" "read" "3" "${MODE_ATTRS[@]}"

# ===============================================================
# TEST : ElectricalEnergyMeasurement
# ===============================================================
MODE_ATTRS=("accuracy" "cumulative-energy-imported" "cumulative-energy-exported" "periodic-energy-imported" "periodic-energy-exported" "cumulative-energy-reset")
run_chip_test "electricalenergymeasurement" "read" "3" "${MODE_ATTRS[@]}"

# ===============================================================
# TEST : DeviceEnergyManagement
# ===============================================================
MODE_ATTRS=("esatype" "esacan-generate" "esastate" "abs-min-power" "abs-max-power" "power-adjustment-capability" "forecast" "opt-out-state" )
run_chip_test "deviceenergymanagement" "read" "4" "${MODE_ATTRS[@]}"

# ===============================================================
# TEST : DeviceEnergyManagementMode
# ===============================================================
MODE_ATTRS=("supported-modes" "current-mode")
run_chip_test "deviceenergymanagementmode" "read" "4" "${MODE_ATTRS[@]}"

echo "All tests complete."
