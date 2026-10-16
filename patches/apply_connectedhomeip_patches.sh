#!/usr/bin/env bash
# SPDX-FileCopyrightText: ElaadNL
# SPDX-License-Identifier: Apache-2.0

# Apply fork-only patches to connectedhomeip/connectedhomeip (after submodules are in sync).
# Run from repository root, after: git submodule update --init --recursive
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CHP_DIR="${REPO_ROOT}/connectedhomeip/connectedhomeip"

if ! git -C "${CHP_DIR}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "error: not a git repo: ${CHP_DIR} — run: git submodule update --init --recursive" >&2
  exit 1
fi

apply_one() {
  local patchfile="$1"
  local relpath
  relpath="${patchfile#${REPO_ROOT}/}"

  if (cd "${CHP_DIR}" && git apply --check -- "${patchfile}" 2>/dev/null); then
    (cd "${CHP_DIR}" && git apply -- "${patchfile}")
    echo "Applied: ${relpath}"
    return 0
  fi

  if (cd "${CHP_DIR}" && git apply --reverse --check -- "${patchfile}" 2>/dev/null); then
    echo "Already applied (skipped): ${relpath}"
    return 0
  fi

  echo "error: cannot apply or verify ${relpath} (merge conflict or tree mismatch?)" >&2
  exit 1
}

# Ordered list; add more 0002-*.patch, etc. here as needed.
shopt -s nullglob
for f in "${SCRIPT_DIR}/connectedhomeip/"*.patch; do
  apply_one "${f}"
done
shopt -u nullglob
