=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- The ALFView interface executes any algorithms on a background thread to provide a smoother user experience.
- The ALFView interface is disabled when running an algorithm, and re-enabled when it has finished. The title of the ALFView window is changed to indicate why the interface is disabled.
- New loader has been added to handle Lagrange NeXus files: LoadILLLagrange.
- :ref:`LagrangeILLReduction <algm-LagrangeILLReduction>` can now accept NeXus data as input for both sample and container runs.
- New menu item for Direct interfaces, Shiver is added.

Bugfixes
############
- The peak picker tool is now offset on the y axis by the background amount.
- The peak picker tool is now set as activate by default.
- The settings loaded into the ALFView interface are now separate from those loaded into the InstrumentView.
- Removing the sample on the ALFView interface will now clear the InstrumentWidget plot.
- The 'Reset View' button on the 'Render' tab of ALFView will no longer prevent the 'Pick' tab tools from working.
- :ref:`LagrangeILLReduction <algm-LagrangeILLReduction>` can now process ASCII data even if they contain only one scan step.


CrystalField
-------------

New features
############
- Allow specifying the range for the x values in getSpectrum in the Crystal Field Python interface
- Added a function to print eigenvectors for a CrystalField object in human readable form.

Bugfixes
############



MSlice
------

New features
############


Bugfixes
############
- Fixed GDOS intensity correction so that the correction is applied in the same way regardless of rotation of the slice plot.
- Fix for error when saving a slice plot as a matlab file.
- Warning now omitted if a cut is taken with a higher resolution than the parent slice. This causes the appearance of missing lines between adjacent datapoints.
- When taking a cut, if a intensity range is provided it is now applied to the plot y limits.
- When changing intensity of a slice plot the axis limits now get reset to their original values. This solves a bug where zooming out after an intensity change was not possible.
- Bragg peaks are now sized more appropriately on an interactive cut.
- MSlice now correctly preserves metadata when saving NXPSE files.
- Fixed a bug relating to intensity correction on some datasets where arrays used to transform data during correction had incorrect dimensions.
- Fixed a bug causing a crash when cancelling an intensity correction from the temperature input pop-up.
- When inputting waterfall limits an error is no longer caused by the invalid use of E to apply an exponent.
- You can now save slices and cuts in an ASCII format from the MSlice command line.


DNS_Reduction
-------------

New features
############


Bugfixes
############
- Two separate "Sample Data" and "Standard Data" buttons are added to the Powder TOF mode.
- Standard data are now loaded by default.
- "Read All" button is removed.
- Functionality of the red asterisk next to the "Data Directory" edit box is improved.


:ref:`Release 6.7.0 <v6.7.0>`
