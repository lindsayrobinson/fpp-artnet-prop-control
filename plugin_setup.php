<div id="global" class="settings">
<?php
PrintSettingGroup("APCGeneral", "", "", 1, "fpp-artnet-prop-control");
PrintSettingGroup("APCLetters", "", "", 1, "fpp-artnet-prop-control");
PrintSettingGroup("APCFestoon", "", "", 1, "fpp-artnet-prop-control");
?>

<div class="alert alert-info mt-3" role="alert">
  <strong>Art-Net slots:</strong>
  1 Master; 2 Letters Brightness; 3 Letters Red; 4 Letters Green; 5 Letters Blue;
  6-9 spare; 10 Festoon Brightness; 11 Festoon Red; 12 Festoon Green; 13 Festoon Blue.
</div>

<div class="alert alert-success" role="alert">
  <strong>Sequence pattern mode:</strong> when enabled and FPP is playing, the sequence supplies only
  each pixel's intensity/pattern. Art-Net supplies the actual colour. When FPP is idle, the whole prop
  displays the Art-Net-selected colour. Prop brightness is applied next and Master is always applied last.
</div>

<div class="alert alert-warning" role="alert">
  Keep FPP channels 10001-10013 (or your configured control block) separate from the prop pixel ranges.
  A sequence's original hue is intentionally discarded; only its per-pixel brightness pattern is retained.
</div>
</div>
