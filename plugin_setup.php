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
  Set all active controls to 255 for a neutral/unmodified result.
</div>

<div class="alert alert-warning" role="alert">
  Keep the FPP control channel range separate from the pixel channel ranges. RGB controls are filters:
  they scale colours already present in the FPP sequence; they do not create a missing colour component.
</div>
</div>
