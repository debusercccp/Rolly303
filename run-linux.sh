#!/usr/bin/env bash
#
# Launch the AcidBadd 303 standalone on Linux.
#
# JUCE apps are X11-only. On a normal X11 desktop this just runs the app.
# On a pure-Wayland compositor that has no X server running (e.g. niri, river,
# or any setup without xwayland-satellite), this script spins up a temporary
# rootful Xwayland — a single desktop window that hosts the synth — and shuts
# it down again when you close the app.
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP="$HERE/build/AcidBadd_artefacts/Release/Standalone/AcidBadd 303"

if [[ ! -x "$APP" ]]; then
    echo "Standalone not found at:"
    echo "  $APP"
    echo "Build it first:  cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j"
    exit 1
fi

# 1) If we already have a working X display, just use it.
if [[ -n "${DISPLAY:-}" ]] && command -v xdpyinfo >/dev/null 2>&1 && xdpyinfo >/dev/null 2>&1; then
    echo "Using existing X display $DISPLAY"
    exec "$APP" "$@"
fi

# 2) No usable X. We need Xwayland + a Wayland compositor to host it.
if ! command -v Xwayland >/dev/null 2>&1; then
    echo "No working X display and Xwayland is not installed."
    echo "Install it:  sudo apt install xwayland     (Debian/Ubuntu)"
    exit 1
fi

WL="${WAYLAND_DISPLAY:-wayland-1}"
if [[ ! -S "${XDG_RUNTIME_DIR:-/run/user/$(id -u)}/$WL" ]]; then
    echo "No Wayland compositor found (WAYLAND_DISPLAY=$WL) and no usable X server."
    echo "Run this from inside your graphical session."
    exit 1
fi

# Pick a free display number for our temporary Xwayland.
DPYNUM=12
while [[ -e "/tmp/.X11-unix/X$DPYNUM" ]]; do DPYNUM=$((DPYNUM + 1)); done

echo "No X server detected — starting a temporary Xwayland on :$DPYNUM (Wayland=$WL)..."
WAYLAND_DISPLAY="$WL" XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}" \
    Xwayland ":$DPYNUM" -ac -decorate -geometry 960x660 >/tmp/acidbadd-xwayland.log 2>&1 &
XWL_PID=$!
cleanup() { kill "$XWL_PID" 2>/dev/null || true; }
trap cleanup EXIT INT TERM

# Wait for it to come up.
for _ in $(seq 1 40); do
    if DISPLAY=":$DPYNUM" xdpyinfo >/dev/null 2>&1; then break; fi
    sleep 0.25
done
if ! DISPLAY=":$DPYNUM" xdpyinfo >/dev/null 2>&1; then
    echo "Xwayland failed to start. Log:"; cat /tmp/acidbadd-xwayland.log
    exit 1
fi

# Rootful Xwayland runs with no window manager inside it, so the root window
# starts with no cursor (None = invisible). Give it a visible arrow that child
# windows inherit, otherwise the pointer disappears over the synth.
if command -v xsetroot >/dev/null 2>&1; then
    DISPLAY=":$DPYNUM" xsetroot -cursor_name left_ptr 2>/dev/null || true
fi

echo "Launching AcidBadd 303 — close its window to quit."
DISPLAY=":$DPYNUM" "$APP" "$@"
