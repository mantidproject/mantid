===========
Sliceviewer
===========

- The Sliceviewer interface has been neatened up to reduce wasted space and devote more to the data view itself.

- Sliceviewer now contains an interactive table of plot details that updates as you move your cursor over the data (functionality from the SpectrumViewer in Mantidplot). This table includes the dimension value and signal for multidimensional workspaces, and for matrix workspaces with a spectrum axis:
   - Signal
   - Spectra Number
   - Detector ID
   - Two Theta
   - Azimuthal Angle
   - Time of Flight
   - Wavelength
   - Energy
   - d-Spacing
   - Mod Q

- On opening, the zoom tool is selected by default.
- Reversed colourmaps are now accessed with a reverse checkbox.
- The default colourmap can be set in File > Settings.
- The scale (lin/log) remains set when reopened.

- Sub-plots in the sliceviewer now follow the scaling on the colorbar.
- When line plots are active on the sliceviewer the arrow keys can now be used to move the cursor a pixel at a time.
- The line width of the line plots has been reduced.

- Figure options have been removed as most options were not appropriate.