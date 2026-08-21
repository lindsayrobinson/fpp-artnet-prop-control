# FPP Art-Net Prop Control — FPP 10

Native FPP 10 `ChannelDataPlugin` for controlling two RGB pixel prop groups from a lighting console over Art-Net while optionally preserving full xLights/FPP sequence colour independently for each prop.

## v0.8 — independent colour modes

Each prop now has its own live Art-Net colour-mode channel:

- **Letters mode — Art-Net slot 6**
- **Festoon mode — Art-Net slot 14**

While a sequence is playing:

- **0-127 = Full Sequence Colour** — preserve the sequence's original RGB values; that prop's Art-Net RGB sliders are ignored. The prop brightness and global Master still apply.
- **128-255 = Desk Colour Override** — preserve the sequence's per-pixel intensity/pattern using `max(R,G,B)`, but recolour the effect with the prop's Art-Net RGB sliders.

When no sequence is running, either mode outputs the desk-selected solid RGB colour across the prop, so the console can light the props directly.

The per-prop brightness is always applied after colour selection, and Art-Net slot 1 **Master** is always applied last across both props.

## Art-Net slot map

| Art-Net slot | Function |
|---:|---|
| 1 | Master brightness — all props |
| 2 | Letters brightness |
| 3 | Letters red |
| 4 | Letters green |
| 5 | Letters blue |
| **6** | **Letters colour mode: 0-127 sequence / 128-255 desk colour** |
| 7-9 | Spare |
| 10 | Festoon brightness |
| 11 | Festoon red |
| 12 | Festoon green |
| 13 | Festoon blue |
| **14** | **Festoon colour mode: 0-127 sequence / 128-255 desk colour** |

## Confirmed FPP layout

- **Festoon:** FPP channels 1-6000 (2,000 RGB pixels)
- **Letters:** FPP channels 6001-6447 (149 RGB pixels)
- **Control block:** FPP channel 10001 onward

With control block start = 10001:

- Art-Net slot 1 -> FPP 10001
- Letters mode slot 6 -> FPP 10006
- Festoon mode slot 14 -> FPP 10014

Configure the Art-Net Channel Input for at least **14 slots**.

## Behaviour examples

### Letters = full sequence colour, Festoon = desk colour

- Ch 6 = 0
- Ch 14 = 255

Letters plays its original xLights rainbow/colour effects. Festoon keeps the xLights chase/twinkle/fade pattern but uses Ch 11/12/13 for its live colour.

### Both = full sequence colour

- Ch 6 = 0
- Ch 14 = 0

Both props use their original sequence colours. Ch 2 and Ch 10 still dim their respective props; Ch 1 remains the global master.

### Both = desk colour override

- Ch 6 = 255
- Ch 14 = 255

Both props preserve sequence patterns/intensity but are recoloured live from their Art-Net RGB sliders.

### No sequence running

Colour mode is ignored because there is no sequence source. Each prop displays its desk-selected solid RGB colour, scaled by its local dimmer and the Master.

## FPP setting required for live controls during playback

Set:

**Settings -> Input/Output -> Bridge Data Priority -> Prioritize Bridge**

The pixel sequence occupies FPP channels 1-6447 while the Art-Net control block begins at 10001, so the control channels do not overlap the sequence data.

## Safety / bypass

The plugin installs with **Bypass ON**. Verify your channel ranges and Art-Net input before turning bypass off.

## Output to Baldrick8

Send FPP channels 1-6447 to the Baldrick8, preferably using DDP. The Art-Net control channels do not need to be sent to the Baldrick8.

## Build / update on FPP 10

```bash
cd /home/fpp/media/plugins/fpp-artnet-prop-control
git pull
./scripts/fpp_install.sh
sudo systemctl restart fppd
```

## Files

- `src/ArtNetPropControl.cpp` — real-time colour-mode, pattern and dimmer processing
- `settings.json` — FPP settings definitions
- `plugin_setup.php` — settings page
- `menu.inc` — FPP menu entry
- `Makefile` — native plugin build
- `scripts/fpp_install.sh` — install-time build
- `callbacks.sh` — FPP native plugin discovery/lifecycle

## Status

Custom **v0.8 / FPP 10** build for `lindsayrobinson/fpp-artnet-prop-control`.
