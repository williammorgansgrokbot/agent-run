#!/bin/bash
# Renders preview PNGs (manifold backend keeps colours with --render)
set -e
cd "$(dirname "$0")"
OS=${OPENSCAD:-openscad-nightly}
R="$OS --backend=manifold --render --projection=p --imgsize=1600,1200 --colorscheme=Tomorrow"
$R -D 'part="assembly"'  --camera=-70,-230,110,83,20,40 --viewall --autocenter -o previews/front_3q_left_usb_side.png cyd_console_tv.scad
$R -D 'part="assembly"'  --camera=230,-210,110,83,20,40 --viewall --autocenter -o previews/front_3q.png cyd_console_tv.scad
$R -D 'part="back_view"' --camera=260,250,120,83,20,40  --viewall --autocenter -o previews/back.png cyd_console_tv.scad
$R -D 'part="exploded"'  --camera=-230,-40,190,88,60,30 --viewall --autocenter -o previews/exploded.png cyd_console_tv.scad
$R -D 'part="assembly"' -D 'base_style="legs"' --camera=-70,-230,110,83,20,40 --viewall --autocenter -o previews/front_3q_legs_variant.png cyd_console_tv.scad
$R -D 'part="section"'   --camera=88,-60,300,88,20,30 --viewall --autocenter -o previews/section_fit.png cyd_console_tv.scad
$R -D 'part="assembly"'  --camera=83,-300,42,83,20,42 --viewall --autocenter -o previews/front_straight.png cyd_console_tv.scad
