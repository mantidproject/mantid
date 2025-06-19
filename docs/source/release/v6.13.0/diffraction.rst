===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- New algorithm :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` (VULCAN only) which is meant to replicate the functionality of :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>`, but performs all the work on the events directly from file.
- Several improvements to :ref:`algm-PEARLTransfit`:

  - Perform an intial fit with fixed ``GaussianFWHM`` for calibration runs
  - Accounted for covariance in error calculation for effective temperature
  - Added new input parameter ``RebinInEnergy`` - if False then the energy bins will be determined from the TOF bins in the input workspace and not the ``Ediv`` parameter.
- A new method named ``getFittingParameter`` has been added to Mantid::Geometry::Component class and exposed to python to access fitting parameters from components in instrument definition file
- Enable user defined sample and container geometry together with the definition of gauge volume to account for the beam size. Implementation made in :ref:`SNSPowderReduction <algm-SNSPowderReduction>` and ``mantid.utils.absorptioncorrutils``.

Bugfixes
############
- Correct binning of output workspace of :ref:`algm-PDFFourierTransform-v2` and correct use of bin edge and bin center in evaluated integral.
- Handle the special case where runs to merge do not share common columns in :ref:`HB2AReduce <algm-HB2AReduce>`.
- Several bugs fixed in :ref:`algm-PEARLTransfit`:

  - Fixed bug in binning which produced empty bins that caused fits to Ta10 resonance to fail with default ``Ediv``
  - Corrected error propagation for effective and sample temperature


Engineering Diffraction
-----------------------

New features
############
- In fitting tab of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` data can now be loaded that is in d-spacing as well as Time-of-flight.
- New verison of :ref:`algm-PoldiAutoCorrelation-v6` that supports the new detector geometry on POLDI post upgrade (in December 2024). For older POLDI data please use the previous version (:ref:`algm-PoldiAutoCorrelation-v5` by calling the algorithm with keyword argument Version=5).
- Helper functions in `poldi_utils` to load POLDI data post detector upgrade (currently ASCII format with no meta-data) and simulate the spectra in a :ref:`Workspace2D <Workspace2D>` from an input powder spectrum. The functions can be imported in a script like so ``from plugins.algorithms.poldi_utils import *``.
- Renamed ``Crop Calibration`` within the ``Engineering Diffraction`` calibration interface to ``Set Calibration Region of Interest`` to more accurately reflect its functionality.
- Renamed ``Custom CalFile`` to ``Custom Grouping File`` and allowed the provided file to be ``.xml`` as well as ``.`cal``. This brings it inline with the current detector grouping IO algorithms, :ref:`algm-SaveDetectorsGrouping-v1` and :ref:`algm-LoadDetectorsGroupingFile-v1`.
- Changed the naming suffix for custom file example_group.xml from _Custom to _Custom_example_group so they don't get overwritten when custom grouping is changed (also more clear to the user what grouping is being used).
- Changed the naming suffix for cropped spectrum list: example_list from _Cropped to _Cropped_example_list so they don't get overwritten when custom grouping is changed (also more clear to the user what grouping is being used).
- Added a warning to Focusing for when the vanadium normalisation has been loaded from the ADS
- `#39139 <https://github.com/mantidproject/mantid/issues/39139>`_ : Type driven refactor of Engineering Diffraction Interface ``GSAS-II`` model and centralisation of how ``GSAS-II`` call is configured. Refactor removes the need for hard-coded paths in favour of recursively searching for files within a user defined path.
- New algorithm :ref:`algm-CreatePoleFigureTableWorkspace` which creates a table with the information required to produce a pole figure (a collection of alphas, betas, and intensities), for use in texture analysis.

Bugfixes
############
- `#38668 <https://github.com/mantidproject/mantid/issues/38668>`_ : Disable ``Rietveld`` from :ref:`GSAS-II UI <ui engineering gsas>` Refinement Method combobox options. Add on-hover tooltip to inform users that Rietveld is not currently supported.
- When Focusing, either within the interface or in a script, you should no longer be able to unknowingly apply an outdated vanadium correction. Previously, when focusing had already been run on a user defined region of interest (Custom or Cropped), the vanadium correction was calculated and saved in the ADS as ``engggui_curves_Custom`` or ``engggui_curves_Cropped``. If this ROI was then updated and recalibrated, when focus was run again, it would load the existing ``engggui_curves`` from the ADS which would be from the old ROI. Now, the naming of these files should be more unique to the specific ROI, and in the case where a file is loaded from the ADS which may be wrong, a warning is supplied to the user.
- `#39148 <https://github.com/mantidproject/mantid/issues/39148>`_ : Resolve formatting issue within ``CEO2.cif`` causing a loop error when trying to load into Mantid Workbech using ``LoadCIF``.
- Add <side-by-side-view-location> elements to the detector banks in ``SNAP_Definition.xml``
- Within  :ref:`algm-AbsorptionCorrection` algorithm, when ``Rasterize`` is called, it now takes both the Integration Volume Shape and the Sample Shape to calculate L1 paths. Before, it would only take the integration volume and would assume that the paths within this shape are equal to the paths within the sample.
- `#38882 <https://github.com/mantidproject/mantid/issues/38882>`_ : Fix issue with ``GSAS-II GSASIIscriptable.py`` hard-coded path which is invalid for newer version of GSAS-II (versions 5758 and later).
- Fixed a flaky crash in :ref:`Fitting tab <ui engineering fitting>` of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` seen when deleting multiple workspaces in the ADS. This also fixed an issue of clearing the whole plot in the same tab when deleting workspaces in the ADS.


Single Crystal Diffraction
--------------------------

New features
############
- Added ``detectorbin`` peak shape for the peaks integrated with :ref:`IntegratePeaks1DProfile <algm-IntegratePeaks1DProfile>` integration algorithm.
- By accessing the detectorbin peak shape, users can now view the detector IDs and the corresponding range in the X dimension associated with each detector for each successfully integrated peak from the algorithm.

Bugfixes
############
- Fixed issue with :ref:`PredictPeaks <algm-PredictPeaks>` and the ``CalculateGoniometerForCW`` option where the angle range was filtered incorrectly when not using the default goniometer convention.

:ref:`Release 6.13.0 <v6.13.0>`
