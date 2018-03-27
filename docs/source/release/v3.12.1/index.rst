.. _v3.12.1:

==========================
Mantid 3.12.1 Release Notes
==========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 3.12.0 <v3.12.0>`.

The main changes are:

**ADD SUMMARY HERE**

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

* `22200 <https://github.com/mantidproject/mantid/pull/22200>`_ Fix unreliable tests: Disable ClearCache doc test
* `22205 <https://github.com/mantidproject/mantid/pull/22205>`_ Fix header for TOPAS files
* `22175 <https://github.com/mantidproject/mantid/pull/22175>`_ Fix failing Reflectometry doctests

Summary of impact
-----------------

+-------+---------------------------------------------------+---------------+--------------+
| Issue | Impact                                            | Solution      | Side Effect  |
|       |                                                   |               | Probability  |
+=======+===================================================+===============+==============+
| 22200 | Fix unreliable tests: Disable ClearCache doc test |               | **unknown**  |
+-------+---------------------------------------------------+---------------+--------------+
| 22205 | Fix header for TOPAS files                        |               | **unknown**  |
+-------+---------------------------------------------------+---------------+--------------+
| 22175 | Fix failing Reflectometry doctests                | Update values | low          |
+-------+---------------------------------------------------+---------------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.12.1
