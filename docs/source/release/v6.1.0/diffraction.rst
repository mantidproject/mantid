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

Improvements
############

- :ref:`PDCalibration <algm-PDCalibration>` now intitialises A,B and S of BackToBackExponential if correpsonding coeficients are in the instrument parameter.xml file.
- Support fitting diffractometer constants with chi-squared cost function in <algm-PDCalibration>.

Bugfixes
########

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
- Correct unit to TOF for ``_tof_xye`` files output for PEARL, when the focusing mode is set to *all*.
- Use caching for Va in :ref:`SNSPowderReduction <algm-SNSPowderReduction>`.
- Improve algorithm :ref:`FitPeaks <algm-FitPeaks>` to enable it to fit with multiple peaks in same spectrum with Back-to-back Exponential function starting from user specified parameters.
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` has additional property, ``DeltaRagged``, which allows using :ref:`RebinRagged <algm-RebinRagged>` to bin each spectrum differently.
- Allow a different number of spectra for absorption correction division of PEARL data. This allows ``create_vanadium`` to work for a non-standard dataset.


Engineering Diffraction
-----------------------
- New IDF for upgraded VULCAN instrument

Improvements
############

- BackToBackExponential fitting parameters read from .xml file and output to .prm file for GSAS-II.

Single Crystal Diffraction
--------------------------
- New version of algorithm :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` provides more accurate calibration results for CORELLI instrument.
- Modified some logs in output workspace from :ref:`LoadWANDSCD <algm-LoadWANDSCD>` to be TimeSeriesProperty so they work with :ref:`SetGoniometer <algm-SetGoniometer>`.
- :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>` has option to integrate ellipsoids around estimated centroid instead of nominal position.
- :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>` has option to determine ellipsoid covariance iteratively and to use the estimated standard deviation rather than scale the major axis of the ellipsoid to the spherical radius.

Improvements
############
- :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>` now allows ellipsoidal shapes to be manually defined for the PeakRadius and Background radii options.
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` now check if previous container is created using the same method before reusing it.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` now update attached UB matrix with given lattice constants (optional).
- :ref:`FilterPeaks <algm-FilterPeaks>` now can select banks in addition to filtering by values.
- :ref:`FindPeaksMD <algm-FindPeaksMD>` has been modified to make use of the multiple goniometers add to :ref:`Run <mantid.api.Run>` and `goniometerIndex` add to MDEvents.
- :ref:`IntegrateEllipsoids <algm-IntegrateEllipsoids>` calculates intensity for satellite peaks with fractional HKL

Instrument Updates
##################

- Added new detector to MANDI instrument geomety with updated calibration. Valid-to dates changed in previous files ``MANDI_Definition_2020_04_01.xml`` and ``MANDI_Parameters_2020_04_01.xml``. Valid-from dates changed in newly added files ``MANDI_Definition_2021_02_01.xml`` and ``MANDI_Parameters_2021_02_01.xml``.

Known Defects
#############
- When using new ellipsoidal peak integration capability in :ref:`IntegratePeaksMD <algm-IntegratePeaksMD>`, some peak intensities are returned as zero. When using the default spherical integration, the same behavior is not observed.

:ref:`Release 6.1.0 <v6.1.0>`
