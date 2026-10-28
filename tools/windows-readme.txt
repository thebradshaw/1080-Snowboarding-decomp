1080 Snowboarding: Recompiled 1.0.1 for Windows

1. Unzip this folder anywhere (use a new folder, not over an older version).
2. Run 1080SnowboardingRecompiled.exe.
3. Choose "Load ROM" and pick your own 1080 Snowboarding (USA) .z64 ROM.
   It is checked against the expected hash and copied into the app's data folder.
4. Choose "Start Game".

Settings, controls and saves live in %LOCALAPPDATA%\1080SnowboardingRecompiled.
To keep everything next to the exe instead, create an empty file named portable.txt here.

Reporting a problem: each run writes log.txt to that same folder
(%LOCALAPPDATA%\1080SnowboardingRecompiled\log.txt). Send that file after the
problem happens.

Needs Windows 10 or 11 (64-bit) and a GPU with Vulkan (the default) or Direct3D 12 (selectable in Settings > Graphics).
The ROM is not included.

Changes in 1.0.1: Fixed the dark bar that flickered across the bottom of the
screen in the Mountain Village ice cave. New VSync setting (launcher main page
and Settings > Graphics): On (the default) keeps the picture from tearing, Off
shows frames as soon as they are ready.

Changes in 1.0: First public release (the same game as v0.1.40). The HD
Textures option is now called HD Board Textures, and the separate -log.exe is
gone: log.txt (above) has the same information.

Changes in v0.1.40: HD Textures now only change the snowboards (sharper
graphics, smooth edges). Menus, text, the HUD, riders, scenery and the sky are
back to their original N64 textures.

Changes in v0.1.39: HD Textures now also cover the game's 2D art: menu
titles and text, rider names and stat labels, the race HUD (time, speed,
damage, place), title cards, logos and menu backgrounds are all sharper. The
character select cabin is back to its original textures. Screens not seen in
testing yet may still show some original text.

Changes in v0.1.38: HD Textures are included and on by default. Sharper,
4x upscaled textures for menus, riders, boards, signs, scenery, sky and the
HUD, with smooth board edges; snow keeps its original look. Turn them off with
HD Textures on the launcher main page or in Settings > Graphics. The pack is
copied into the mods folder on first start, so it also shows in the Mods menu.

Changes in v0.1.37: The Custom Music option from the v0.1.36 test is removed.

Changes in v0.1.35: New Board Thickness setting (launcher main page and
Settings > Graphics). Thick, the default, gives the boards on the board select
screen a solid dark edge instead of a paper-thin one. Original keeps the N64 look.

Changes in v0.1.34: New CRT Filter setting (launcher main page and Settings >
Graphics): Scanlines, or CRT for a curved TV with its fine colour stripes.
Off by default.

Changes in v0.1.33: Fixed a crash that could happen near the ice cave in Golden
Forest, in the code that draws the ice reflection.

Changes in v0.1.32: The launcher's main page has the Ice Reflection setting,
and Frame Rate lines up with the other settings.

Changes in v0.1.31: The ice reflection is tinted the grey-green of the ice
itself. Ice Reflection Original still gives the N64 look.

Changes in v0.1.30: In widescreen, the sides of 2D screens that only cover the
middle 4:3 area (such as name entry) are now black, instead of keeping leftover
pictures from the screen before. The ice reflection is a little greyer and
bluer.

Changes in v0.1.29: Ice Reflection Solid draws the reflection as one solid
figure in muted, icy colors, keeping the rider's textures and shading, instead
of see-through body parts stacking on each other. The rider is still drawn over
it.

Changes in v0.1.28: Ice Reflection Solid draws the rider's reflection with
the rider's own lighting and textures, instead of the flat, unshaded look of
the original.

Changes in v0.1.27: The Ice Reflection shows on ice again (v0.1.26 put it
exactly at the ice's collision height, just under the ice you see), and the
rider is always drawn over it.

Changes in v0.1.26: Ice Reflection Solid no longer covers the rider's board
and legs or flickers on ice. The reflection is now drawn once, behind the rider,
with a light icy-blue tint so it reads as a reflection seen in the ice.

Changes in v0.1.25: Ice Reflection Solid now applies to the whole rider (in
v0.1.24 only the first group of body parts changed, so it still looked patchy)
and sits just above the ice so it shows steadily. Under Rider: the trail the
board leaves in the snow no longer shows in front of a hill the rider has gone
behind. It takes effect from the next course you load.

Changes in v0.1.24: New Ice Reflection option (Graphics, default Solid). The
upside-down rider on ice is drawn as one evenly see-through figure instead of
patchy overlapping body parts, and fades out as the rider leaves the ice.
Original keeps the N64 look.

Changes in v0.1.23: Fixes a crash that could happen as a race or demo loaded.
The renderer sometimes wrote the last frame of the previous screen back into
memory the game had already reused for the new course.

Changes in v0.1.22: Under Rider: the faint picture of the board that the game
draws on the snow under the rider no longer shows in front of a hill the rider
has gone behind. It takes effect from the next course you load.

Changes in v0.1.21: Under Rider flattens the shadow straight onto the snow under
the rider instead of straight down. On steep walls and banks the old way
stretched the shadow into long thin slivers that could show across the slope
and over hills.

Changes in v0.1.20: The rider's shadow no longer judders at 60 and 120 fps. The
frame smoothing kept losing track of it, so on in-between frames it jumped back
to the last real frame while the rider moved on (this affected both shadow
settings). Under Rider: the shadow sits right on the snow again, so it no
longer covers the board or shows over the top of a crest the rider has gone
behind; it is kept clear of the snow it lies on in a way that doesn't move it
on screen.

Changes in v0.1.19: The Under Rider shadow no longer loses pieces (which
looked like flicker) on bumpy snow; it now sits a little higher above the snow.
The board's part of the shadow, which still showed faintly through hills, is
now hidden by them too.

Changes in v0.1.18: With Under Rider shadows, hills and banks in front of the
rider now hide the shadow instead of it showing through them. (v0.1.17 meant to
do this but changed a different draw, so it had no effect on the rider's
shadow.) The shadow sits a hair above the snow so the snow it lies on doesn't
hide it, and the setting takes effect immediately.

Changes in v0.1.17: An attempt at the hill fix above that changed a different
draw instead of the rider's shadow; v0.1.18 takes it out.

Changes in v0.1.16: Under Rider shadow fades out sooner on jumps and eases in
and out instead of flashing when the ground under the rider changes (over a
crest or a halfpipe wall). The Achievements page no longer lists
RetroAchievements' "Unknown Emulator" notice as an unlock, and the Sign out
button fits its label.

Changes in v0.1.15: Character select and other screens whose frame pacing
varies now smooth at the pace of their slowest frames, using the same whole
frame rates (30, 20, 15...) as the rest of the game. v0.1.14 used in-between
rates there, which could stop the game at the Match Race rider select.

Changes in v0.1.14: RetroAchievements. Pick Achievements in the launcher and
sign in with your RetroAchievements account to see this game's set and earn
achievements as you ride, with a pop-up for each unlock. Only a sign-in token
is saved (achievements_login.json), never your password. Unlocks are kept on
this PC (achievements_local.json) and are not sent to RetroAchievements until
it approves this program. Also: Resolution is set to Auto once for existing
installs, the Under Rider shadow no longer slides into place or sinks into the
snow, and character select and other screens whose frame pacing varies now
get the smooth in-between frames too.

Changes in v0.1.13: New launcher: a snowy title screen with the menu on the
left and the main settings (window, aspect, resolution, frame rate 30/60/120,
draw distance, field of view, rider shadow) on the right. Changes save
straight away and are the same settings as in Settings > Graphics.

Changes in v0.1.12: New Settings > Graphics > Field of View slider (100% to
150%) to see more of the course around the rider. 100% matches the N64.

Changes in v0.1.11: Rider Shadow > Under Rider now fades the shadow out on big
jumps instead of drawing it on whatever is far below (it could land on the far
side of a hill), and steadies it when the ground under the rider changes.

Changes in v0.1.10: New Settings > Graphics > Rider Shadow option. Original
casts the shadow along the sun like the N64, so with a low sun it can land far
from the rider and jump across the slope in the air. Under Rider keeps it
directly beneath the rider.

Changes in v0.1.9: Fixed the rest of the background flickering in the fog. The
renderer rebuilt the camera from parts and lost precision in the depth, so the
fog on distant hills jumped from frame to frame. It now keeps the game's own
camera and only adds the smoothing movement.

Changes in v0.1.8: Fixed the in-between frames made for 60 and 120 fps. The game
moves its near and far planes every frame, and blending them made single frames
show the hills fogged over or distant scenery drawn in front, which looked like
the background flickering in the fog.

Changes in v0.1.7: Draw Distance defaults to Maximum (the whole course is drawn,
so nothing builds in the distance and the far edge no longer flickers through
the fog). Extended now reaches further too. If v0.1.6 saved Extended, it moves
to Maximum once.
Changes in v0.1.6: fixed the crash (black screen) when a race loads: the
renderer could write a late frame over memory the game had just reused.
60 fps is the default again (Settings > Graphics > Framerate,
also set once for anyone v0.1.5 left on Original). Camera cuts no longer show
blended or broken in-between frames. New Draw Distance setting in Settings >
Graphics: Extended (default) draws about three times as far, Maximum draws the
whole course with almost no fog, Original matches the N64. A bad audio frame
no longer closes the game.
Changes in v0.1.5: Framerate defaults to Original, which removes the bad frames
on camera cuts. If you set it before, your setting is kept.
Changes in v0.1.4: log.txt includes a crash report.
Changes in v0.1.3: writes log.txt for bug reports.
Changes in v0.1.2: Match Race no longer crashes at character select, board
select or when the race loads (tested under Wine and Linux).
