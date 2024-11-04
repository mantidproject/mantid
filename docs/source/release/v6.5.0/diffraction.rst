===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- The ISIS powder diffraction scripts support the PaalmanPings correction method, calling :ref:`PaalmanPingsMonteCarloAbsorption <algm-PaalmanPingsMonteCarloAbsorption>`. To access it, set ``empty_can_subtraction_method: 'PaalmanPings'`` in the user config file. Also, there is new optional config settings (with their default values) ``paalman_pings_events_per_point: 1000``.
- It is now possible to exclude specific banks from the total scattering merge banks for Polaris using -1 in the ``q_lims``. For more details see :ref:`Polaris.create_total_scattering_pdf<create_total_scattering_pdf_polaris-isis-powder-ref>`.
- Added Total radial distribution function (:math:`G_k(r)`) option to :ref:`PDFFourierTransform v2 <algm-PDFFourierTransform-v2>`.
- Added a switch to allow an absolute normalization to be performed by the ISIS powder diffraction scripts when running on the POLARIS instrument. As part of this the setting ``crystal_density`` has been removed from the SampleDetails class. The density of the material can now be set by supplying two of ``number_density``, ``number_density_effective`` and ``packing_fraction``.
- Users can now choose to run :ref:`Polaris.create_total_scattering_pdf <create_total_scattering_pdf_polaris-isis-powder-ref>` with either First order or Second order Placzek Corrections.
- Users can now choose to use either First order or Second order Placzek Corrections with :ref:`TotScatCalculateSelfScattering <algm-TotScatCalculateSelfScattering>`.

Bugfixes
############
- Changed the Fourier Filter applied during ISIS total scattering reduction so that :math:`g(r)=0` instead of :math:`g(r)-1=0`.
- The error message raised when attempting to use the :ref:`Polaris.create_total_scattering_pdf <create_total_scattering_pdf_polaris-isis-powder-ref>` algorithm, if the focus has been run with ``do_van_normalisation=false``, has been improved.
- Fixed an issue in the Powder Diffraction Reduction GUI where the GUI would crash when being launched the second time.
- Fixed deprecated syntax in :ref:`LoadWAND <algm-LoadWAND>` that gives h5py warnings.


Engineering Diffraction
-----------------------

New features
############
- A new tab has been added to the :ref:`ISIS Engineering Diffraction UI<Engineering_Diffraction-ref>` to support running refinements in GSAS-II.

.. image:: ../../images/6_5_release/Diffraction/GSASII_tab.png
    :align: center


Single Crystal Diffraction
--------------------------

New features
############
- New TOPAZ IDF with updated calibration and current number of active banks.
- New integration algorithm :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>` to integrate single-crystal Bragg peaks.

Bugfixes
############
- Fixed issue with :ref:`DGSPlanner <dgsplanner-ref>` introduced by diffractometer instrument WAND\ :sup:`2`.
- Fixed logic issues in :ref:`IntegrateEllipsoids <algm-IntegrateEllipsoids>` that prevents the integration of satellite peaks.
- Fixed issue with errorbar in :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDToQ>`.
- Fixed deprecated syntax in  :ref:`LoadWANDSCD <algm-LoadWANDSCD>` that gives h5py warnings.
- Fix bug in :ref:`FindSXPeaks <algm-FindSXPeaks>` which caused a crash for non-finite data (e.g. NaN after dividing intensity in a bin by 0).
- Fix typo in :ref:`HB3AIntegratePeaks <algm-HB3AIntegratePeaks>` and :ref:`HB3AIntegrateDetectorPeaks <algm-HB3AIntegrateDetectorPeaks>` Lorentz correction factors.


:ref:`Release 6.5.0 <v6.5.0>`
