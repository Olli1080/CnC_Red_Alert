# Knowledge Base — Index

Cross-cutting, emerging knowledge about this codebase that isn't tied to one specific module.

- **[build_toolchains.md](build_toolchains.md)** — The full landscape of coexisting build systems (Watcom, Borland/Turbo C++, Codewright, MSVC6) across the repo, which one is authoritative per module, and the environment variables the makefiles expect.
- **[dependencies_and_missing_libs.md](dependencies_and_missing_libs.md)** — Third-party SDKs/libraries the repo depends on but doesn't vendor (DirectX 5/Media SDKs, GCL, HMI SOS), plus which pieces of some of these *are* partially vendored (headers, prebuilt `.LIB`s).
- **[file_formats.md](file_formats.md)** — Westwood's proprietary on-disk formats implemented in this codebase: `.MIX` archives, `.AUD` audio, `.WSA` animation, `.VQA`/`VQ` video, LCW/LZO/LZW/Blowfish/CRC "Straw & Pipe" compression filters, `.INI`-based rules/scenario data.
- **[codebase_history_and_cruft.md](codebase_history_and_cruft.md)** — The repo's layered history: multiple generations of the same subsystem coexisting (DOS→Win32 library ports, three generations of online-multiplayer backend, Borland→Watcom toolchain migration), and where dead/backup/superseded code lives so it isn't mistaken for active code.
