# Orchestral Studio Reverb

A free, open-source orchestral reverb experiment. The project is licensed under the MIT License.

**Current milestone:** stereo early-reflection tapped-delay network combined in parallel with the eight-line late FDN.

## Current implementation

- Eight delay lines with sample-rate-dependent lengths in the late FDN.
- Normalized 8-point Hadamard feedback mixing matrix.
- Per-line one-pole low-pass damping in the feedback paths.
- Feedback gains derived from a user-set RT60 target.
- Stereo input injection and stereo output projections.
- Separate stereo early-reflection network with ten interpolated taps.
- Room-size scaling and pre-delay controls for the early-reflection pattern.
- Independent Early, Dry and Late Wet return levels.
- Minimal charcoal/bronze JUCE interface.

The early-reflection network and late FDN are separate DSP components. Their outputs are mixed in parallel with the direct signal, so the early and late returns can be balanced independently. This is an algorithmic starting point rather than a geometrically modelled concert hall.

## Build the standalone DSP tests (no JUCE required)

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
ctest --test-dir build --config Release --output-on-failure
```

## Build the JUCE plugin

Use a local JUCE checkout and point CMake at it:

```powershell
cmake -S . -B build -DJUCE_DIR="C:/JUCE" -DOSR_BUILD_VST3=ON -DOSR_BUILD_TESTS=ON
cmake --build build --config Release
```

Alternatively, allow CMake to fetch JUCE 8.0.6:

```powershell
cmake -S . -B build -DOSR_FETCH_JUCE=ON -DOSR_BUILD_VST3=ON
cmake --build build --config Release
```

Fetching JUCE requires an internet connection and a supported C++ compiler/platform SDK. On Windows, use MSVC for the JUCE plugin build.

## DSP notes

The late FDN feedback gain for each branch starts from:

`g_i = 10^(-3 * T_i / RT60)`

where `T_i` is that branch's delay time. The normalized Hadamard matrix is energy-preserving in ideal arithmetic; damping filters shape the frequency-dependent decay.

The early-reflection network uses a fixed ten-tap stereo pattern with fractional-delay interpolation. Room Size scales the tap times and Pre-delay offsets the pattern. The current early-reflection model is intentionally compact; later work can add multiple room profiles, geometry-derived patterns, and more detailed stereo decorrelation.

## Real-time safety

All delay buffers are allocated in `prepare()`. The per-sample processing paths perform no allocation, locking, file access, or dynamic object creation. Room-size and pre-delay changes are smoothed inside the early-reflection processor. Dry, Early and Late Wet return levels are independently smoothed by the JUCE wrapper.

## License

The original Orchestral Studio Reverb source in this repository is licensed under the MIT License. See [LICENSE](LICENSE). JUCE is a separate dependency and remains under its own license; it is not bundled or relicensed by this project.
