MSNX Delay - Mac
=================

A recreation of the Akai MPC3000's built-in 3-channel delay.
Works on Apple Silicon and Intel Macs, macOS 11 or newer.

INSTALL (about 1 minute)
------------------------
1. Double-click "Install.command".
   - If macOS says it can't be opened: right-click it, choose Open,
     then click Open again.
2. Restart your DAW. It shows up as "MSNX Delay".
   (Logic and GarageBand use the Audio Unit version; Ableton, Bitwig,
   Reaper, FL Studio and most others can use the VST3.)

WHAT THE INSTALLER DOES
-----------------------
It copies the plugins into your user plugin folders:
   ~/Library/Audio/Plug-Ins/VST3/MSNX Delay.vst3
   ~/Library/Audio/Plug-Ins/Components/MSNX Delay.component
and tells macOS you trust them. That's needed because this plugin
isn't signed with a paid Apple developer account.

PREFER TO DO IT BY HAND?
------------------------
1. Copy the two files above into those folders.
2. Open Terminal and run:
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"MSNX Delay.vst3" ~/Library/Audio/Plug-Ins/Components/"MSNX Delay.component"

JUST WANT TO TRY IT?
--------------------
Open "MSNX Delay.app" (right-click > Open the first time).
In its Options, pick your audio device and turn on an input.

QUICK START
-----------
- Channel 1 starts on. Set its Time to ~375 ms and Feedback to ~30%.
- Turn up Channel 2 and Channel 3 Volume for more echoes at other times.
- Careful: if the three Feedback knobs add up to more than 100%,
  the echoes grow forever (just like the real MPC3000). Keep your volume low.
