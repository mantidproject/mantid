===================
Diffraction Changes
===================

Powder Diffraction
------------------

New features
############
- (`#41373 <https://github.com/mantidproject/mantid/pull/41373>`_) Version 2 of the :ref:`PEARLTransfit <algm-PEARLTransfit-v2>` algorithm has been added, with improvements in input/output property management.
- (`#41404 <https://github.com/mantidproject/mantid/pull/41404>`_) The ``PawleyPattern`` classes have some new getter/setter methods.

  - ``get_profile_param``/``set_profile_param`` and ``get_profile_free_param``/``set_profile_free_param`` functions to support setting profile parameters and fixing free parameters for a given phase through a profile parameter label.
  - ``get_phase_param``/``set_phase_param`` and ``get_phase_free_param``/``set_phase_free_param`` functions to support setting phase parameters and fixing free parameters for a given phase through a phase parameter label.

- (`#40842 <https://github.com/mantidproject/mantid/pull/40842>`_) The algorithms :ref:`algm-FitPeaks` and :ref:`algm-PDCalibration` now take a boolean flag ``RespectFixedPeakParameters`` to determine whether peak-shape parameters tagged as ``<\fixed>`` should remain fixed or be allowed to refine in the calibration fit (previously, this algorithm has operated under the, now default, value of ``False``).
- (`#41021 <https://github.com/mantidproject/mantid/pull/41021>`_) The algorithms :ref:`algm-FitPeaks` and :ref:`algm-PDCalibration` now take a boolean flag ``CopyLastGoodPeakParameters`` to optionally disable (when set to ``False``) inheriting initial peak-shape parameters from the most recently successful peak fit within the same spectrum.
- (`#41035 <https://github.com/mantidproject/mantid/pull/41035>`_) The :ref:`POLARIS ISIS Powder Scripts <create_total_scattering_pdf_polaris-isis-powder-ref>` have a new option in ``create_total_scattering_pdf``. Entering values for the ``stitch_points``, ``overlap_width``, and ``stitch_lims`` parameters will allow the output workspace to be stitched without rebinning when ``merge_banks`` is set.
- (`#41070 <https://github.com/mantidproject/mantid/pull/41070>`_) :ref:`algm-LoadDiffCal` no longer requires the instrument to be resolvable. If the instrument definition file cannot be found or loaded, the grouping and mask workspaces are created directly from the detector IDs stored in the calibration file. This also includes a change to :ref:`algm-AlignAndFocusPowderSlim` to skip the instrument for loading the calibration information.
- (`#41278 <https://github.com/mantidproject/mantid/pull/41278>`_) The new algorithm :ref:`algm-CylinderAbsorptionCW` was added for calculating absorption and multiple scattering corrections for cylindrical samples with constant wavelength and in-plane scattering only.
- (`#41399 <https://github.com/mantidproject/mantid/pull/41399>`_) :ref:`algm-AlignAndFocusPowderFromFiles` has a new ``AllowSlimProcess`` property (default ``True``). When enabled and the inputs are compatible, the algorithm delegates the reduction to :ref:`algm-AlignAndFocusPowderSlim` for significantly faster processing (seconds versus tens of minutes for large datasets). Set ``AllowSlimProcess=False`` to always use the standard reduction path.


Bugfixes
############


Engineering Diffraction
-----------------------

New features
############
- (`#41082 <https://github.com/mantidproject/mantid/pull/41082>`_) The :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` :ref:`GSAS-II tab <ui engineering gsas>` now allows filtering of the region when loading focused data.
- (`#40842 <https://github.com/mantidproject/mantid/pull/40842>`_) The :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` now supports IMAT instrument selection.
- (`#40842 <https://github.com/mantidproject/mantid/pull/40842>`_) The :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` now supports :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` as a default peak option for calibration and fitting.
- (`#41167 <https://github.com/mantidproject/mantid/pull/41167>`_) Fitting ENGINX data now supports default starting parameters for the :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` peak function.
- (`#41243 <https://github.com/mantidproject/mantid/pull/41243>`_) Added a new IDF for the new VULCAN instrument configuration.

Bugfixes
############
- (`#41049 <https://github.com/mantidproject/mantid/pull/41049>`_) The :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` now honors the GSAS-II timeout value from the settings panel.
- (`#41031 <https://github.com/mantidproject/mantid/pull/41031>`_) A bug in the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` where data from pixels that were not defined in the IDF resulted in an exception. Now those events are skipped.
- (`#41242 <https://github.com/mantidproject/mantid/pull/41242>`_) ENGIN-X data collected in Event Mode no longer throws exceptions upon focusing.
- (`#41313 <https://github.com/mantidproject/mantid/pull/41313>`_) For ``TextureUtils.create_pf`` with ``save_ascii = True`` and ``create_combined_output = True`` an error is no longer thrown when spectra contain different sized histograms.


Single Crystal Diffraction
--------------------------

New features
############
  - (`#40901 <https://github.com/mantidproject/mantid/pull/40901>`_) Updated the IDF for MANDI with the latest detector positions calibrated with garnet.
  - (`#41375 <https://github.com/mantidproject/mantid/pull/41375>`_) :ref:`algm-HB3AAdjustSampleNorm` and :ref:`algm-LoadWANDSCD` now support an optional ``OutputGroupingWorkspace`` property that produces a ``GroupingWorkspace`` mapping each ungrouped detector to its pixel group when ``Grouping`` is set to ``2x2`` or ``4x4``.


Bugfixes
############
- (`#41005 <https://github.com/mantidproject/mantid/pull/41005>`_) :ref:`algm-IntegratePeaksMD` now better handles un-indexed peaks in HKL mode; warnings are given and numeric issues with ellipse finding are avoided by falling back to spherical integration.

:ref:`Release 6.16.0 <v6.16.0>`
