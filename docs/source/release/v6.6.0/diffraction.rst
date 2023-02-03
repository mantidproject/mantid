===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- Introduce an input parameter in :ref:`WANDPowderReduction <algm-WANDPowderReduction>` to specify that the input workspaces are from event filtering. In such a situation, the overall reduction time could be reduced significantly.
- Added support for ILL D4C instrument, a liquid diffractometer. This adds a new algorithm responsible for the reduction stage: :ref:`D4ILLReduction <algm-D4ILLReduction>`, which handles calling the loader, correcting for dead time, bank position offsets, relative efficiency, and normalisation to monitor or time.
- Increase the speed of the fourier filter in the POLARIS total scattering reduction by reducing the Rmax parameter used in the pair of (forward and backward) PDFFourierTransform calls and optimising the integration code inside :ref:`PDFFourierTransform v2 <algm-PDFFourierTransform-v2>`

Bugfixes
############
- Change to use time average mean value of logs in :ref:`PDDetermineCharacterizations <algm-PDDetermineCharacterizations>`
- Fix problem when calling focus twice in a reduction using the ISIS Powder diffraction scripts. The second run of focus was using a partially processed input file from the first focus resulting in some of the reduction steps (normalisation, absorption correction) were being run twice
- Fixed an issue with :ref:`HB2AReduce <algm-HB2AReduce>` when 'colltrans' column is not available in the data file. This corresponds to the situation when the 'colltrans' motor is not connected.


Engineering Diffraction
-----------------------

New features
############
- On the GSASII tab, when a successful refinement is run, the output table workspace for the lattice parameters now includes the Microstrain value and the column title is marked when this parameter has been refined. A similar instrument parameter table workspace is now also generated which includes Sigma-1 and Gamma(Y), with a refined flag, and the fitting range. The Pawley reflections are output to a separate table workspace if they are available.
- An indeterminate progress bar has been added below the FitPropertyBrowser on the Engineering Diffraction Fitting tab. This will display when the fit is in progress and whether the recent fit status was success or fail.

Bugfixes
############
- Correct the tabbing order between widgets on the Engineering Diffraction interface.
- Add a scrollbar to the GSASII tab to allow a smaller interface height, like the fitting tab.
- Check that there is enough data points before attempting to fit a diffraction peak in :ref:`FitPeaks <algm-FitPeaks>`
- SaveVulcanGSS algorithm marked as deprecated as of 2022-11-30


Single Crystal Diffraction
--------------------------

New features
############
* Added new option to :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`: to get initial TOF window from back-to-back exponential coefficients if specified in the instrument parameters.xml
* Added back-to-back exponential coefficients to SXD parameters.xml file
* Simplified input arguments to :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`: the parameter `FractionalTOFWindow` has been removed, if a user wants to integrate with an initial window dTOF/TOF = constant for all peaks then this can be achieved by setting ThetaWidth = 0 - note this is technically a breaking change.
* New algorithm :ref:`SaveINS <algm-SaveINS>` to write a .ins input file for SHELX
* Added two options to :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`: to scale angular resolution parameter (``ThetaWidth``) by wavelength, and to specify minimum number of TOF bins in a valid peak (``NTOFBinsMin``).
* Improve estimation of resolution parameters in :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>` (more robust to outliers).
* Made Lorentz correction in :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>` optional.
* Added back bank 14 to TOPAZ for a total of 20 banks
* Exposed indexing tolerance for :ref:`SCDCalibratePanels  <algm-SCDCalibratePanels>` when using `RecalculateUB` option.
* Parallelize loop over peaks in IntegratePEaksMD which provides significant speed-up for ellipsoid integration.

Bugfixes
############
- :ref:`IntegratePeaksProfileFitting <algm-IntegratePeaksProfileFitting>` library `ICCFitTools` module updated to support more recent versions of SciPy where the factorial function moved from `scipy.misc.factorial` to `scipy.special.factorial`. On newer versions of SciPy, the algorithm previously failed.
- Replaced deprecated NumPy `np.int` type in :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ>`.
- :ref:`SaveReflections <algm-SaveReflections>` now warns if an empty peak table is saved and doesn't fail with an error
- Replaced deprecated Matplotlib `bivariate_normal` function in `BVGFitTools.py`
- Fix issue where :ref:`LoadIsawPeaks <algm-LoadIsawPeaks>` fails when loading peaks with modulation vectors that cannot recover modulated UB matrix.
- Ensuring colorscale max > min (found for weak peaks with pixels having 0 intensity) in :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`.
- :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>` will now close a generated figure if an error occured while trying to write to file (e.g. file of same name was already open)
- Fix bug that didn't set HKL of peak if ``UpdatePeakPosition = True`` in :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`
- Fix bugs in :ref:`SelectCellWithForm <algm-SelectCellWithForm>` and ref:`SelectCellOfType <algm-SelectCellOfType>` that incorrectly transform modulation vectors.
- Fix bug where modulation vectors are not transformed in :ref:`TransformHKL <algm-TransformHKL>`.

:ref:`Release 6.6.0 <v6.6.0>`