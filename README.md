# FPP Art-Net Prop Control

A small native FPP ChannelDataPlugin for controlling two RGB pixel prop groups from a lighting console over Art-Net while FPP/xLights continues to generate the actual pixel effects.

## What it does

For each prop, processing is performed in this order:

1. Scale the existing Red, Green and Blue components using the prop's RGB controls.
2. Apply the prop's local brightness control.
3. Apply the global Master control **last**.

At 255/255/255 RGB, local brightness 255, and Master 255, the FPP sequence is unchanged.

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

## Default FPP layout

The plugin defaults to the provisional contiguous channel layout discussed during design:

- **Letters:** FPP channels 1-447 (149 RGB pixels)
- **Festoon:** FPP channels 448-6447 (2,000 RGB pixels)
- **Control block:** FPP channel 10001 onward

With control block start = 10001:

- Art-Net slot 1 -> FPP 10001
- Art-Net slot 2 -> FPP 10002
- ...
- Art-Net slot 13 -> FPP 10013

If your xLights/FPP prop start channels differ, simply change them on the plugin settings page.

## FPP Channel Input

In FPP, create an Art-Net Channel Input for your chosen universe and map its first slot to FPP channel **10001** (or whatever you configure as the plugin's control start channel). At least the first 13 slots are required.

Do not overlap the control block with either prop's pixel channels.

## Neutral console values

Set these to 255 for an unmodified show:

- Slot 1 Master = 255
- Slots 2-5 Letters = 255
- Slots 10-13 Festoon = 255

The plugin installs with **Bypass ON**, so verify your ranges and Art-Net input before turning bypass off.

## RGB controls are filters

The RGB controls scale what is already in the sequence. They do not recolour a missing component.

Example: if a pixel is pure blue (R=0, G=0, B=255), raising the Letters/Festoon Red control cannot make it red because the source red value is zero. This is intentional and preserves xLights/FPP effects and colour relationships.

## DMX-loss behaviour in v0.1

FPP's bridge input expires when Art-Net stops arriving. This v0.1 reads the merged FPP channel buffer, so when the bridge values expire the result depends on the underlying values at the reserved control channels. If those channels are otherwise zero (recommended), the controls fall to zero and the props black out.

That is a safe default for this first version. A later version can add an explicit heartbeat/fail-to-full mode if required.

## Output to Baldrick8

Use FPP network output to send the finished pixel data to the Baldrick8, preferably using DDP. The plugin changes FPP's channel buffer before it reaches the output stage; the Baldrick8 does not need to know about the four controls for each prop.

## Build / install

This plugin is designed for FPP's native plugin build process.

1. Put this directory in a Git repository named `fpp-artnet-prop-control`.
2. Replace `YOUR-USERNAME` in `pluginInfo.json` with your GitHub username/organisation if hosting on GitHub.
3. Install it through FPP's Plugin Manager from the repository, or copy/clone it into FPP's plugin directory for development.
4. During installation, `scripts/fpp_install.sh` builds the native shared library using FPP's own make environment.
5. Open **Input/Output Setup -> Art-Net Prop Control**, check the ranges, then disable **Bypass processing**.

If developing via SSH, the typical plugin directory is `/home/fpp/media/plugins/fpp-artnet-prop-control`.

## Files

- `src/ArtNetPropControl.cpp` — real-time pixel processing
- `settings.json` — FPP settings definitions
- `plugin_setup.php` — settings page
- `menu.inc` — FPP menu entry
- `Makefile` — native plugin build
- `scripts/fpp_install.sh` — install-time build
- `callbacks.sh` — enables live plugin lifecycle support

## Status

This is a custom **v0.1 / test build**. The source conditionally supports the FPP 9.5 plugin API and the newer FPP 10 plugin lifecycle API. Test with the Baldrick output brightness/current limited and Bypass available before using it in a live event.
