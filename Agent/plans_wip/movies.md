# Movies (briefings, intro, cutscenes) - options and findings

Status: **not started** (decision pending). `CODE/VQASTUB.CPP` stubs all `VQA_*` calls (`VQA_Open` returns -1), so every movie is skipped silently. Written 2026-11-07 after the owner asked to discuss porting the VQA player.

## What the owner's game data actually contains
Steam "CnC Remastered Collection", `Data\CNCDATA\RED_ALERT\`:
- `CD1\`, `AFTERMATH\` hold only `MAIN.MIX`, `REDALERT.MIX`, `EXPAND*.MIX`, `HIRES1.MIX`, `LORES1.MIX`, `REDALERT.INI` - **no `MOVIES*.MIX`, no `.VQA`**.
- `COUNTERSTRIKE\` additionally has `SIZZLE.VQA` (4.4 MB) and `SIZZLE2.VQA` (4.7 MB): classic VQA ("FORM....WVQA", `VQHD` header). Only these two are used by `CODE/INIT.CPP:1350` (attract/idle intro `Play_Movie(VQ_SIZZLE...)`).
- All other Red Alert movies were replaced by the Remastered HD videos: `Data\MOVIES_RA.MEG` (4.2 GB, MEG v2 archive, ~147 files such as `ALLY1.BK2`, `ALLIESMAP_01.BK2`). The `.BK2` files start with `KB2j` = **Bink 2, revision j**. (`Data\MOVIES.MEG` is 44 MB, `MOVIES_TD.MEG`, `MOVIES_BONUS_CD*.MEG` are the other games/bonus material.) The Steam install also ships `bink2w32.dll`/`binkw32.dll` - 32-bit, proprietary (Epic/RAD), unusable from our x64 exe and not to be loaded.

## Options
1. **Own small VQA decoder (not a port of `VQ/VQA32`)** - implement `VQA_*` in `VQASTUB.CPP`'s place: parse the FORM/WVQA chunks (`VQHD`, `FINF`, `VQFR` with `CBF*/CBP*/VPT*/CPL*`, `SND*`), LCW (already in `CODE/LCWUNCMP.CPP`) and IMA-ADPCM (`SOSCODEC.CPP`) exist; the block drawing in `VQ/VQA32/UNVQ*.ASM` must be rewritten in C++. Plays only the two Sizzle movies with the owner's data (small payoff), but would also play briefings if the owner ever supplies classic `MOVIES*.MIX`/VQA files (e.g. from another legally owned copy). Do NOT port the old multi-threaded engine (`TASK.CPP`, `DSTREAM.CPP`, DOS timers).
2. **Bink 2 playback of the Remastered `.BK2` files** - this is what actually exists in the owner's data. No usable decoder is known yet:
   - FFmpeg decodes Bink 1 only; Bink 2 decoder patches by Paul B Mahol (2019, 2020, resubmitted 2022, https://patchwork.ffmpeg.org/project/ffmpeg/patch/20220524133650.18299-1-onemda@gmail.com/) were not merged as far as a web search could tell (trac ticket #4976 "BK2 format support" open). **Re-check current FFmpeg master/changelog before dismissing**; the search was not exhaustive.
   - **NihAV** (Kostya Shishkov, Rust, **AGPL**) is named by the bink2enc README as the open-source decoder it validates against - the most promising lead (found by the owner). Not evaluated by us yet: Bink 2 revision j support, audio, license compatibility with this GPLv3 repo, how to link Rust into the CMake/MSVC build or run it out of process (decode to raw frames + audio and feed our presenter).
   - https://github.com/SBerda/bink2enc (owner's link): **encoder only** (KB2j, Rust, AGPL-3.0, no audio, one commit, CLI without a C API). Useful only as a format reference, not as a decoder.
   - Official RAD Bink SDK: proprietary, license needed - out of scope.
   - Open questions for any Bink route: extracting single files from the 4 GB MEG (MEG v2 header: magic `FFFFFFFF 3F7D70A4`, name table + file table; trivial to read), A/V sync and audio output through `WIN32LIB/AUDIO`, where to hook (`Play_Movie` in `CODE/CONQUER.CPP` ~2931 and `CODE/INTRO.CPP`, which call `VQA_*` - a replacement would sit behind the same API or replace `Play_Movie`), and the owner's "own data only / no third-party binaries" rule (a source dependency built from source is different from a downloaded binary, but ask before adding one).
3. **Leave movies skipped** (current state). The game is playable; briefings fall back to text.

## Recommendation / order
Soundeffects first (owner reports almost all effects missing - a bug in our port, affects every minute of play), then decide between 1 and 2. If the goal is "briefings and cutscenes as shipped with the Remastered data", only option 2 delivers that.

## References
- Call sites: `CODE/CONQUER.CPP` (Play_Movie), `CODE/INTRO.CPP`, `CODE/INIT.CPP:1350`, `CODE/VQASTUB.CPP`.
- Old player sources: `VQ/VQA32/` (`VQAPLAY.H` API, `LOADER.CPP`, `DRAWER.CPP`, `UNVQ*.ASM`, `AUDIO.CPP`).
- Format notes: [../knowledge_base/file_formats.md](../knowledge_base/file_formats.md).
