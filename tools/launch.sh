#!/bin/bash
# Auto-launcher for darktable dev build on macOS
# Sets required environment variables automatically

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Required for GTK3/GSettings on macOS when running from build tree
export GSETTINGS_SCHEMA_DIR=/opt/homebrew/share/glib-2.0/schemas

DT_BINARY="${SCRIPT_DIR}/build/bin/darktable"

if [ ! -x "$DT_BINARY" ]; then
    echo "Error: darktable binary not found at ${DT_BINARY}" >&2
    echo "Build first: cd build && make -j\$(sysctl -n hw.ncpu)" >&2
    exit 1
fi

exec "$DT_BINARY" "$@"
