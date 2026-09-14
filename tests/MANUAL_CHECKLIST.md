# Logic in-app manual checklist (not a pass)

This is for a human with Logic Pro. Completing or reading this file is **not** a pass of the Logic in-app ACs. Automated host-validation prints `FAIL-UNVERIFIED` when Logic.app is missing or the GUI cannot be driven.

Controls (display names, not APVTS IDs): **Drive**, **Output**, **Auto Gain**, **Color**. Color at **0** is the Mackity / v1 sound, **not** “off.” Host text for Color is Classic / Blend / Even; an FL Studio hint bar still showing a number or `%` is not a fail.

## Stereo insert

1. Scan/rescan Audio Units so `mach1` (manufacturer Seto) appears.
2. Create a stereo audio track with a known source (click, loop, or generator).
3. Insert **mach1** as a stereo insert.
4. Raise **Drive**: the processed signal should audibly saturate.
5. **Auto Gain** on: output level should hold closer to the unprocessed loudness than with Auto Gain off at the same Drive.
6. **Auto Gain** off: **Output** should change loudness (lower Output = quieter).

## Mono insert

1. Create a **mono** audio track.
2. Insert **mach1**.
3. Play audio: no crash, and the insert produces sound.

## Reaper VST3 (when `/Applications/REAPER.app` exists)

1. Scan VST3; insert mach1; record/play automation on the **Drive** control (APVTS ID `inTrim`) — the automatable Drive parameter must move Drive (display name Drive, ID unchanged); save and reload the session and confirm parameters restore.
