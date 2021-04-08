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

- fixed a bug where `import CaChannel` would cause a hard crash within MantidWorkbench on Linux

- updated the ISIS Reflectometry live data monitor to work with new instrument PV names

- corrected the table value for a partially invalid, filtered log, with only one remaining entry

- fixed a bug where Muon Analysis would crash when using default grouping with no runs

- fixed an unhandled exception when marking columns in a table workspace for plotting or deleting rows
  from a PeaksWorkspace

- updated ALC documentation to clarify instrument used for data loading

- avoid error reporter on entering comma in scale box in MSlice

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
- `29706 <https://github.com/mantidproject/mantid/pull/29706>`_ Update hard-coded PV names in RROLD
- `29754 <https://github.com/mantidproject/mantid/pull/29754>`_ Filtered Value is displayed in Sample logs table
- `29708 <https://github.com/mantidproject/mantid/pull/29708>`_ Muon analysis crash when using default grouping with no runs
- `29792 <https://github.com/mantidproject/mantid/pull/29792>`_ Fixes exceptions on altering table/peaks workspace data
- `29796 <https://github.com/mantidproject/mantid/pull/29796>`_ Updated ALC documentation to clarify instrument used for data loading
- `552 <https://github.com/mantidproject/mslice/pull/552>`_ Avoid error reporter on entering comma in scale box in MSlice

Summary of impact
-----------------

+-------+---------------------------------------------------------------+--------------+
| Issue | Impact                                                        | Side Effect  |
|       |                                                               | Probability  |
+=======+===============================================================+==============+
| 29695 | Fixes cachannel import on Linux in workbench                  | **medium**   |
+-------+---------------------------------------------------------------+--------------+
| 29672 | Fixed muon fitting bugs                                       | **low**      |
+-------+---------------------------------------------------------------+--------------+
| 29706 | Update hard-coded PV names in RROLD                           | **low**      |
+-------+---------------------------------------------------------------+--------------+
| 29754 | Fixes filtered value is displayed in Sample logs table        | **low**      |
+-------+---------------------------------------------------------------+--------------+
| 29708 | Fixes Muon analysis crash using default grouping with no runs | **low**      |
+-------+---------------------------------------------------------------+--------------+
| 29792 | Fixes exceptions on altering table/peaks workspace data       | **low**      |
+-------+---------------------------------------------------------------+--------------+
| 29796 | Updated ALC documentation on instrument used for data loading | **low**      |
+-------+---------------------------------------------------------------+--------------+
| 552   | Avoid error reporter on entering comma in scale box in MSlice | **low**      |
+-------+---------------------------------------------------------------+--------------+


.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v5.1.1
