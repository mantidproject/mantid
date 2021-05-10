===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Powder Diffraction
------------------
New features
############

- New algorithm :ref:`RebinRagged <algm-RebinRagged>` which can rebin a workspace with different binning parameters for each spectrum
- :ref:`PDCalibration <algm-PDCalibration>` now supports workspaces with grouped detectors (i.e. more than one detector per spectrum).
- New diagnostic plotting tool `Calibration.tofpd..diagnostics.plot2d` which adds markers for expected peak positions
- New diagnostic plotting tool `Calibration.tofpd.diagnostics.difc_plot2d` which plots the change in DIFC between two instrument calibrations.
- New diagnostic plotting tool `Calibration.tofpd.diagnostics.plot_peakd` which plots the d-spacing relative strain of peak positions.
- New diagnostic plotting tool `Calibration.tofpd.diagnostics.plot_corr` which plots the Pearson correlation coefficient for time-of-flight and d-spacing for each detector.
- New diagnostic plotting tool `Calibration.tofpd.diagnostics.plot_peak_info` which plots fitted peak parameters for instrument banks.

Improvements
############

- New motor convention for HB2A implemented in :ref:`HB2AReduce <algm-HB2AReduce>`.
- :ref:`PDCalibration <algm-PDCalibration>` now intitialises A,B and S of BackToBackExponential if correpsonding coeficients are in the instrument parameter.xml file.
- Support fitting diffractometer constants with chi-squared cost function in <algm-PDCalibration>.
- Differential evolution minimizer added to :ref:`AlignComponents <algm-AlignComponents>`.
- Differential evolution minimizer added to :ref:`CorelliPowderCalibrationCreate <algm-CorelliPowderCalibrationCreate>`.
- Added option to fix banks' vertical coordinate :ref:`CorelliPowderCalibrationCreate <algm-CorelliPowderCalibrationCreate>`.
- :ref:`AlignComponents <algm-AlignComponents>` has option to output a table listing the changes in position and orientation for each component
- :ref:`CorelliPowderCalibrationCreate <algm-CorelliPowderCalibrationCreate>` now outputs a table listing the changes in position and orientation for each bank

- :ref:`WANDPowderReduction <algm-WANDPowderReduction>` GUI now only shows relevant items in drop down menu and no longer has a confusing copy input workspace name button.

Bugfixes
########

- Fix the issue in saving reduced data as GSAS format using :ref:`HB2AReduce <algm-HB2AReduce>`.
- Fix the format inconsistency (with data saved from autoreduction workflow) issue for saving GSAS data using :ref:`HB2AReduce <algm-HB2AReduce>` - both are now using :ref:`SaveGSSCW <algm-SaveGSSCW>` for saving GSAS data.
- Fix out-of-range bug in :ref:`FitPeaks <algm-FitPeaks>` for histogram data.
- Fix bug in :ref:`FitPeaks <algm-FitPeaks>` not correctly checking right window for an individual peak
- Fix bug to actually implement intended sequential fit of DIFC, DIFA, TZERO in :ref:`PDCalibration <algm-PDCalibration>`.
- New options, including three "cache directory" and one "clean cache" in the Advanced Setup tab of the SNS Powder Reduction interface
- New caching feature is added to :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to speed up calculation using same sample and container.
- New algorithm :ref:`RebinRagged <algm-RebinRagged>` which can rebin a workspace with different binning parameters for each spectrum
- New options, including three "cache directory" and one "clean cache" in the Advanced Setup tab of the SNS Powder Reduction interface
- New caching feature is added to :ref:`SNSPowderReduction <algm-SNSPowderReduction>` to speed up calculation using same sample and container.
- New property `CleanCache` in algorithm :ref:`SNSPowderReduction <algm-SNSPowderReduction>`
- New options "cache directory" and "clean cache" in the Advanced Setup tab of the SNS Powder Reduction interface
- Use caching for Va in :ref:`SNSPowderReduction <algm-SNSPowderReduction>`.
- Improve algorithm :ref:`FitPeaks <algm-FitPeaks>` to enable it to fit with multiple peaks in same spectrum with Back-to-back Exponential function starting from user specified parameters.
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` has additional property, ``DeltaRagged``, which allows using :ref:`RebinRagged <algm-RebinRagged>` to bin each spectrum differently.
- Allow a different number of spectra for absorption correction division of PEARL data. This allows ``create_vanadium`` to work for a non-standard dataset.

- The :ref:`ConvertUnits <algm-ConvertUnits>` algorithm has been extended to use a quadratic relationship between d spacing and TOF when doing conversions between these units. The diffractometer constants DIFA, DIFC and TZERO that determine the form of the quadratic can be loaded into a workspace using a new :ref:`ApplyDiffCal <algm-ApplyDiffCal>` algorithm. This functionality was previously only available in :ref:`AlignDetectors <algm-AlignDetectors>` which only performed the conversion in the direction TOF to d spacing. This change will ensure that the conversion of focussed datasets from d spacing back to TOF at the end of the ISIS powder diffraction data reduction is performed correctly.

Engineering Diffraction
-----------------------
- New IDF for upgraded VULCAN instrument

Single Crystal Diffraction
--------------------------
- New version of algorithm :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` provides more accurate calibration results for CORELLI instrument.
- Modified some logs in output workspace from :ref:`LoadWANDSCD <algm-LoadWANDSCD>` to be TimeSeriesProperty so they work with :ref:`SetGoniometer <algm-SetGoniometer>`.
- :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>` has option to integrate ellipsoids around estimated centroid instead of nominal position.
- :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>` has option to determine ellipsoid covariance iteratively and to use the estimated standard deviation rather than scale the major axis of the ellipsoid to the spherical radius.
- :ref:`ConvertHFIRSCDtoMDE <algm-ConvertHFIRSCDtoMDE>` has new geometrical correction factor `ObliquityParallaxCoefficient` for shift in vertical beam position due to wide beam.
- :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ>` has new geometrical correction factor `ObliquityParallaxCoefficient` for shift in vertical beam position due to wide beam.
- :ref:`TransformHKL <algm-TransformHKL>` has new keyword argument `FindError` allowing the lattice parameter error calculation to be skipped. This can be used to transform HKL of a peaks workspace without enough peaks to do an optimization so they are simply set to zero.

Improvements
############
- :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>` now allows ellipsoidal shapes to be manually defined for the PeakRadius and Background radii options.
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` now check if previous container is created using the same method before reusing it.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` now update attached UB matrix with given lattice constants (optional).
- :ref:`FilterPeaks <algm-FilterPeaks>` now can select banks in addition to filtering by values.
- :ref:`FindPeaksMD <algm-FindPeaksMD>` has been modified to make use of the multiple goniometers add to :ref:`Run <mantid.api.Run>` and `goniometerIndex` add to MDEvents.
- :ref:`HB3APredictPeaks <algm-HB3APredictPeaks>` can now predict satellite peaks for DEMAND data
- :ref:`IntegrateEllipsoids <algm-IntegrateEllipsoids>` calculates intensity for satellite peaks with fractional HKL
- :ref:`MDNorm <algm-MDNorm>` algorithm can now efficiently process background.
- method ``IPeaksWorkspaceaddPeak(V3D, SpecialCoordinateSystem)`` exposed to the python interface.

Bugfixes
########
- Correctly format FullProf files in :ref:`SaveReflections <algm-SaveReflections>` - there is now a title line in the header, the multiplicity is by default 1 and there are two rows per modulation vector.
- :ref:`SaveReflections <algm-SaveReflections>` now determines the parent HKL of a satellite correctly, previously the satellite HKL was rounded.
- Fixed changes in :ref:`ConvertQtoHKLMDHisto <algm-ConvertQtoHKLMDHisto>` to accomodate zoom in slice viewer, as a result this algorithm will now drop the connection to the original workspace

Instrument Updates
##################

- Added new detector to MANDI instrument geomety with updated calibration. Valid-to dates changed in previous files ``MANDI_Definition_2020_04_01.xml`` and ``MANDI_Parameters_2020_04_01.xml``. Valid-from dates changed in newly added files ``MANDI_Definition_2021_02_01.xml`` and ``MANDI_Parameters_2021_02_01.xml``.

Known Defects
#############

Bugfixes
########
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` no longer returns null calibration outputs.
- Fix failure in :ref:`HB3AFindPeaks <algm-HB3AFindPeaks>` when switching to crystallographic convention.
- Make :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ>` awear of k convention.

LeanElasticPeak
###############

A new Peak concept has been create, a LeanElasticPeak where the
instrument is not included as part of Peak. The only requirement for
this peak is a Q-sample vector. There are a number of modifications
made to facilitate this. The new LeanElasticPeak and
LeanElasticPeakWorkspace concept is show in detail at
:ref:`LeanElasticPeaksWorkspace <LeanElasticPeaksWorkspace>`

- :ref:`CreatePeaksWorkspace <algm-CreatePeaksWorkspace>` has been modified to optionally create a :ref:`LeanElasticPeaksWorkspace <LeanElasticPeaksWorkspace>`.
- :ref:`FindPeaksMD <algm-FindPeaksMD>` has been modified to optionally create a :ref:`LeanElasticPeaksWorkspace <LeanElasticPeaksWorkspace>`.
- :ref:`PredictPeaks <algm-PredictPeaks>` has been modified to optionally create a :ref:`LeanElasticPeaksWorkspace <LeanElasticPeaksWorkspace>`.
- :ref:`PredictSatellitePeaks <algm-PredictSatellitePeaks>` will work with :ref:`LeanElasticPeaksWorkspace <LeanElasticPeaksWorkspace>`.
- New algorithm :ref:`HFIRCalculateGoniometer <algm-HFIRCalculateGoniometer>` allows the goniometer to be found for constant wavelength peaks after creation, works with :ref:`LeanElasticPeaksWorkspace <LeanElasticPeaksWorkspace>`.
- These following other algorithms have either been made to work or confirmed to already work with the LeanElasticPeak:

   - :ref:`algm-AddPeakHKL`
   - :ref:`algm-CalculatePeaksHKL`
   - :ref:`algm-CalculateUMatrix`
   - :ref:`algm-CentroidPeaksMD`
   - :ref:`algm-CompareWorkspaces`
   - :ref:`algm-CombinePeaksWorkspaces`
   - :ref:`algm-FilterPeaks`
   - :ref:`algm-FindUBUsingFFT`
   - :ref:`algm-FindUBUsingIndexedPeaks`
   - :ref:`algm-FindUBUsingLatticeParameters`
   - :ref:`algm-FindUBUsingMinMaxD`
   - :ref:`algm-IndexPeaks`
   - :ref:`algm-IntegratePeaksMD`
   - :ref:`algm-LoadNexusProcessed`
   - :ref:`algm-OptimizeLatticeForCellType`
   - :ref:`algm-SaveNexusProcessed`
   - :ref:`algm-SaveHKLCW`
   - :ref:`algm-SelectCellOfType`
   - :ref:`algm-SelectCellWithForm`
   - :ref:`algm-SortPeaksWorkspace`
   - :ref:`algm-ShowPossibleCells`
   - :ref:`algm-TransformHKL`

:ref:`Release 6.1.0 <v6.1.0>`
