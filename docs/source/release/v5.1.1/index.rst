.. _v5.1.1:

==========================
Mantid 5.1.1 Release Notes
==========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 5.1.0 <v5.1.0>`.

The main changes are:

- a bug in the Muon interface that lead the error reporter to repeatedly appearing when incrementing
  the run number with sequential fitting enabled has been fixed

- a bug in the Muon interface that lead to error bars being lost on fitted data has been fixed

- fixed a bug where `import CaChannel` would cause a hard crash within MantidWorkbench on Linux.

- updated the ISIS Reflectometry live data monitor to work with new instrument PV names

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 5.1.1: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
  `doi: 10.5286/Software/Mantid5.1.1 <http://dx.doi.org/10.5286/Software/Mantid5.1.1>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments
  and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166
  `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_
  (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)

Changes in this version
-----------------------

- `29672 <https://github.com/mantidproject/mantid/pull/29672>`_ Fixed muon fitting bugs
- `29695 <https://github.com/mantidproject/mantid/pull/29695>`_ Workaround crash with importing CaChannel on Linux

Summary of impact
-----------------

+-------+----------------------------------------------+----------+--------------+
| Issue | Impact                                       | Solution | Side Effect  |
|       |                                              |          | Probability  |
+=======+==============================================+==========+==============+
| 29672 | Fixed muon fitting bugs                      |          | **low**      |
+-------+----------------------------------------------+----------+--------------+
| 29695 | Fixes cachannel import on Linux in workbench |          | **medium**   |
+-------+----------------------------------------------+----------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v5.1.1
