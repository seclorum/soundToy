# OSDK Oric-1 / Atmos Conventions

## Target
- Oric-1 and Oric Atmos (48K / 64K overlay possible)
- OSDK toolchain (C + 6502 Assembly)
- Default load address usually $600 (see osdk_config.bat → OSDKADDR)

## Languages
- Prefer pure 6502 Assembly for performance-critical code (graphics, sound, IRQ, scrolling).
- Use C for higher-level logic, game state, menus.
- Never mix C and Assembly in the same source file.
- Assembly files use `.s` extension (OSDK style). Use `.text`, `.data`, `.bss`, `.zero` segments.
- C files use standard OSDK C (no modern C99/C11 features that the old compiler rejects).

## Memory map awareness
- Zero page ($00–$FF) is precious – document every ZP variable.
- Stack is fixed at $0100–$01FF.
- TEXT screen: $BB80
- HIRES screen: $A000–$BFDF
- Standard charset: $B500 (or $9800 in some maps)
- Avoid writing into ROM ($C000–$FFFF) unless overlay RAM is enabled.
- Prefer absolute addressing only when necessary; use zeropage addressing when possible.

## Coding style
- Assembly labels: lowercase with underscores (`player_x`, `irq_handler`).
- Global C symbols that are called from Assembly must be declared `extern` and use the OSDK naming.
- Always keep `osdk_config.bat` (or Makefile) in sync when adding/removing source files (OSDKFILE).
- Prefer self-modifying code only when the gain is clear and the code is well commented.
- Comment all hardware register accesses (VIA $0300, AY-3-8912, Microdisc $0314, etc.).

## Build
- The user will manually build - indicate to them when they must do it in another terminal.
- Never commit `.tap` / `.dsk` / intermediate `.o65` files.
