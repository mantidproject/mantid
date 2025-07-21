.. _v6.13.1:

===========================
Mantid 6.13.1 Release Notes
===========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 6.13.0 <v6.13.0>`.

The changes are:

- MantidWorkbench full installers will now work correctly on all Linux distributions.
- Muon Analysis and Frequency Domain Analysis interfaces will now open on all Linux distributions.
- :ref:`SaveNXSPE <algm-SaveNXSPE-v1>` will no longer throw an exception when creating nested groups with the same name.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 6.13.1: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
  `doi: 10.5286/Software/Mantid6.13.1 <http://dx.doi.org/10.5286/Software/Mantid6.13.1>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments
  and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166
  `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_
  (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)

Changes in this version
-----------------------

* `39670 <https://github.com/mantidproject/mantid/pull/39670>`_ Avoid Linux standalone launching error
* `39656 <https://github.com/mantidproject/mantid/pull/39656>`_ Remove link between muon and curvefitting libs
* `39686 <https://github.com/mantidproject/mantid/pull/39686>`_ Allow Nexus to make nested groups with same name

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v6.13.1
