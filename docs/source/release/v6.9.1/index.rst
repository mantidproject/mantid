.. _v6.9.1:

==========================
Mantid 6.9.1 Release Notes
==========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 6.9.0 <v6.9.0>`.

The changes are:

 - A fix to stop workbench freezing when plotting data from a workspace which is changing (e.g plotting live data)
 - Removed slit lookup that was specific to OFFSPEC in :ref:`algm-ReflectometryReductionOneLiveData` as it is no longer required and was causing regular crashes when running live data on OFFSPEC.
 - Fixed a bug in :ref:`Elwin Tab <elwin>` of :ref:`Data Manipulation Interface <interface-inelastic-data-processor>` where changing integration range with the sliders did not change default integration range.
 - Add sample log values to the live data workspace before the instrument is loaded in :ref:`algm-ReflectometryReductionOneLiveData` to ensure log values are available when setting the detector positions.
 - Fixed a crash when using multiple Indirect or Inelastic interfaces. This crash was present on the :ref:`Bayes Fitting <interface-inelastic-bayes-fitting>` interface, but could also be replicated elsewhere.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 6.9.1: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
  `doi: 10.5286/Software/Mantid6.9.1 <http://dx.doi.org/10.5286/Software/Mantid6.9.1>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments
  and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166
  `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_
  (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)

Changes in this version
-----------------------

* `37052 <https://github.com/mantidproject/mantid/pull/37052>`_ Replace draw with draw_idle for some functions
* `37053 <https://github.com/mantidproject/mantid/pull/37053>`_ Load instrument after loading sample logs in ReflectometryReductionOneLiveData
* `37016 <https://github.com/mantidproject/mantid/pull/37016>`_ Fix sliders not changing integration limits in Elwin tab
* `36935 <https://github.com/mantidproject/mantid/pull/36935>`_ Fix regular live data crashes on OFFSPEC
* `37074 <https://github.com/mantidproject/mantid/pull/37074>`_ Fix crash on Indirect/Inelastic interfaces.

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v6.9.1
