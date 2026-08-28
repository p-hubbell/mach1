# Character fixtures (48 kHz stereo IEEE-float WAV)

| File | Contents |
|---|---|
| `sine_1khz_m6dbfs_48k.wav` | 1 s, 1 kHz sine at −6 dBFS, L=R |
| `sine_1khz_m6dbfs_48k_mackity_ref.wav` | Same signal through the Mackity topology at A=0.1, B=1.0 (no makeup) |
| `drum_loop_excerpt_48k.wav` | 0.75 s **synthetic** stereo percussion (kick-like decaying oscillator + noise transients). Not a licensed drum loop. |
| `drum_loop_excerpt_48k_mackity_ref.wav` | Same excerpt through that topology at A=0.1, B=1.0 |

## Regenerating

From the repo root, after configuring CMake:

```
cmake --build build --target generate_mackity_fixtures
./build/generate_mackity_fixtures tests/fixtures
```

The generator (`tests/generate_mackity_fixtures.cpp`) is an offline renderer of Mackity’s process loop from `MackityProc.cpp`: DC-A → In Trim `(A*10)²` → biquad LP Q≈0.4317 → clip ±1 then `x − x⁵·0.1768` → biquad LP Q≈1.1582 → DC-B → Out Pad. Coefficients are computed once (not every buffer). **Dither (`frexpf` / TPDF) and denormal noise injection are omitted** so digital zero stays zero in the refs; the 15% RMS character bound is still the gate.

Character tests always load these four files and **fail** if any is missing.
