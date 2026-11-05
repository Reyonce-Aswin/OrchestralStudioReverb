# Orchestral Studio Reverb

A free, open-source orchestral reverb experiment. The project is licensed under the MIT License.

**Current milestone:** first 8-line stereo Feedback Delay Network (FDN), standalone DSP test, and an initial JUCE VST3 shell.

## Current implementation

- Eight delay lines with sample-rate-dependent lengths.
- Normalized 8-point Hadamard feedback mixing matrix.
- Per-line one-pole low-pass damping in the feedback paths.
- Feedback gains derived from a user-set RT60 target.
- Stereo input injection and stereo output projections.
- JUCE parameters for decay, damping, and independent dry/wet return levels (parallel mix).
- Minimal charcoal/bronze starter GUI.

This is an early development build. It is not yet a Synchron Stage / AIR Studios replacement: there is no image-source early reflection engine, room geometry, orchestral positioning, or microphone array yet. The current FDN is intentionally a first listening/test target.

## Build the standalone DSP test (no JUCE required)

### Windows (Visual Studio)

```powershell
cmake -S . -B build -DOSR_BUILD_VST3=OFF -DOSR_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### MSYS2 UCRT64 / MinGW

```bash
cmake -S . -B build -G "MinGW Makefiles" -DOSR_BUILD_VST3=OFF -DOSR_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Build the JUCE plugin

Use a local JUCE checkout (recommended) and point CMake at it:

```powershell
cmake -S . -B build -DJUCE_DIR="C:/JUCE" -DOSR_BUILD_VST3=ON -DOSR_BUILD_TESTS=ON
cmake --build build --config Release
```

Alternatively, allow CMake to fetch JUCE 8.0.6:

```powershell
cmake -S . -B build -DOSR_FETCH_JUCE=ON -DOSR_BUILD_VST3=ON
cmake --build build --config Release
```

Fetching JUCE requires an internet connection. A suitable C++ compiler and the platform SDK are also required.

## DSP notes

The feedback gain for each delay branch starts from:

`g_i = 10^(-3 * T_i / RT60)`

where `T_i` is that branch's delay time. The Hadamard matrix is normalized by `1/sqrt(8)`, making the ideal matrix energy-preserving. Damping filters intentionally make the network frequency-dependent and reduce high-frequency decay.

The current damping is a single-pole low-pass, so the displayed decay is an approximate broadband target, not a calibrated multi-band RT60. The plugin's Dry and Wet controls are independent parallel levels: turning Wet up does not turn Dry down, and both may be set to 100%. Later work should add per-band decay calibration, modulation, fractional delays, early reflections, and proper stereo/microphone decoding.

## Real-time safety

All delay buffers are allocated in `prepare()`. `processSample()` performs no allocation, locking, file access, or dynamic object creation. Parameter changes are currently applied at block boundaries in the JUCE wrapper; separate dry and wet gains are smoothed to avoid abrupt level changes. Further parameter smoothing and safe configuration handoff will be added as the engine grows.

## License

The original Orchestral Studio Reverb source in this repository is licensed under the MIT License. See [LICENSE](LICENSE). JUCE is a separate dependency and remains under its own license; it is not bundled or relicensed by this project.
