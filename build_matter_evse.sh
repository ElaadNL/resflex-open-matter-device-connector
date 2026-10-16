#!/bin/bash
# SPDX-FileCopyrightText: ElaadNL
# SPDX-License-Identifier: Apache-2.0

# Usage (either form works):
#   SET_TARGET=1 BUILD=1 ./build_matter_evse.sh
#   ./build_matter_evse.sh SET_TARGET=1 BUILD=1

SET_TARGET=${SET_TARGET:-0}
BUILD=${BUILD:-0}
FLASH_APP=${FLASH_APP:-0}

# Allow KEY=VALUE arguments (override env defaults).
for arg in "$@"; do
	case "$arg" in
	SET_TARGET=* | BUILD=* | FLASH_APP=*)
		export "$arg"
		;;
	*)
		echo "Unknown argument: $arg" >&2
		echo "Usage: $0 [SET_TARGET=0|1] [BUILD=0|1] [FLASH_APP=0|1]" >&2
		exit 1
		;;
	esac
done

SET_TARGET=${SET_TARGET:-0}
BUILD=${BUILD:-0}
FLASH_APP=${FLASH_APP:-0}

# Patches under patches/connectedhomeip/ are applied manually — not by this script.

if [[ "$SET_TARGET" -ne 1 && "$BUILD" -ne 1 && "$FLASH_APP" -ne 1 ]]; then
	echo "Nothing to do. Example: $0 SET_TARGET=1 BUILD=1" >&2
	exit 1
fi

#assume you have setup the env and matter path
pushd examples/energy_evse/ > /dev/null

if [ "$SET_TARGET" -eq 1 ]; then
	idf.py set-target esp32s3 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
fi

if [ "$BUILD" -eq 1 ]; then
	idf.py build
fi

if [ "$FLASH_APP" -eq 1 ]; then
	idf.py app-flash
fi

popd > /dev/null
