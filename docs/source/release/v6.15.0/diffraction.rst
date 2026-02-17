===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- (`#40659 <https://github.com/mantidproject/mantid/pull/40659>`_) The :ref:`algm-PDFFourierTransform` algorithm has a new minimum range limit, ``Rmin`` for ``Direction="Forward"``  and ``Qmin`` for ``Direction="Backward"``.
- (`#40042 <https://github.com/mantidproject/mantid/pull/40042>`_) HFIR MIDAS instrument definition has been added.
- (`#40035 <https://github.com/mantidproject/mantid/pull/40035>`_) :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` now outputs multiple workspaces, one for each target, when using the ``SplitterWorkspace`` property. The ``SplitterTarget`` property has been removed.
- (`#40095 <https://github.com/mantidproject/mantid/pull/40095>`_) The ``PawleyPattern1D`` class now supports Pawley refinement of TOF workspaces and has new TOF peak profile class ``BackToBackGauss`` for a back-to-back exponential convoluted with a Gaussian.
- (`#40095 <https://github.com/mantidproject/mantid/pull/40095>`_) The ``PawleyPattern1D`` class now supports zero shift and scale factor for reflection d-spacings (equivalent to modifying TZERO and DIFC).
- (`#40478 <https://github.com/mantidproject/mantid/pull/40478>`_) :ref:`algm-StitchByBackground` has been added, which allows for multiple workspaces with different bin widths to be stitched together.
- (`#40571 <https://github.com/mantidproject/mantid/pull/40571>`_) :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` can now accept a :mod:`mantid.dataobjects.GroupingWorkspace` (via the ``GroupingWorkspace`` property) to define which detectors go to which output spectra.
- (`#40643 <https://github.com/mantidproject/mantid/pull/40643>`_) :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` is no longer hard-coded for the VULCAN instrument.

Bugfixes
############
- (`#40201 <https://github.com/mantidproject/mantid/pull/40201>`_) :ref:`SNSPowderReduction <algm-SNSPowderReduction>` no longer crashes due to the definition of non-standard sample and container geometries.
- (`#40639 <https://github.com/mantidproject/mantid/pull/40639>`_) :ref:`polaris.create_total_scattering_pdf <create_total_scattering_pdf_polaris-isis-powder-ref>` has a new option :ref:`force_high_q_to_1 <enforce_high_q_to_1_polaris_isis-powder-diffraction-ref>` to work around a normalisation bug.


Engineering Diffraction
-----------------------

New features
############
- (`#40668 <https://github.com/mantidproject/mantid/pull/40668>`_) The :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` can now perform Vanadium normalisation during calibration.
- (`#39922 <https://github.com/mantidproject/mantid/pull/39922>`_) The :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` has two new tabs, ``Absorption Correction`` and ``Texture``.

  - The new ``Absorption Correction`` tab corrects workspaces for the beam attenuation through the sample.
  - The new ``Texture`` tab combines data from focused workspaces and peak fitting and plots pole figures for experimental data.
  - Basic usage of these tabs as part of a texture analysis reduction pipeline has been documented in the :ref:`texture technique documentation <Texture_Reduction>`.

- (`#39931 <https://github.com/mantidproject/mantid/pull/39931>`_) The automated peak fitting routine ``Engineering.texture.TextureUtils.fit_all_peaks`` has been updated to improve robustness, particularly in terms of fitting peak positions.
- (`#40326 <https://github.com/mantidproject/mantid/pull/40326>`_) :ref:`algm-CreatePoleFigureTableWorkspace` has two new input properties. ``SpectraWorkspace`` is the name for a new output workspace which contains spectra corresponding to rows in the standard ``OutputWorkspace`` table. ``IncludeSpectrumInfo`` is a flag which adds information columns to the final ``OutputWorkspace`` about where each spectra has come from.
- (`#40326 <https://github.com/mantidproject/mantid/pull/40326>`_) The ``Engineering.texture.texture_helper`` module has been created to hold stand alone texture functionality which is independent of specific instrument workflows or workflow classes:

  - ``show_texture_sample_shape`` shows the sample present on a workspace along with the texture sample directions, and optionally a gauge volume.
  - ``load_all_orientations`` takes a sequence of workspaces and apply sample orientations, loaded from an input file (as described in the :ref:`orientation file section <OrientationSection>` of the :ref:`Texture Analysis Concept doc <TextureAnalysis>`).
  - ``create_pole_figure_tables`` creates a single pole figure table for a sequence of workspaces and their fit parameters.
  - ``plot_pole_figure`` creates a pole figure plot for a pole figure table.
  - Pole figure plots created by ``create_pole_figure_tables`` and ``plot_pole_figure`` have flags to add interactive annotation of points in the pole figure (``include_spec_info`` and ``display_debug_info`` respectively). You can also access this information bt holding the "a" key on the keyboard clicking.

- (`#40441 <https://github.com/mantidproject/mantid/pull/40441>`_) :ref:`algm-EnggEstimateFocussedBackground` now uses multi-threading to speed up execution time by factor ~2-3.
- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) Added calibrated IDF for POLDI (PSI) post upgrade in December 2025.
- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) New loader ``load_poldi_h5f`` for h5f file produced by POLDI post instrument upgrade available in ``plugins.algorithms.poldi_utils``.
- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) New features in :ref:`algm-PoldiAutoCorrelation-v6`:

  - Added parameter ``NGroups``  to split POLDI detectors into groups and produce an autocorrelation spectrum per grouping.

  .. image:: ../../images/6_15_release/poldi-ngroups.png
     :class: screenshot
     :width: 500px

  - Detector IDs are now associated with autocorrelation spectra in the output workspace.

- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) New features in the ``PawleyPattern1D`` class from ``Engineering.pawley_utils``:

  - ``fit_no_constraints`` method to perform 'free' fit - i.e. do not constrain peak centres to predicted positions.
  - Improvements to the robustness of background estimation.
  - ``fit_background`` method to fit background function to 1D data using a cost function that is biased to be less sensitive to positive outliers.

- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) Added ``PawleyPattern2DNoConstraints`` class in ``Engineering.pawley_utils`` to do a 'free' 2D refinement of POLDI data

Bugfixes
############
- (`#40367 <https://github.com/mantidproject/mantid/pull/40367>`_) Updated the ``Focus`` tab processing in the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` to remove zeros outside the data limits.
- (`#40833 <https://github.com/mantidproject/mantid/pull/40833>`_) A bug in :ref:`create_total_scattering_pdf <create_total_scattering_pdf_polaris-isis-powder-ref>` where loading non-consecutive runs was causing an error has been fixed.


Single Crystal Diffraction
--------------------------

New features
############
- (`#40042 <https://github.com/mantidproject/mantid/pull/40042>`_) Added HFIR IMAGINE instrument image plate instrument definition.
- (`#40187 <https://github.com/mantidproject/mantid/pull/40187>`_) Added new instrument IMAGINE-X.

Bugfixes
############
- (`#40291 <https://github.com/mantidproject/mantid/pull/40291>`_) :ref:`algm-ConvertQtoHKLMDHisto` is now faster by removing an unnecessary copy.
- (`#40422 <https://github.com/mantidproject/mantid/pull/40422>`_) Verified and corrected the IMAGINE-X detector layout using neutron absorbing masks.

:ref:`Release 6.15.0 <v6.15.0>`
