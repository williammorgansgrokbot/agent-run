#!/bin/bash
# Export every printable part (already in print orientation)
set -e
cd "$(dirname "$0")"; mkdir -p stl
OS=${OPENSCAD:-openscad-nightly}
for p in front_shell back_panel bezel knob_channel knob_volume leg plinth plunger light_pipe; do
  $OS --backend=manifold --render -D "part=\"$p\"" -o stl/$p.stl cyd_console_tv.scad 2>&1 | grep -E 'Status|WARN|ERROR' | sed "s/^/$p: /"
done
