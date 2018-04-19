.. _v3.12.1:

===========================
Mantid 3.12.1 Release Notes
===========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 3.12.0 <v3.12.0>`.

The main changes are:

* Several fixes to the Muon Analysis GUI, including to the results table and fit menu.
* Several issues which caused mantid to crash have been fixed.
* Allowing the live listener funtionality to be used outside ISIS and from the python API.
* Fixing the header for TOPAS files.
* Removed version 1 of ``ReflectometryReductionOne`` and ``ReflectometryReductionOneAuto``

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.12.1: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
  `doi: 10.5286/Software/Mantid3.12.1 <http://dx.doi.org/10.5286/Software/Mantid3.12.1>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments
  and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166
  `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_
  (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)

Changes in this version
-----------------------

* `22205 <https://github.com/mantidproject/mantid/pull/22205>`_ Fix header for TOPAS files
* `22213 <https://github.com/mantidproject/mantid/pull/22215>`_ Fix bug when using StartLiveData through Python API
* `22195 <https://github.com/mantidproject/mantid/pull/22195>`_ CrystalField Multi-spectrum resolution model segfault
* `22194 <https://github.com/mantidproject/mantid/pull/22194>`_ SofQW3 segfault
* `22190 <https://github.com/mantidproject/mantid/pull/22190>`_ OSX Muon Interface (No need for data)
* `22182 <https://github.com/mantidproject/mantid/pull/22182>`_ Update mslice to fix issue with matplotlib < 1.5
* `22200 <https://github.com/mantidproject/mantid/pull/22200>`_ Fix unreliable tests: Disable ClearCache doc test
* `22244 <https://github.com/mantidproject/mantid/pull/22244>`_ MR: correct dQ
* `22178 <https://github.com/mantidproject/mantid/pull/22178>`_ Muon Analysis bug that disabled fit menu
* `22177 <https://github.com/mantidproject/mantid/pull/22177>`_ Muon analysis and results table
* `21655 <https://github.com/mantidproject/mantid/pull/21655>`_ Remove dependence of Kafka Live Listener on ISIS specific event data
* `22226 <https://github.com/mantidproject/mantid/pull/22226>`_ Error when deleting a workspace group in MantidPlot
* `20997 <https://github.com/mantidproject/mantid/pull/20997>`_ Re #20991: Updated Reflectometry IDFs
* `22290 <https://github.com/mantidproject/mantid/pull/22290>`_ Updated print statement to python 3 in SaveWorkspaces
* `22152 <https://github.com/mantidproject/mantid/pull/22152>`_ Updated IDF for D2B
* `22175 <https://github.com/mantidproject/mantid/pull/22175>`_ Fix failing Reflectometry doctests

Summary of impact
-----------------

+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| Issue | Impact                                                                                  | Solution                  | Side Effect  |
|       |                                                                                         |                           | Probability  |
+=======+=========================================================================================+===========================+==============+
| 22205 | Fix header for TOPAS files                                                              | Check for header type     | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22215 | Fix bug when using StartLiveData through Python API                                     | Remove kwarg if None      | **medium**   |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22195 | CrystalField Multi-spectrum resolution model segfault                                   | Check sizes               | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22194 | SofQW3 segfault no longer occurs                                                        | Indexing change           | **medium**   |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22190 | OSX Muon Interface data requirments fixed                                               | GUI changes               | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22182 | Update mslice to fix issue with matplotlib < 1.5                                        | Update sha1               | **medium**   |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22200 | Fix unreliable tests: Disable ClearCache doc test                                       | Clear cache before build  | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22244 | Fix dQ calculation in MR Reduction                                                      | Now uses radians          | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22178 | Fix menu is Muon Analysis not disabled                                                  | Change enabled conditions | **medium**   |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22177 | Muon analysis results table generated correctly                                         | Additional checks         | **medium**   |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 21655 | Remove dependence of Kafka Live Listener on ISIS specific event data                    | Remove dependence         | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22226 | Error when deleting a workspace group in MantidPlot                                     | Better thread safety      | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 20997 | Updated Reflectometry IDFs                                                              | Changed IDFs              | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 20997 | Removed version 1 of ``ReflectometryReductionOne`` and ``ReflectometryReductionOneAuto``| Removed old algorithms    | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22290 | Updated print statement in SaveWorkspaces                                               | Made Python3 compatible   | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 11152 | Updated IDF for D2B                                                                     | Updated IDF               | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+
| 22175 | Fix failing Reflectometry doctests                                                      | Updated expected values   | **low**      |
+-------+-----------------------------------------------------------------------------------------+---------------------------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.12.1
