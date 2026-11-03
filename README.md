# template-mcu-minimal-STM32G474VET6-XPulse

Minimal STM32G474VET6 starter template for CubeMX + PlatformIO + VS Code.

## Target

- MCU: STM32G474VET6 (LQFP100, 512 KB Flash, 128 KB RAM)
- Board: XPulse / Penguins-G474
- Debug/flash: ST-Link over SWD

## Required tools

- STM32CubeMX
- PlatformIO Core (`pio`)
- VS Code + PlatformIO + Cortex-Debug

## Build

```bash
pio run
```

## Upload

```bash
pio run -t upload
```

## Modify peripherals

Open `template-mcu-minimal-STM32G474VET6-XPulse.ioc` in STM32CubeMX → change clocks/pins/peripherals → Generate Code → run `pio run`.

## Default pins and clock

- PC13: red LED (active low)
- PA13: SWDIO
- PA14: SWCLK
- Default clock: HSI 16 MHz (no PLL)
- Board also provides 24 MHz HSE, which you can enable later in CubeMX
