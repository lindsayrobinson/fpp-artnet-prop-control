<div id="global" class="settings">
<?php
PrintSettingGroup("APCGeneral", "", "", 1, "fpp-artnet-prop-control");
PrintSettingGroup("APCLetters", "", "", 1, "fpp-artnet-prop-control");
PrintSettingGroup("APCFestoon", "", "", 1, "fpp-artnet-prop-control");
?>

<div class="alert alert-info mt-3" role="alert">
  <strong>Art-Net slots:</strong><br>
  1 Master; 2 Letters Brightness; 3 Letters Red; 4 Letters Green; 5 Letters Blue;
  <strong>6 Letters Colour Mode</strong>; 7-9 spare;<br>
  10 Festoon Brightness; 11 Festoon Red; 12 Festoon Green; 13 Festoon Blue;
  <strong>14 Festoon Colour Mode</strong>.
</div>

<div class="alert alert-success" role="alert">
  <strong>Colour modes (independent per prop):</strong><br>
  <strong>0-127:</strong> Full Sequence Colour — keep the sequence RGB and ignore that prop's RGB sliders while playing.<br>
  <strong>128-255:</strong> Desk Colour Override — keep the sequence pattern/intensity but recolour it with the prop's Art-Net RGB sliders.<br>
  When FPP is idle, either mode outputs the desk-selected solid colour. Prop brightness is applied next and Master is always applied last.
</div>

<div class="alert alert-warning" role="alert">
  Use <strong>Bridge Data Priority = Prioritize Bridge</strong> so Art-Net control changes remain live during sequence playback.
  Keep FPP channels 10001-10014 (or your configured control block) separate from the prop pixel ranges.
</div>
</div>
