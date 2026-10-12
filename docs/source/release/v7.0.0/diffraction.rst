===================
Diffraction Changes
===================

Powder Diffraction
------------------

New features
############
- (`#41280 <https://github.com/mantidproject/mantid/pull/41280>`_) POLARIS now provides sensible starting parameters for :ref:`Back2BackExponential <func-BackToBackExponential>`
- (`#41734 <https://github.com/mantidproject/mantid/pull/41734>`_) :ref:`algm-AlignAndFocusPowderSlim` now accepts a ``CalibrationWorkspace`` and a ``MaskWorkspace``. When supplied, these take precedence over the corresponding information in ``CalFileName``. :ref:`algm-AlignAndFocusPowderFromFiles` has been updated to pass these parameters to :ref:`algm-AlignAndFocusPowderSlim`.
- (`#42228 <https://github.com/mantidproject/mantid/pull/42228>`_) Add new IDF for ISIS GEM instrument, including Bank 5X, updating geometry from GEM_225_DAE3_detector.dat tables and fix inversions of B1 modules.

Bugfixes
############
- (`#41672 <https://github.com/mantidproject/mantid/pull/41672>`_) :ref:`ISIS Powder SampleDetails <isis-powder-diffraction-sampleDetails-ref>` now accepts a ``number_density_units`` argument which can be set appropriately to fix issues where total scattering ``S(Q)-1`` does not tend to 0 at high Q when number density has been provided as: formula units per volume (see :ref:`number_density_unit_sampleDetails_isis-powder-diffraction-ref`).
- (`#41855 <https://github.com/mantidproject/mantid/pull/41855>`_) Fixed :ref:`CylinderAbsorptionCW <algm-CylinderAbsorptionCW>` Sabine calculation for large μR
- (`#42080 <https://github.com/mantidproject/mantid/pull/42080>`_) :ref:`CylinderAbsorptionCW <algm-CylinderAbsorptionCW>` now scales the ``AttenuationXSection`` property from its tabulated 1.7982 Å value to the requested ``Wavelength``. Previously cross-sections provided through the properties were used unscaled, so they gave a different result to the same cross-sections set on the sample material with :ref:`algm-SetSampleMaterial` at any wavelength other than 1.7982 Å.
- (`#42095 <https://github.com/mantidproject/mantid/pull/42095>`_) Fixed :ref:`algm-HFIRPowderReduction` producing incorrect results when SampleBackgroundFilename was supplied. The sample background was subtracted without being normalised to monitor or time, and ``SampleBackgroundScaleFactor`` was ignored.
- (`#42110 <https://github.com/mantidproject/mantid/pull/42110>`_) Fixed the binning of :ref:`algm-HFIRPowderReduction`. The bins are now placed on a grid of ``XBinWidth`` anchored at ``XMin``, or at zero when ``XMin`` is not given, so bin centres always fall on ``XMin + XBinWidth * (n + 1/2)``. Previously a ``XMin`` (or ``XMax``) outside the range of the data was replaced by the extreme value found in the data, which shifted every bin and made the bins slightly wider than ``XBinWidth``. ``XMin`` and ``XMax`` are now both optional and take a single number rather than a list of per-spectrum values, and only the bins that hold data are output.

Removed
############
- (`#41728 <https://github.com/mantidproject/mantid/pull/41728>`_) The obsolete properties ``UnwrapRef``, ``LowResRef`` and ``LowResSpectrumOffset`` have been removed from :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>`, :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` and :ref:`SNSPowderReduction <algm-SNSPowderReduction>`.


Engineering Diffraction
-----------------------

New features
############
- (`#40961 <https://github.com/mantidproject/mantid/pull/40961>`_) New :ref:`Texture Planning User Interface <Texture_Planner-ref>` for aiding the user in planning and executing Texture Analysis experiments. See the linked docs for more information.
- (`#41114 <https://github.com/mantidproject/mantid/pull/41114>`_) Pawley refinement classes in ``Engineering.pawley_utils`` can now output the fitted peak parameters (intensity, centre and FWHM, with errors where available) as a table workspace per phase.
- (`#41114 <https://github.com/mantidproject/mantid/pull/41114>`_) Pawley refinement classes in ``Engineering.pawley_utils`` can now have bounds set on their fit parameters, as a fraction of the current value, as a multiplicative factor, or as explicit limits (e.g. to force non-negative peak intensities).
- (`#41114 <https://github.com/mantidproject/mantid/pull/41114>`_) The 2D Pawley refinement in ``Engineering.pawley_utils`` now offers an alternating fit strategy (selected with ``PawleyFitStrategy``), which re-estimates the per-spectrum scale factors and backgrounds between successive refinements rather than applying a single overall scale.
- (`#41114 <https://github.com/mantidproject/mantid/pull/41114>`_) The 2D Pawley refinement in ``Engineering.pawley_utils`` can now optionally apply a Lorentz correction to the simulated pattern.
- (`#41114 <https://github.com/mantidproject/mantid/pull/41114>`_) The ``Phase`` class in ``Engineering.pawley_utils`` can now crop its hkl list to the reflections accessible in a given workspace (from the two-theta coverage of that workspace and the wavelength limits), for use when several virtual detectors each cover a subset of the pixels.
- (`#41114 <https://github.com/mantidproject/mantid/pull/41114>`_) POLDI 2D data simulated with ``poldi_utils.simulate_2d_data`` now accounts for the wavelength dependence of the incident flux.
- (`#41709 <https://github.com/mantidproject/mantid/pull/41709>`_) In :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` the :ref:`GSASII tab <ui engineering gsas>` now supports CIF file selection from a list of defaults distributed with Mantid.
- (`#41819 <https://github.com/mantidproject/mantid/pull/41819>`_) In ``Engineering.EnggUtils``, focusing using the ``focus_run`` method will now adjust the calibration DIFCs per detector pixel to account for an offset in the scattering volume centre-of-mass (due to sample partially illuminated within the gauge volume). This is only done when both a Sample Shape and Gauge Volume are present on the Workspace.
- (`#41935 <https://github.com/mantidproject/mantid/pull/41935>`_) In :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` the settings now supports saving experiment specific settings, based on RB Number. Previous behaviour was that every time a setting was changed it would overwrite the previous saved version of the setting, where as now, it will overwrite the previous version saved for that RB Number. This allows users to return to experiments and find have specific settings have been remembered.
- (`#42146 <https://github.com/mantidproject/mantid/pull/42146>`_) The :ref:`Texture Planning User Interface <Texture_Planner-ref>` now has a guided tutorial. It appears the first time the interface is opened and can be re-run at any time from the mortarboard button on the bottom toolbar. The tutorial drives a separate, temporary copy of the interface, so it never affects the session you are working in.

Bugfixes
############
- (`#41114 <https://github.com/mantidproject/mantid/pull/41114>`_) The POLDI instrument definition has been corrected so that all detector pixels face the sample position — previously half of each detector block was rotated about the wrong axis and the other half was not rotated at all. The IDF schema now also allows a rotation axis to be given on a ``<locations>`` element.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`Run Processing tab <ui engineering run_processing>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when loading an existing calibration from a ``.prm`` file, the interface now records the vanadium run, so the calibration reports itself as valid immediately instead of only once the Focus tab has been used.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`Run Processing tab <ui engineering run_processing>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when setting a calibration region of interest on ENGIN-X, the options are now labelled ``Texture20`` and ``Texture30`` consistently, so they no longer appear as ``Texture (20 spec)`` and ``Texture (30 spec)`` until the instrument is changed.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`Run Processing tab <ui engineering run_processing>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when plotting the output of a calibration, the fitted peak centre is now read from the peak function in use, so plotting no longer fails with a ``KeyError`` if the default peak function has been changed from ``BackToBackExponential``.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`Absorption Correction tab <ui engineering correction>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when a preset gauge volume shape is selected, the custom gauge volume file finder is now hidden, so it is only offered for a custom shape.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`Absorption Correction tab <ui engineering correction>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when ``No Gauge Volume`` is selected, any gauge volume already defined on a run is now cleared, so the correction no longer silently continues to use the previous one.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`Absorption Correction tab <ui engineering correction>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when a run is loaded from a processed NeXus file with no sample material, the table now reports the material as unset, so a blank material is no longer mistaken for one that has been defined.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`Fitting tab <ui engineering fitting>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when a sequential fit stops because the change in the fitted values fell below tolerance, the fit is now treated as converged, so it is no longer reported as a failure and its result is carried over to seed the next workspace. This is the normal outcome for a fit started from an already-optimal set of parameters, which is every run after the first.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`GSAS II tab <ui engineering gsas>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when a refinement is rejected by validation or fails, its working directory is now removed, so an empty ``tmp_EngDiff_GSASII_*`` directory is no longer left behind in the save location on every attempt. Two refinements started within the same second no longer collide.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`GSAS II tab <ui engineering gsas>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when a refinement is started with no project name, it is now rejected with an error, so it no longer proceeds and fails later.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) In the :ref:`GSAS II tab <ui engineering gsas>` of the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`, when the range markers are moved or the toolbar's home button is pressed, the plot window now keeps the title naming the refined file, so it no longer reverts to ``GSAS-II Plot``.


Single Crystal Diffraction
--------------------------

New features
############
- (`#41275 <https://github.com/mantidproject/mantid/pull/41275>`_) On WISH the :ref:`Back2BackExponential <func-BackToBackExponential>` starting parameter values have been updated post upgrade, late 2025
- (`#41726 <https://github.com/mantidproject/mantid/pull/41726>`_) :ref:`algm-HB3AAdjustSampleNorm` can now output an unnormalized Q-sample event workspace and a separate normalization workspace for use with :ref:`algm-MDNorm`.
- (`#41777 <https://github.com/mantidproject/mantid/pull/41777>`_) :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` has a new ``WavelengthFromUB`` option to derive each peak's wavelength from Bragg's law using the UB matrix and its integer HKL instead of from the measured TOF. This is intended for quasi-Laue workflows, where a peak's TOF-derived wavelength can be unreliable, but it also works for standard time-of-flight Laue data.
- (`#41809 <https://github.com/mantidproject/mantid/pull/41809>`_) :ref:`MDNorm <algm-MDNorm>` can now normalize monochromatic single crystal diffraction data (e.g. WAND, DEMAND) using a pre-computed normalization workspace, via the new ``MonoSCDNormalizationWorkspace`` property.
- (`#41960 <https://github.com/mantidproject/mantid/pull/41960>`_) Added the :ref:`FindUBFromConventionalCell <algm-FindUBFromConventionalCell>` algorithm, which determines a UB matrix (crystal orientation) from unindexed peaks and the known conventional-cell lattice parameters and centering.
- (`#42122 <https://github.com/mantidproject/mantid/pull/42122>`_) New algorithm :ref:`algm-IntegratePeaksShapeMD` integrates single crystal Bragg peaks by reusing the ellipsoidal peak shape already stored on each peak (e.g. derived from a model of the instrument's resolution), instead of fitting a new shape from the events around each peak.

Bugfixes
############
- (`#41646 <https://github.com/mantidproject/mantid/pull/41646>`_) Fixed the ``Counts`` vanadium normalization option in :ref:`algm-LoadWANDSCD` so detector counts are scaled by the mean vanadium signal when applying the vanadium correction.
- (`#42023 <https://github.com/mantidproject/mantid/pull/42023>`_) :ref:`algm-SaveIsawDetCal` and :ref:`algm-SaveIsawPeaks` now record the calibrated positions of CORELLI detector panels. Since Mantid 6.15.0.1 they wrote the nominal positions taken from the instrument definition, so a panel calibration was silently absent from the ``.DetCal`` and ``.peaks`` files they produced. :ref:`algm-SaveIsawPeaks` also wrote nominal panel orientations and sizes, which affected WISH as well as CORELLI. CORELLI and WISH files written with an affected version should be regenerated.
- (`#42023 <https://github.com/mantidproject/mantid/pull/42023>`_) :ref:`algm-SaveHKL` and :ref:`algm-AnvredCorrection` now use the calibrated detector distance in the slant-path absorption correction for CORELLI, having used the nominal distance since Mantid 6.15.0.1.

:ref:`Release 7.0.0 <v7.0.0>`
