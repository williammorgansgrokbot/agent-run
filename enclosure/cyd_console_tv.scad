// =====================================================================
//  "Curtis Mathes 1978" miniature console-TV cabinet
//  for the Hosyond / LCDWiki E32R40T 4.0" ESP32-32E display (ST7796S, 480x320,
//  XPT2046 resistive touch).  Board is mounted LANDSCAPE, USB-C end on the LEFT.
//
//  Sources (downloaded 2026-10-17):
//   - https://www.lcdwiki.com/4.0inch_ESP32-32E_Display
//   - https://www.lcdwiki.com/res/E32R40T/E32R40T_Size.pdf   (LCM outline V1.0, 2025-04-15)
//   - https://www.lcdwiki.com/res/E32R40T/E32R40T_3D.zip     (STEP model, used for
//     component positions/heights: USB-C, buttons, RGB LED, sockets)
//
//  Coordinate system (assembly): X = right, Z = up, Y = depth (front face at y=0,
//  back at y=body_d, the cabinet looks toward -Y).
//  Board coords: u = mm from the USB-C short edge, v = mm from the long edge that
//  carries the I2C/SPI/SPEAKER/IO35 sockets (that edge is at the bottom).
//
//  Usage:  openscad -D 'part="front_shell"' -o front_shell.stl cyd_console_tv.scad
//  parts:  assembly | exploded | back_view | section | clash
//          front_shell | back_panel | bezel | knob_channel | knob_volume
//          leg | plinth | plunger | light_pipe | print_plate_small
// =====================================================================

part = "assembly";

/* [Print / fit] */
wall      = 2.0;     // general wall thickness
clr       = 0.3;     // clearance around PCB / LCD
$fn       = 64;

/* [Board: LCDWiki E32R40T (drawing E32R40T_Size.pdf)] */
pcb_l     = 111.11;  // PCB long side
pcb_w     = 60.88;   // PCB short side
pcb_t     = 1.60;
pcb_r     = 3.50;    // corner radius (4-R3.50)
hole_u    = 3.50;    // hole centre from short edges (104.11 pitch)
hole_v    = 3.80;    // hole centre from long edges  (53.28 pitch)
hole_d    = 3.20;    // 4-ø3.20 (pad ø6.60)
lcd_u0    = 8.27;    // LCD backlight outline starts 8.27 from USB edge (94.57 long)
lcd_l     = 94.57;
lcd_stack = 4.05;    // RTP + LCD + back tape above PCB front (5.65 total - 1.60 PCB)
aa_u0     = 16.67;   // LCD active area 83.52 x 55.68 (10.92 from antenna edge, 2.60 from long edges)
aa_v0     = 2.60;
aa_l      = 83.52;
aa_w      = 55.68;
va_l      = 85.22;   // RTP visible area 85.22 x 56.88 (window must stay inside this)
va_w      = 56.88;
back_h    = 4.70;    // tallest rear component (speaker socket, 6.30 - 1.60)
// rear components (from STEP model, converted to u/v)
usb_v     = 30.44;   // USB-C centre (15.38 + 15.06 on drawing), on the u=0 edge
usb_wv    = 8.94;    // receptacle width (along v)
usb_hy    = 4.38;    // receptacle height (mid-mount, -0.50..-4.88 below PCB front)
usb_cy    = 2.69;    // receptacle centre below PCB front surface
btn_u     = 3.26;    // RESET/BOOT centre from USB edge
reset_v   = 15.36;   // RESET (EN)  - 15.38 from edge on drawing
boot_v    = 45.48;   // BOOT (IO0)
btn_top   = 2.50;    // button height above PCB back
led_u     = 52.56;   // 5050 RGB LED centre (STEP)  - on the BACK of the PCB
led_v     = 29.90;
led_h     = 1.60;
spk_sock_u= 48.19;   // 1.25mm 2P speaker socket centre, on the v=0 edge

/* [Cabinet] */
board_x0  = wall + clr;          // PCB left edge (USB end)
board_z0  = 10.5;                // PCB bottom edge height in cabinet
strip_w   = 60;                  // right-hand control/speaker strip width
body_d    = 40;                  // cabinet depth (front face to back face)
body_h    = 84;                  // cabinet height (without legs)
top_t     = 4;                   // top slab thickness
top_ov    = 1.5;                 // top slab side overhang
standoff_gap = 0.10;             // extra air in front of the touch glass (do not pre-load the RTP)
standoff_d   = 5.5;
pilot_d      = 2.2;              // M2.5 self-tap pilot (use 3.5-3.6 for M2.5 heat-set inserts)
boss_d       = 6.4;              // back-panel screw bosses
boss_pilot_d = 2.2;              // M2.5 self-tap (3.6 for M2.5x4 heat-set, 3.2 for M2)
boss_pilot_l = 9;
panel_screw_d= 2.8;              // clearance hole in back panel
panel_t      = wall;
rib_t        = 1.5;

/* [Screen bezel] */
scr_a     = 42.6;    // half-width of CRT window (AA half = 41.76, VA half = 42.61)
scr_b     = 28.4;    // half-height of CRT window (AA half = 27.84, VA half = 28.44)
scr_n     = 7;       // squircle exponent (higher = squarer CRT corners)
scr_chamf = 1.2;     // 45deg chamfer on the shell window
bezel_a   = 50;      // bezel ring outer half-width
bezel_b   = 34;      // bezel ring outer half-height
bezel_n   = 4.2;
bezel_t   = 2.4;     // bezel ring thickness
bezel_rec = 0.6;     // depth of locating recess in shell

/* [Speaker / controls] */
spk_d     = 36;      // speaker diameter (28..40); ring ID = spk_d + 0.4
spk_ring_h= 4;
spk_z     = 30;
grille_w  = 44;     // slatted grille area (wider than the speaker, console style)
grille_h  = 40;
slat_h    = 1.6;
slat_pitch= 3.2;
knob_z    = 66.5;
knob1_d   = 17;      // channel selector
knob2_d   = 12.5;    // volume
knob_h1   = 8;
knob_h2   = 7;
knob_peg_d= 2.6;     // peg on knob
knob_hole_d=2.9;     // blind hole in front wall
brand_text= "CURTIS MATHES";

/* [Base] */
leg_h     = 11;
leg_top_d = 8;
leg_bot_d = 5;
leg_peg_d = 4.0;
leg_peg_l = 4.5;
socket_d  = 4.4;
rail_w    = 9;
rail_h    = 4;
leg_inset_x = 13;
leg_inset_y = 9;
plinth_h  = 9;
plinth_in = 5;

woodgrain = true;
base_style = "plinth";   // "plinth" (recessed toe-kick) or "legs" (4 short tapered legs)

/* [Plungers / light pipe] */
plunger_d   = 2.6;
plunger_fl_d= 4.6;
tube_id     = 3.0;
tube_od     = 5.6;
pipe_d      = 3.0;

// ------------------------------------------------------------- derived
board_x1  = board_x0 + pcb_l;
pcb_front_y = wall + lcd_stack + standoff_gap;   // front face of PCB
pcb_back_y  = pcb_front_y + pcb_t;
strip_x0  = board_x1 + clr + rib_t + 1.0;      // divider line between screen and controls
body_w    = strip_x0 + strip_w;
W = body_w; H = body_h; D = body_d;
in_top    = H - wall;

function bx(u) = board_x0 + u;          // board u -> cabinet x
function bz(v) = board_z0 + v;          // board v -> cabinet z
scr_x     = bx(aa_u0 + aa_l/2);
scr_z     = bz(aa_v0 + aa_w/2);
spk_x     = strip_x0 + strip_w/2 - 1;
knob1_x   = strip_x0 + strip_w*0.30;
knob2_x   = strip_x0 + strip_w*0.76;
holes_uv  = [[hole_u,hole_v],[hole_u,pcb_w-hole_v],[pcb_l-hole_u,hole_v],[pcb_l-hole_u,pcb_w-hole_v]];
bosses_xz = [[wall+boss_d/2-0.6, wall+boss_d/2-0.6], [wall+boss_d/2-0.6, in_top-boss_d/2+0.6],
             [W-wall-boss_d/2+0.6, wall+boss_d/2-0.6], [W-wall-boss_d/2+0.6, in_top-boss_d/2+0.6],
             [strip_x0+2.2, wall+boss_d/2-0.6], [strip_x0+2.2, in_top-boss_d/2+0.6]];
legs_xy   = [[leg_inset_x, leg_inset_y],[W-leg_inset_x, leg_inset_y],
             [leg_inset_x, D-leg_inset_y-2],[W-leg_inset_x, D-leg_inset_y-2]];
reset_xz  = [bx(btn_u), bz(reset_v)];
boot_xz   = [bx(btn_u), bz(boot_v)];
led_xz    = [bx(led_u), bz(led_v)];
btn_top_y = pcb_back_y + btn_top;
led_top_y = pcb_back_y + led_h;
usb_y     = pcb_front_y + usb_cy;
usb_z     = bz(usb_v);

echo(str("OVERALL body W x H x D = ", W, " x ", H, " x ", D,
         "  with top overhang W = ", W+2*top_ov, "  height on legs = ", H+leg_h));
echo(str("screen window centre x,z = ", scr_x, ", ", scr_z));

// ------------------------------------------------------------- helpers
function squircle(a,b,n,N=160) = [for(i=[0:N-1]) let(t=360*i/N, c=cos(t), s=sin(t))
    [a*sign(c)*pow(abs(c),2/n), b*sign(s)*pow(abs(s),2/n)]];
// extrude a 2D shape drawn in the XZ plane (2D y = Z) between y0 and y1
module xz(y0,y1){ translate([0,y1,0]) rotate([90,0,0]) linear_extrude(y1-y0) children(); }
module rrect(w,h,r){ offset(r) offset(-r) square([w,h]); }
module rrect_c(w,h,r){ translate([-w/2,-h/2]) rrect(w,h,r); }
module slot2d(l,h){ hull(){ translate([-l/2+h/2,0]) circle(d=h); translate([l/2-h/2,0]) circle(d=h);} }

// ------------------------------------------------------------- FRONT SHELL
module body_outer(){
    // main carcass
    cube([W, D, H - top_t]);
    // top slab with side overhang, slight chamfer on the overhang
    hull(){
        translate([-top_ov, 0, H-top_t+0.8]) cube([W+2*top_ov, D, top_t-1.6]);
        translate([-top_ov+0.8, 0, H-top_t]) cube([W+2*top_ov-1.6, D, top_t]);
    }
}

module window2d(grow=0){ translate([scr_x, scr_z]) offset(delta=grow) polygon(squircle(scr_a, scr_b, scr_n)); }
module bezel2d(grow=0){ translate([scr_x, scr_z]) offset(delta=grow) polygon(squircle(bezel_a, bezel_b, bezel_n)); }

module front_engraving(){
    d = 0.6;
    // decorative double trim line under the top slab and a kick-rail line
    for(z=[H-top_t-1.8, H-top_t-3.8]) xz(-1,0.8) translate([1.5,z]) square([W-3,1.0]);
    xz(-1,0.8) translate([1.5,4.0]) square([W-3,1.0]);
    // vertical divider between screen section and control strip
    xz(-1,d) translate([strip_x0-0.5, 5.0]) square([1.0, H-top_t-1.8-5.0]);
    // inset panel outline around the control strip
    xz(-1,0.5) translate([strip_x0+2.5, 7.5]) difference(){ rrect(strip_w-5, H-top_t-12-7.5, 2.5); offset(-0.6) rrect(strip_w-5, H-top_t-12-7.5, 2.5); }
    // grille frame
    xz(-1,0.6) translate([spk_x, spk_z]) difference(){ rrect_c(grille_w+5, grille_h+5, 3); rrect_c(grille_w+3.6, grille_h+3.6, 2.3); }
    // nameplate
    xz(-1,0.5) translate([spk_x, spk_z+grille_h/2+5.2]) difference(){ rrect_c(38,6,1.5); rrect_c(36.8,4.8,1); }
    xz(-1,0.5) translate([spk_x, spk_z+grille_h/2+5.2]) text(brand_text, size=2.8, font="Liberation Serif:style=Bold", halign="center", valign="center", spacing=1.05);
    // channel dial ticks
    for(a=[-135:22.5:135]) xz(-1,0.5) translate([knob1_x,knob_z]) rotate(-a) translate([-0.35, knob1_d/2+1.2]) square([0.7,1.8]);
    for(a=[-120:30:120])  xz(-1,0.5) translate([knob2_x,knob_z]) rotate(-a) translate([-0.3, knob2_d/2+1.0]) square([0.6,1.4]);
    // wood grain (shallow, along the length of each face)
    if (woodgrain) {
        // front stiles/rails
        for(i=[0:5]) xz(-1,0.25) grain_line(1.2, W-1.2, 1.2+i*0.55+ (i%2)*0.2, 0.3, i);
        // top surface
        for(i=[0:13]) translate([0,0,H-0.25]) linear_extrude(1) grain_line(-top_ov+1, W+top_ov-1, 2+i*2.7+(i%3)*0.4, 0.35, i+7);
        // sides
        for(s=[0,1]) for(i=[0:11]) let(x = s==0 ? -0.75 : W+0.75) translate([x,0,0]) rotate([0,0,90]) rotate([90,0,0])
            translate([0,0,-0.5]) linear_extrude(1) grain_line(1, D-1, 6+i*6.2+(i%2)*1.3, 0.35, i+30);
    }
}
// wavy grain line in 2D from x0..x1 around y
module grain_line(x0,x1,y,w,seed){
    N=28; amp=0.35+0.25*sin(seed*47);
    pts=[for(i=[0:N]) let(x=x0+(x1-x0)*i/N) [x, y+amp*sin(i*37+seed*61)+0.2*sin(i*91+seed*13)]];
    for(i=[0:N-1]) hull(){ translate(pts[i]) circle(d=w,$fn=8); translate(pts[i+1]) circle(d=w,$fn=8); }
}

module front_shell(){
    difference(){
        union(){
            difference(){
                body_outer();
                // hollow, open at the back
                translate([wall, wall, wall]) cube([W-2*wall, D, in_top-wall]);
            }
            // PCB standoffs (touch glass rests just behind the front wall)
            for(h=holes_uv) translate([bx(h[0]), wall-0.01, bz(h[1])]) rotate([-90,0,0])
                cylinder(d=standoff_d, h=pcb_front_y-wall+0.01);
            // locating ribs around PCB/LCD (only up to the PCB back face so they never touch rear parts)
            translate([wall, wall-0.01, board_z0-clr-rib_t]) cube([pcb_l+2*clr, pcb_back_y-0.25-wall, rib_t]);
            translate([wall, wall-0.01, board_z0+pcb_w+clr]) cube([pcb_l+2*clr, pcb_back_y-0.25-wall, rib_t]);
            translate([board_x1+clr, wall-0.01, board_z0+6]) cube([rib_t, pcb_back_y-0.25-wall, pcb_w-12]);
            // back-panel screw bosses (full-depth columns, print without supports)
            for(b=bosses_xz) translate([b[0], wall-0.01, b[1]]) rotate([-90,0,0]) cylinder(d=boss_d, h=D-panel_t-wall+0.01);
            // speaker mounting ring
            translate([spk_x, wall-0.01, spk_z]) rotate([-90,0,0]) difference(){
                cylinder(d=spk_d+0.4+3.2, h=spk_ring_h);
                translate([0,0,-1]) cylinder(d=spk_d+0.4, h=spk_ring_h+2);
                for(a=[0,120,240]) rotate(a+90) translate([0,-1.5,1.2]) cube([spk_d,3,spk_ring_h]); // glue/wire notches
            }
            // leg-socket rails along the floor (grow from the front wall)
            for(x=[leg_inset_x, W-leg_inset_x]) translate([x-rail_w/2, wall-0.01, wall-0.01]) cube([rail_w, D-panel_t-wall-2.5, rail_h]);   // stops short of the back-panel stiffening rib
        }
        // CRT window with 45deg chamfer
        hull(){ xz(-1,0.01) window2d(scr_chamf); xz(scr_chamf, scr_chamf+0.01) window2d(0); }
        xz(-1, wall+1) window2d(0);
        // bezel-ring locating recess
        xz(-1, bezel_rec) bezel2d(0.25);
        // speaker grille slats (chamfered louvres)
        let(n = floor((grille_h-1)/slat_pitch), gw = grille_w)
        for(i=[0:n-1]) let(z = spk_z - (n-1)*slat_pitch/2 + i*slat_pitch) {
            xz(-1, wall+1) translate([spk_x, z]) slot2d(gw - 2*abs(i-(n-1)/2)*0.0, slat_h);
            hull(){ xz(-1,0.01) translate([spk_x, z]) slot2d(gw+0.8, slat_h+0.8); xz(0.4,0.41) translate([spk_x,z]) slot2d(gw, slat_h); }
        }
        // knob peg holes (blind)
        for(k=[[knob1_x],[knob2_x]]) translate([k[0], -1, knob_z]) rotate([-90,0,0]) cylinder(d=knob_hole_d, h=1+wall-0.6);
        front_engraving();
        // USB-C access in left side wall (with counterbore for fat plug over-moulds)
        translate([-1, usb_y, usb_z]) rotate([0,90,0]) linear_extrude(wall+2) rrect_c(usb_wv+1.2, usb_hy+1.0, 1.2);
        translate([-1, usb_y, usb_z]) rotate([0,90,0]) linear_extrude(1+1.0) rrect_c(13.0, 7.6, 3.2);
        // PCB standoff pilot holes (stop 1 mm short of the front face)
        for(h=holes_uv) translate([bx(h[0]), 1.0, bz(h[1])]) rotate([-90,0,0]) cylinder(d=pilot_d, h=20);
        // boss pilot holes
        for(b=bosses_xz) translate([b[0], D-panel_t-boss_pilot_l, b[1]]) rotate([-90,0,0]) cylinder(d=boss_pilot_d, h=boss_pilot_l+1);
        // leg sockets
        for(l=legs_xy) translate([l[0], l[1], -1]) cylinder(d=socket_d, h=1+wall+rail_h-0.8);
        // little vent strip in the floor behind the board (warm air in, out the back)
        for(i=[0:6]) translate([30+i*9, 16, -1]) cube([5, 16, wall+2]);
    }
}

// ------------------------------------------------------------- BACK PANEL
module back_panel(){
    pw = W - 2*wall - 0.4; ph = in_top - wall - 0.4;
    difference(){
        union(){
            translate([wall+0.2, D-panel_t, wall+0.2]) cube([pw, panel_t, ph]);
            // button plunger guide tubes
            for(p=[reset_xz, boot_xz]) translate([p[0], btn_top_y+2.3, p[1]]) rotate([-90,0,0]) cylinder(d=tube_od, h=D-panel_t-(btn_top_y+2.3)+0.01);
            // LED light-pipe guide tube
            translate([led_xz[0], led_top_y+4, led_xz[1]]) rotate([-90,0,0]) cylinder(d=pipe_d+2.6, h=D-panel_t-(led_top_y+4)+0.01);
            // stiffening frame rib
            translate([wall+0.2+3.5, D-panel_t-2, wall+0.2+3.5]) difference(){
                cube([pw-7, 2.01, ph-7]); translate([1.2,-1,1.2]) cube([pw-9.4, 4, ph-9.4]); }
        }
        // screw holes + countersink-ish counterbore
        for(b=bosses_xz) translate([b[0], D+1, b[1]]) rotate([90,0,0]) { cylinder(d=panel_screw_d, h=10); cylinder(d=5.2, h=1.6); }
        // keep the frame rib away from the bosses
        for(b=bosses_xz) translate([b[0], D-panel_t-3, b[1]]) rotate([-90,0,0]) cylinder(d=boss_d+1.2, h=3.01);
        // plunger + light pipe holes
        for(p=[reset_xz, boot_xz]) translate([p[0], 0, p[1]]) rotate([-90,0,0]) cylinder(d=tube_id, h=D+2);
        translate([led_xz[0], 0, led_xz[1]]) rotate([-90,0,0]) cylinder(d=pipe_d+0.3, h=D+2);
        // LED "pilot" window bezel
        translate([led_xz[0], D-0.5, led_xz[1]]) rotate([-90,0,0]) cylinder(d=7, h=1);
        // vents over the electronics (horizontal slots)
        for(r=[0:7]) for(c=[[24,49],[70,108]]) translate([0, D+1, 0]) rotate([90,0,0])
            linear_extrude(panel_t+3.5) translate([(c[0]+c[1])/2, 16+r*6.6]) slot2d(c[1]-c[0], 2.4);
        // small vents behind the control strip, above the speaker
        for(r=[0:2]) translate([0, D+1, 0]) rotate([90,0,0]) linear_extrude(panel_t+3.5) translate([spk_x, 60+r*5.5]) slot2d(30, 2.2);
        // engraved labels (mirrored so they read correctly from behind)
        translate([0, D+0.01, 0]) rotate([90,0,0]) linear_extrude(0.5) {
            translate([reset_xz[0]+4.5, reset_xz[1]]) mirror([1,0]) text("RESET", size=3, font="Liberation Sans:style=Bold", halign="right", valign="center");
            translate([boot_xz[0]+4.5, boot_xz[1]]) mirror([1,0]) text("BOOT", size=3, font="Liberation Sans:style=Bold", halign="right", valign="center");
            translate([wall+4, usb_z]) mirror([1,0]) text("USB-C >", size=3, font="Liberation Sans:style=Bold", halign="right", valign="center");
            translate([led_xz[0]+5, led_xz[1]]) mirror([1,0]) text("RGB", size=3, font="Liberation Sans:style=Bold", halign="right", valign="center");
            translate([spk_x, 30]) mirror([1,0]) text("MODEL E32R40T", size=2.6, font="Liberation Sans:style=Bold", halign="center");
            translate([spk_x, 24]) mirror([1,0]) text("CAUTION: NO USER-", size=2.2, font="Liberation Sans", halign="center");
            translate([spk_x, 20.5]) mirror([1,0]) text("SERVICEABLE PARTS", size=2.2, font="Liberation Sans", halign="center");
            translate([spk_x, 17]) mirror([1,0]) text("INSIDE", size=2.2, font="Liberation Sans", halign="center");
        }
    }
}

// ------------------------------------------------------------- BEZEL RING (print flat, back face down)
module bezel_ring(){   // built in print orientation: z=0 is the back face (sits in recess)
    difference(){
        hull(){ linear_extrude(bezel_t-0.8) bezel2d0(0); linear_extrude(bezel_t) bezel2d0(-0.8); }
        // continue the CRT funnel
        hull(){ translate([0,0,0.4]) linear_extrude(0.01) window2d0(0.05);
                translate([0,0,bezel_t]) linear_extrude(0.02) window2d0(bezel_t*1.0); }
        translate([0,0,-1]) linear_extrude(bezel_t+2) window2d0(0);
        // fine "mask" line
        translate([0,0,bezel_t-0.4]) linear_extrude(1) difference(){ window2d0(bezel_t+2.6); window2d0(bezel_t+2.0); }
    }
}
module window2d0(g=0){ offset(delta=g) polygon(squircle(scr_a, scr_b, scr_n)); }
module bezel2d0(g=0){ offset(delta=g) polygon(squircle(bezel_a, bezel_b, bezel_n)); }

// ------------------------------------------------------------- KNOBS (print face down, peg up)
module knob(d, h, flutes){
    difference(){
        union(){
            // front cap with chamfer
            cylinder(d1=d-1.6, d2=d, h=0.8);
            translate([0,0,0.8]) cylinder(d=d, h=h-0.8-1.5);
            // skirt flange at the back
            translate([0,0,h-1.5]) cylinder(d1=d, d2=d+2.2, h=1.5);
        }
        // flutes
        for(i=[0:flutes-1]) rotate(i*360/flutes) translate([d/2+0.15, 0, 1.4]) cylinder(d=1.3, h=h-2.9, $fn=12);
        // dished face + pointer groove
        translate([0,0,-0.01]) cylinder(d1=d-3.4, d2=d-5, h=0.6);
        translate([-0.5, 0, -0.01]) cube([1.0, d/2-0.6, 0.9]);
    }
    // centre boss on face
    cylinder(d=d*0.32, h=0.6);
    // pointer bar
    translate([-0.6, d*0.16, 0]) cube([1.2, d/2-0.8-d*0.16, 0.5]);
    // peg
    translate([0,0,h-0.01]) cylinder(d=knob_peg_d, h=wall-0.6-0.2);
}
module knob_channel(){ knob(knob1_d, knob_h1, 24); }
module knob_volume(){  knob(knob2_d, knob_h2, 18); }

// ------------------------------------------------------------- LEGS + PLINTH
module leg(){  // foot at z=0, peg up
    cylinder(d1=leg_bot_d, d2=leg_top_d, h=leg_h);
    translate([0,0,leg_h-0.01]) cylinder(d=leg_peg_d, h=leg_peg_l);
    // brass ferrule ring (paint gold)
    cylinder(d=leg_bot_d+0.6, h=1.6);
}
module plinth(){  // alternative to the legs: recessed toe-kick plinth with pegs into the leg sockets
    pi = plinth_in;
    difference(){
        translate([pi, pi, 0]) cube([W-2*pi, D-2*pi, plinth_h]);
        translate([pi+wall, pi+wall, -1]) cube([W-2*pi-2*wall, D-2*pi-2*wall, plinth_h+2]);
        // air inlets in the rear wall (feed the floor vents of the cabinet)
        for(i=[0:3]) translate([35+i*30, D-pi-wall-1, -1]) cube([14, wall+2, 4]);
    }
    for(l=legs_xy) {
        translate([l[0]-6, l[1]-5, 0]) intersection(){ cube([12,10,plinth_h]); translate([-l[0]+6+pi,-l[1]+5+pi,0]) cube([W-2*pi, D-2*pi, plinth_h]); }
        translate([l[0], l[1], plinth_h-0.01]) cylinder(d=leg_peg_d, h=leg_peg_l);
    }
}

// ------------------------------------------------------------- PLUNGER + LIGHT PIPE
module plunger(){  // flange (button end) on the bed
    cylinder(d=plunger_fl_d, h=1.0);
    cylinder(d=plunger_d, h= (D + 0.8) - (btn_top_y + 0.35));
}
module light_pipe(){ // print in clear/natural PETG, or replace with 3 mm acrylic rod
    L = D - (led_top_y + 0.5) + 0.3;
    cylinder(d=pipe_d, h=L);
    translate([0,0,L-0.01]) sphere(d=pipe_d, $fn=24);
}

// ------------------------------------------------------------- DUMMIES (fit check)
// rear components from STEP model: [px0,px1,py0,py1,zS(min, mm below PCB front)]
step_parts = [[59.10,66.75,55.99,60.09,-6.30],[81.82,89.47,2.10,7.29,-5.00],[61.12,71.27,0.51,5.70,-5.00],
              [12.92,23.07,55.18,60.37,-5.00],[35.92,46.07,55.18,60.37,-5.00],[77.85,85.50,55.19,60.38,-5.00],
              [0.30,25.80,20.75,38.75,-4.80],[106.25,109.45,13.30,17.50,-4.10],[106.25,109.45,43.42,47.62,-4.10],
              [60.43,65.45,43.57,49.61,-3.20],[55.78,61.32,28.42,33.53,-3.20],[73.54,79.77,26.21,36.19,-3.17],
              [35.5,51.1,2.2,17.8,-3.45]];   // last = microSD socket, estimated from the board image (not in STEP)
module board_dummy(){
    // PCB
    color("forestgreen") difference(){
        translate([board_x0, pcb_back_y, board_z0]) rotate([90,0,0]) linear_extrude(pcb_t) translate([0,0]) rrect(pcb_l, pcb_w, pcb_r);
        for(h=holes_uv) translate([bx(h[0]), 0, bz(h[1])]) rotate([-90,0,0]) cylinder(d=hole_d, h=50);
    }
    // LCD + touch glass
    color([0.15,0.15,0.17]) translate([bx(lcd_u0), pcb_front_y-lcd_stack, board_z0]) cube([lcd_l, lcd_stack, pcb_w]);
    color([0.20,0.35,0.55]) translate([bx(aa_u0), pcb_front_y-lcd_stack-0.02, bz(aa_v0)]) cube([aa_l, 0.05, aa_w]);
    // rear parts (STEP px measured from antenna edge => u = pcb_l - px; v = pcb_w - py)
    for(p=step_parts) color("silver")
        translate([bx(pcb_l-p[1]), pcb_back_y, bz(pcb_w-p[3])]) cube([p[1]-p[0], -p[4]-pcb_t, p[3]-p[2]]);
    // USB-C (mid-mount, overhangs edge by 0.37)
    color("silver") translate([bx(-0.37), usb_y-usb_hy/2, usb_z-usb_wv/2]) cube([7.58, usb_hy, usb_wv]);
}
module speaker_dummy(){ color("dimgray") translate([spk_x, wall+0.2, spk_z]) rotate([-90,0,0]) { cylinder(d=spk_d, h=1.5); cylinder(d=spk_d*0.55, h=5); } }

// ------------------------------------------------------------- PLACEMENT
wood = [0.45,0.27,0.13];
module place_bezel(dy=0){ translate([scr_x, -bezel_t+bezel_rec+dy, scr_z]) rotate([-90,0,0]) mirror([0,0,1]) translate([0,0,-bezel_t]) bezel_ring(); }
module place_knobs(dy=0){
    translate([knob1_x, -knob_h1+dy, knob_z]) rotate([-90,0,0]) rotate([0,0,180]) rotate([0,0,-30]) knob_channel();
    translate([knob2_x, -knob_h2+dy, knob_z]) rotate([-90,0,0]) rotate([0,0,180]) rotate([0,0,40]) knob_volume();
}
module place_legs(dz=0){ for(l=legs_xy) translate([l[0], l[1], -leg_h-dz]) leg(); }
module place_plungers(dy=0){ for(p=[reset_xz, boot_xz]) translate([p[0], btn_top_y+0.35+dy, p[1]]) rotate([-90,0,0]) plunger(); }
module place_pipe(dy=0){ translate([led_xz[0], led_top_y+0.5+dy, led_xz[1]]) rotate([-90,0,0]) light_pipe(); }

module assembly(ex=0, show_board=true){
    color(wood) front_shell();
    color([0.40,0.24,0.11]) translate([0, ex*4.2, 0]) back_panel();
    color([0.08,0.08,0.08]) place_bezel(-ex*0.6);
    color([0.80,0.80,0.82]) place_knobs(-ex*0.7);
    if (base_style=="legs") color([0.30,0.18,0.08]) place_legs(ex*0.6);
    else color([0.10,0.07,0.05]) translate([0,0,-plinth_h-ex*0.6]) plinth();
    color([0.85,0.75,0.55]) place_plungers(ex*3.6);
    color([0.9,0.9,1.0,0.8]) place_pipe(ex*3.6);
    if (show_board) translate([0, ex*2.2, 0]) board_dummy();
    if (show_board) translate([0, ex*0.5, 0]) speaker_dummy();
}

// ------------------------------------------------------------- OUTPUT
if (part=="assembly")      assembly(0);
else if (part=="exploded") assembly(25);
else if (part=="back_view") assembly(0, false);
else if (part=="section")  intersection(){ assembly(0); translate([-10,-20,-30]) cube([W+20, D+40, usb_z+30]); }
else if (part=="section2") intersection(){ assembly(0); translate([-10,-20,-20]) cube([reset_xz[0], D+40, H+40]); }
else if (part=="clash2")   intersection(){ front_shell(); union(){ back_panel(); place_plungers(); place_pipe(); } }
else if (part=="clash")    intersection(){ union(){ front_shell(); back_panel(); place_plungers(); place_pipe(); } board_dummy(); }
// ---- printable parts, already in print orientation
else if (part=="front_shell") rotate([90,0,0]) front_shell();                        // front face on the bed
else if (part=="back_panel")  translate([0,0,D]) rotate([-90,0,0]) back_panel();     // outer face on the bed
else if (part=="bezel")        bezel_ring();
else if (part=="knob_channel") knob_channel();
else if (part=="knob_volume")  knob_volume();
else if (part=="leg")          leg();
else if (part=="plinth")       plinth();
else if (part=="plunger")      plunger();
else if (part=="light_pipe")   light_pipe();
else if (part=="speaker_dummy") speaker_dummy();
