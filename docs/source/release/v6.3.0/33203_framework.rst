SliceViewer
-----------

Bugfixes
########
- When entering a specific value for the center of the slicepoint of an integrated dimension/axis it will no longer jump to the nearest bin-center (this fix also affects MDEvent workspaces as it was assumed each dimension had 100 bins for the purpose of updating the slider for a integrated dimension/axis).
- Slicepoint center now set to correct initial value (consistent with position of slider) for MDHisto workspaces.

:ref:`Release 6.3.0 <v6.3.0>`
