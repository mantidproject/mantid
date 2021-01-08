=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New Features
############
- Added the ability to save a detector grouping to a XML file from a Custom Grouping String on Indirect Data Reduction. 

Improvements
############
- Added an option to load diffraction data on IN16B.
- Indirect Data Analysis multiple input tabs can now accept different fit ranges for each spectra.
- :ref:`QENSFitSequential <algm-QENSFitSequential>` now accepts multiple inputs for `StartX` and `EndX` for each spectra.
- Plotted fits in indirect data analysis are cleared when the fit parameters are changed.
- The context menu for plots in the  indirect data analysis GUI is now enabled.
- The function browser in the ConvFit tab should now correctly show Q values for Q dependent functions.
- The ConvFit tab in the Indirect Data Analysis GUI will now set default parameters for the FWHM when a resolution file is loaded.
- QENSFitSequential and PlotPeakByLogValue can accept a different mask range for each spectra via the `ExcludeMultiple` Parameter.
- Tabs in the Indirect Corrections interface not has a scroll bar to prevent being squashed on small screens.

Bug Fixes
#########
- A bug has been fixed in Indirect data analysis on the Elwin tab that causes the integration range to be reset when changing between plotted workspaces.
- A bug in Indirect Data Analysis causing logs not to load in the Edit Local Fit Parameters dialog has been fixed.
- Fixed a bug in Indirect Data Reduction causing the colon separator in Custom Grouping to act like a dash separator. This colon separator should now act
  as expected (i.e. `1:5` means the same as `1,2,3,4,5`).
- Fixed a crash on Indirect Bayes ResNorm when clicking `Plot Current Preview` without a workspace loaded.
- A bug has been fixed that caused fitting tabs in data analysis to use the X mask of the first spectra for every single spectra.

:ref:`Release 6.0.0 <v6.0.0>`
