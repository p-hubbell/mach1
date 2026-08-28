# Logic in-app manual checklist (not a pass)

This is for a human with Logic Pro. Completing or reading this file is **not** a pass of the Logic in-app ACs. Automated host-validation prints `FAIL-UNVERIFIED` when Logic.app is missing or the GUI cannot be driven.

## Stereo insert

1. Scan/rescan Audio Units so `mach1` (manufacturer Seto) appears.
2. Create a stereo audio track with a known source (click, loop, or generator).
3. Insert **mach1** as a stereo insert.
4. Raise **In Trim**: the processed signal should audibly drive / saturate.
5. **AutoGain** on: output level should hold closer to the unprocessed loudness than with AutoGain off at the same drive.
6. **AutoGain** off: **Out Pad** should change loudness (lower pad = quieter).

## Mono insert

1. Create a **mono** audio track.
2. Insert **mach1**.
3. Play audio: no crash, and the insert produces sound.

## Reaper VST3 (when `/Applications/REAPER.app` exists)

1. Scan VST3; insert mach1; record/play In Trim automation; save and reload the session and confirm parameters restore.
