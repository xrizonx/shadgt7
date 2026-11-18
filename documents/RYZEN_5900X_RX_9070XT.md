# Ryzen 9 5900X + Radeon RX 9070 XT: renderer validation profile

This profile describes **safe starting defaults**, not a claim of measured performance.
Test on Windows 11 with a current AMD graphics driver and the complete ryGT7 launcher.
The renderer cannot validate GPU driver features or game compatibility at build time.

## Why these are the defaults

- The Ryzen 9 5900X has 12 physical cores / 24 hardware threads. The renderer already
  reserves the last third of physical cores for pipeline compilation on uniform-core
  CPUs. Keep `RYGT_COMPILE_CORES=1` (the default) initially.
- Shader compilation now creates **only the number of workers it can actually run**:
  on a 24-thread system, the active-build cap is 12, rather than creating 24
  workers with half permanently unable to run.
- The RX 9070 XT uses AMD RDNA 4. Keep the Vulkan GPU selection on automatic unless
  the log shows the wrong GPU: the renderer already prefers a Vulkan-capable discrete
  GPU and recognizes Radeon RX 9000 devices.
- `RYGT_COMPILE_PLAY_WIDTH=2` is the existing race default; the project's earlier
  measurements found larger widths could contend with the rendering queue.
- `RYGT_CORE_OWNERS=0` is the existing default. Dedicated renderer cores can
  **hurt** if they leave the game too few physical cores. Benchmark before opting in.
- `RYGT_SHADER_OBJECT` stays **off**: the renderer documents missing garage-icon
  parts in GT7 with shader objects. Do not force it on just because the GPU supports
  the Vulkan extension. If testing that experimental mode, leave `RYGT_SO_CACHE=1`
  and check garage assets, race rendering, and crash recovery.
- `RYGT_ANTI_LAG` is opt-in, only when the driver's `VK_AMD_anti_lag` extension is
  present. Measure latency before enabling it; it is not a guaranteed frame-rate gain.
- Keep the existing fast-flips default in native rendering without frame generation.
  Frame generation and fast flips have separate pacing behavior.

## Repeatable comparison

1. Use the **same game update, track, camera, weather, resolution and renderer
   options**, and first confirm the RX 9070 XT is selected in the Vulkan log.
2. Take a baseline after the first launch and shader warm-up. Run at least three
   repeatable race captures, then compare median frame time, 1% low FPS,
   stutters (for example frames over 33.3 ms / 50 ms), peak VRAM, and GPU
   utilization. Do not report compilation time as rendering throughput.
3. Compare a cold-cache and warm-cache run separately. The initial run must
   build pipelines and can behave quite differently from subsequent runs.
4. Check menus, garage icons, first and later race loads, texture streaming,
   replays, force feedback and clean exit. Recheck after a driver upgrade.
5. If CPU-bound during racing, test only **one** switch at a time, starting with
   `RYGT_COMPILE_PLAY_WIDTH=1` versus `2`. If shader-build backlog is the
   bottleneck, measure the load-screen time and eventual asset completion as
   well as frame pacing. Restore defaults if compilation becomes slower or
   visual elements remain missing.
6. If GPU-bound, benchmark resolution and existing in-game/upscaling controls
   before enabling experimental Vulkan paths. No undocumented RDNA4 hacks,
   forced shader-object mode, or process-wide affinity changes are applied.

## CI coverage

The Windows PR workflow builds the renderer and runs driver-independent
`ResourcePoolSelection`, `ShaderObjectCache`, and `BuildQueue` regression
tests. CI can catch compilation/logic regressions; an actual RX 9070 XT run is
still needed to validate FPS, image correctness, frame pacing and driver loss.

## References

- AMD Ryzen 9 5900X specifications: https://www.amd.com/en/products/processors/desktops/ryzen/5000-series/amd-ryzen-9-5900x.html
- AMD Radeon Developer Tool Suite / RDNA4 support: https://gpuopen.com/learn/radeon-developer-tool-suite-amd-rdna4/
- Radeon GPU Profiler (Vulkan frame captures): https://gpuopen.com/manuals/rgp_manual/
- Renderer switch metadata: `src/video_core/rygt_switches.cpp`
