.. _v7.0.1:

===========================
Mantid 7.0.1 Release Notes
===========================

This is a patch release that corrects some significant issues since :ref:`version 7.0.0 <v7.0.0>`.

The changes are:
- Fixed the UI elements for Mac users while :ref:`fitting models to data <01_fitting_models_to_data>` using the :ref:`UserFunction <func-UserFunction>` and :ref:`TabulatedFunction <func-TabulatedFunction>`. `#42606 <https://github.com/mantidproject/mantid/pull/42606>`_
- Remove workaround for qsettings location on nfs home area `#42716 <https://github.com/mantidproject/mantid/pull/42716>`_
- Fix bug creating ReflectometryISISLoadAndProcess dialog `#42646 <https://github.com/mantidproject/mantid/pull/42646/>`_
- Fixed the parameter line edits collapsing on macOS in the :ref:`QENS Fitting <interface-inelastic-qens-fitting>` interface, which made the parameter values difficult to edit `#42735 <https://github.com/mantidproject/mantid/pull/42735>`_
- :class:`~mantid.api.MatrixWorkspace` in Python has a new method ``getXMinMax``, which returns the lowest and highest X values of all spectra or of the given workspace indices. Spectra with a non-finite first or last value are skipped unless ``includeNonFiniteSpectra`` is true, in which case they contribute their finite values. `#42675 <https://github.com/mantidproject/mantid/pull/42675>`_
- Fixed a large temporary rise in memory use, and the long pause that came with it, in the new Instrument View for workspaces whose spectra do not all share the same bin edges, such as data in d-spacing. It occurred when changing the projection, masking detectors, or opening the view. `#42675 <https://github.com/mantidproject/mantid/pull/42675>`_
- The legacy ISIS SANS reduction no longer supports the ``UNWRAP`` user file command, following the removal of UnwrapMonitorsInTOF. ``UNWRAP ON`` is now ignored with a warning. `#42540 <https://github.com/mantidproject/mantid/pull/42540>`_
- Removed UnwrapMonitorsInTOF and UnwrapSNS (deprecated since release 6.13.0). `#42540 <https://github.com/mantidproject/mantid/pull/42540>`_

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 7.0.1: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
  `doi: 10.5286/Software/Mantid7.0.1 <https://doi.org/10.5286/Software/Mantid7.0.1>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments
  and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166
  `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_
  (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)

Changes in this version
-----------------------

* `41744 <https://github.com/mantidproject/mantid/pull/41744>`_ Fix empty Instrument View in Windows standalone
* `41732 <https://github.com/mantidproject/mantid/pull/41732>`_ Fix crash in `TextureUtils` for RaggedWorkspaces

.. _download page: http://download.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v7.0.1
