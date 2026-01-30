===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- (`#40659 <https://github.com/mantidproject/mantid/pull/40659>`_) Implement minimum range limit for :ref:`algm-PDFFourierTransform` algorithm, e.g. ``Rmin`` for ``Direction="Forward"``  and ``Qmin`` for ``Direction="Backward"``.
- (`#40042 <https://github.com/mantidproject/mantid/pull/40042>`_) HFIR MIDAS instrument definition added
- (`#40035 <https://github.com/mantidproject/mantid/pull/40035>`_) Now when using the spliiter workspace with :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` it will output multiple workspaces, one for each target. The ``SplitterTarget`` property has been removed.
- (`#40095 <https://github.com/mantidproject/mantid/pull/40095>`_) Support Pawley refinement of TOF workspaces in ``PawleyPattern1D`` class and added TOF peak profile ``BackToBackGauss`` for back-to-back exponential convoluted with a Gaussian.
- (`#40095 <https://github.com/mantidproject/mantid/pull/40095>`_) Support zero shift and scale factor for reflection d-spacings in ``PawleyPattern1D`` class (equivalent to modifying TZERO and DIFC).
- (`#40478 <https://github.com/mantidproject/mantid/pull/40478>`_) :ref:`algm-StitchByBackground` has been added, which allows for multiple workspaces with different bin widths to be
  stitched together.
- (`#40571 <https://github.com/mantidproject/mantid/pull/40571>`_) A GroupingWorkspace can now be provided to :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` to define what detectors go to which output spectra.
- (`#40493 <https://github.com/mantidproject/mantid/pull/40493>`_) :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` is no longer hard-coded for the VULCAN instrument.

Bugfixes
############
- (`#40073 <https://github.com/mantidproject/mantid/pull/40073>`_) Disabled StripVanadiumPeaks in ref:`SNSPowderReduction <algm-SNSPowderReduction>`'s test, "SNSPowderRedux.PG3Analysis" because it was causing stochastic test failures on ornl runners and contributed little to the test's purpose.
- (`#40201 <https://github.com/mantidproject/mantid/pull/40201>`_) Solve the crash issue in ref:`SNSPowderReduction <algm-SNSPowderReduction>` regarding the definition of non-standard sample and container geometries for the absorption correction purpose.
- (`#40639 <https://github.com/mantidproject/mantid/pull/40639>`_) Add ``force_high_q_to_1`` option to POLARIS ``create_total_scattering_pdf`` to work around a `normalisation bug <https://github.com/mantidproject/mantid/issues/40566>`__


Engineering Diffraction
-----------------------

New features
############
- (`#40668 <https://github.com/mantidproject/mantid/pull/40668>`_) Vanadium normalisation can now be performed during calibration in the :ref:`Engineering_Diffraction-ref` interface.
- (`#39922 <https://github.com/mantidproject/mantid/pull/39922>`_) Update to the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>`, adding two tabs, ``Absorption Correction`` and ``Texture``.
- (`#39922 <https://github.com/mantidproject/mantid/pull/39922>`_) The new ``Absorption Correction`` tab allows the user to correct workspaces for the beam attenuation through the sample.
- (`#39922 <https://github.com/mantidproject/mantid/pull/39922>`_) The new ``Texture`` tab allows the user to combine data from focused workspaces and peak fitting, to plot pole figures for their experimental data
- (`#39922 <https://github.com/mantidproject/mantid/pull/39922>`_) Basic usage of these tabs as part of a texture analysis reduction pipeline has been documented in the Texture Technique Documentation
- (`#39931 <https://github.com/mantidproject/mantid/pull/39931>`_) The automated peak fitting routine ``fit_all_peaks`` in ``Engineering.texture.TextureUtils`` has been updated to improve robustness, particularly in terms of fitting peak positions.
- (`#40326 <https://github.com/mantidproject/mantid/pull/40326>`_) In :ref:`algm-CreatePoleFigureTableWorkspace`, two new options have been added to the inputs. ``SpectraWorkspace`` allows the user to provide a name for a new output workspace which contains spectra corresponding to rows in the standard ``OutputWorkspace`` table. ``IncludeSpectrumInfo`` is a flag which adds information columns to the final ``OutputWorkspace`` about where each spectra has come from.
- (`#40326 <https://github.com/mantidproject/mantid/pull/40326>`_) ``texture_helper.py`` has been created to hold stand alone texture functionality which is independent of specific instrument workflows or workflow classes.
- (`#40326 <https://github.com/mantidproject/mantid/pull/40326>`_) ``show_texture_sample_shape`` in ``texture_helper.py`` is a new standalone function which allows the user to show the sample present on a workspace along with the texture sample directions, and optionally a gauge volume.
- (`#40326 <https://github.com/mantidproject/mantid/pull/40326>`_) ``load_all_orientations`` in ``texture_helper.py`` is a new standalone function which allows the user to take a sequence of workspaces and apply sample orientations, loaded from an input file (as described in the :ref:`orientation file section <OrientationSection>` of the :ref:`Texture Analysis Concept doc <TextureAnalysis>`)
- (`#40326 <https://github.com/mantidproject/mantid/pull/40326>`_) ``create_pole_figure_tables`` in ``texture_helper.py`` is a new standalone function which allows the user to directly create a single pole figure table for a sequence of workspaces and their fit parameters.
- (`#40326 <https://github.com/mantidproject/mantid/pull/40326>`_) ``plot_pole_figure`` in ``texture_helper.py`` is a new standalone function which allows the user to directly create a pole figure plot for a pole figure table.
- (`#40326 <https://github.com/mantidproject/mantid/pull/40326>`_) Pole Figure plots can now be created from ``texture_helper`` functions with a debug flag which allows interactive annotation of points in the pole figure by holding "a" key on the keyboard and clicking.
- (`#40441 <https://github.com/mantidproject/mantid/pull/40441>`_) Use multi-threading in :ref:`algm-EnggEstimateFocussedBackground` algorithm to speed up execution time by factor ~2-3
- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) Added calibrated IDF for POLDI (PSI) post upgrade in December 2025
- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) New loader ``load_poldi_h5f`` for h5f file produced by POLDI post instrument upgrade in ``plugins.algorithms.poldi_utils``
- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) New features in :ref:`algm-PoldiAutoCorrelation-v6`

  - Add parameter ``NGroups```  to split POLDI detectors into groups and produce a autocorrelation spectrum per grouping
  - Associate detector IDs with autocorrelation spectra in output workspace

- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) New features in ``PawleyPattern1D`` class in ``Engineering.pawley_utils``

  - Method to perform 'free' fit - i.e. do not constrain peak centres to predicted positions
  - Improvements to robustness of background estimation
  - Method to fit background function to 1D data using a cost function that is biased to be less sensitive to positive outliers.

- (`#40599 <https://github.com/mantidproject/mantid/pull/40599>`_) Added class ``PawleyPattern2DNoConstraints`` in ``Engineering.pawley_utils`` to do a 'free' 2D refinement of POLDI data

Bugfixes
############
- (`#40367 <https://github.com/mantidproject/mantid/pull/40367>`_) Updated the ``Focus`` tab processing in the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` to use :ref:`CropWorkspaceRagged <algm-CropWorkspaceRagged>` in order to remove zeros outside the data limits.


Single Crystal Diffraction
--------------------------

New features
############
- (`#40042 <https://github.com/mantidproject/mantid/pull/40042>`_) HFIR IMAGINE instrument image plate instrument definition detector add to instruments
- (`#40187 <https://github.com/mantidproject/mantid/pull/40187>`_) Added new instrument IMAGINE-X

Bugfixes
############
- (`#40291 <https://github.com/mantidproject/mantid/pull/40291>`_) ConvertQtoHKLMDHisto is faster by removing unnecessary copy
- (`#40422 <https://github.com/mantidproject/mantid/pull/40422>`_) Verified and corrected IMAGINE-X detector layout using neutron absorbing masks.

:ref:`Release 6.15.0 <v6.15.0>`
