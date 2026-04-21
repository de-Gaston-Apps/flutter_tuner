# Test Fixture WAV Files

Place **16-bit PCM mono WAV** files here for the real-instrument sample tests.

## Expected files

| Filename           | Instrument | Note | Expected Frequency |
|--------------------|------------|------|--------------------|
| `guitar_a4.wav`    | Guitar     | A4   | 440.00 Hz          |
| `piano_c4.wav`     | Piano      | C4   | 261.63 Hz          |
| `violin_g3.wav`    | Violin     | G3   | 196.00 Hz          |
| `cello_c2.wav`     | Cello      | C2   | 65.41 Hz           |
| `ukulele_a4.wav`   | Ukulele    | A4   | 440.00 Hz          |
| `voice_a3.wav`     | Voice      | A3   | 220.00 Hz          |

## Requirements

- **Format:** WAV, 16-bit PCM
- **Channels:** Mono (stereo will use only the first channel)
- **Sample rate:** 44100 Hz recommended
- **Duration:** At least 0.1 seconds (4096+ samples at 44100 Hz)

The test runner will **skip** (not fail) any missing files, so CI stays
green until you add them.
