# Diagnostic v0.7

This build logs Art-Net control changes at FPP's early post-bridge callback and at the final channel callback while a sequence is playing. Use it to diagnose live fader updates.

# FPP Art-Net Prop Control — FPP 10

A native FPP 10 `ChannelDataPlugin` for controlling two RGB pixel prop groups from a lighting console over Art-Net while optionally preserving xLights/FPP sequence patterns.


## v0.6 live-control fix

FPP merges live Art-Net/E1.31/DDP bridge data before the `modifySequenceData()` plugin callback. v0.6 latches the nine active control values at that early stage and then uses the latched values in `modifyChannelData()` immediately before output. This is intended to make RGB, local dimmers and the global master react during an active sequence rather than only after playback stops.

When FPP is idle, the plugin still refreshes the controls directly from the channel buffer so solid-colour desk control continues to work without a sequence.

## Operating behaviour

With **Use sequence as pattern mask** enabled (the default):

- **When an FPP sequence/player is active:** the sequence controls each pixel's intensity/pattern, while Art-Net controls the actual RGB colour.
- **When FPP is idle:** every pixel is treated as 100% pattern intensity, so the selected Art-Net colour fills the entire prop even with no sequence running.
- The prop's local brightness is applied after colour/pattern processing.
- Art-Net slot 1 **Master** is always applied last across both props.

The sequence pattern intensity is calculated as `max(R,G,B)` for each source pixel. This makes any fully saturated sequence colour (red, green, blue, white, etc.) equivalent to 100% intensity, while preserving fades, chases, twinkles and black pixels. The original sequence hue is intentionally discarded.

If **Use sequence as pattern mask** is disabled, both props are always solid Art-Net-selected colours regardless of sequence playback.

## Processing order

For each pixel:

1. Determine pattern intensity: `max(sequence R, G, B)` if a sequence/player is active, otherwise `255`.
2. Apply that pattern intensity to the Art-Net-selected R/G/B colour.
3. Apply the prop's local brightness channel.
4. Apply the global Master channel **last**.

## DMX / Art-Net slot map

| Art-Net slot | Function |
|---:|---|
| 1 | Master brightness — all props |
| 2 | Letters brightness |
| 3 | Letters red |
| 4 | Letters green |
| 5 | Letters blue |
| 6-9 | Spare |
| 10 | Festoon brightness |
| 11 | Festoon red |
| 12 | Festoon green |
| 13 | Festoon blue |

## Confirmed FPP layout

- **Festoon:** FPP channels 1-6000 (2,000 RGB pixels)
- **Letters:** FPP channels 6001-6447 (149 RGB pixels)
- **Control block:** FPP channel 10001 onward

With control block start = 10001:

- Art-Net slot 1 -> FPP 10001
- Art-Net slot 2 -> FPP 10002
- ...
- Art-Net slot 13 -> FPP 10013

## Examples

### No sequence running

With Master and prop brightness at 255:

- Letters R=255, G=0, B=0 -> all 149 Letters pixels solid red
- Festoon R=0, G=0, B=255 -> all 2,000 Festoon pixels solid blue

### Sequence running

If xLights has a chase where only selected Festoon pixels are lit, and Festoon RGB is set to red at the desk, the output is the same chase pattern in red. Change the desk RGB to blue and the same chase immediately becomes blue.

If xLights fades a pixel from 255 to 128 to 0, the Art-Net-selected colour follows the same 100% -> 50% -> 0% fade.

## FPP Channel Input

Create an Art-Net Channel Input for your chosen universe and map its first slot to FPP channel **10001**. At least the first 13 slots are required. Do not overlap the control block with the prop channel ranges.

## Safety / bypass

The plugin installs with **Bypass ON**. Verify your channel ranges and Art-Net input before turning bypass off.

## Art-Net loss behaviour

FPP's bridge input expires when Art-Net stops arriving. This test version reads the merged FPP control channels, so if those reserved channels otherwise contain zero, loss of Art-Net causes the controls to fall to zero and the props black out. This is the current safe failure behaviour.

## Output to Baldrick8

Send FPP channels 1-6447 to the Baldrick8, preferably using DDP. The plugin modifies FPP's channel buffer before the output stage; the Baldrick8 does not need to know about the Art-Net control channels.

## Build / update on FPP 10

For an existing Git checkout:

```bash
cd /home/fpp/media/plugins/fpp-artnet-prop-control
git pull
./scripts/fpp_install.sh
sudo systemctl restart fppd
```

The plugin builds against the FPP 10 headers and `libfpp` on the device.

## Files

- `src/ArtNetPropControl.cpp` — real-time pattern/colour/dimmer processing
- `settings.json` — FPP settings definitions
- `plugin_setup.php` — settings page
- `menu.inc` — FPP menu entry
- `Makefile` — native plugin build
- `scripts/fpp_install.sh` — install-time build
- `callbacks.sh` — FPP native plugin discovery/lifecycle

## Status

Custom **v0.5 / FPP 10 test build** for `lindsayrobinson/fpp-artnet-prop-control`.
