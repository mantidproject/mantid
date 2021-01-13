=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
############
- A Custom Grouping String in Indirect Data Reduction can now be saved to an XML file.

Improvements
############
- Diffraction data can now be loaded on IN16B.
- Indirect Data Analysis multiple input tabs can now accept different fit ranges for each spectra.
- :ref:`QENSFitSequential <algm-QENSFitSequential>` can now accept different inputs for `StartX` and `EndX` for each
  spectra.
- Plotted fits in indirect data analysis are now cleared when the fit parameters are changed.
- The context menu for plots in the indirect data analysis GUI is now enabled.
- The function browser in the ConvFit tab will now correctly show Q values for Q dependent functions.
- The ConvFit tab in the Indirect Data Analysis GUI will now set default parameters for the FWHM when a resolution file
  is loaded.
- QENSFitSequential and PlotPeakByLogValue can now accept a different mask range for each spectra via the
  `ExcludeMultiple` Parameter.
- Tabs in the Indirect Corrections interface now have a scroll bar to prevent being squashed on small screens.

Bug Fixes
#########
- A bug has been fixed in Indirect data analysis on the Elwin tab that causes the integration range to be reset when
  changing between plotted workspaces.
- A bug has been fixed in Indirect Data Analysis that caused logs not to load in the Edit Local Fit Parameters dialog.
- A bug has been fixed in Indirect Data Reduction that caused the colon separator in Custom Grouping to act like a dash
  separator. This colon separator should now act as expected (i.e. `1:5` means the same as `1,2,3,4,5`).
- A bug has been fixed in Indirect Bayes ResNorm that caused a crash when clicking `Plot Current Preview` without a
  workspace loaded.
- A bug has been fixed in Indirect Data Analysis that caused fitting tabs to use the X mask of the first spectra for
  every single spectra.

:ref:`Release 6.0.0 <v6.0.0>`
