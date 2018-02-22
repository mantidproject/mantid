.. _v3.10.1:

===========================
Mantid 3.10.1 Release Notes
===========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 3.10.0 <v3.10.0>`.

There is no common theme to the fixes contained in this patch, rather
it is collection of small but significant fixes. Please see below for
the full list of changes.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.10.1: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
  `doi: 10.5286/Software/Mantid3.10.1 <http://dx.doi.org/10.5286/Software/Mantid3.10.1>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166 `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_ (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)

Changes in this version
-----------------------

* `19988 <https://github.com/mantidproject/mantid/pull/19988>`_ updated the amplitude in GausOsc
* `19995 <https://github.com/mantidproject/mantid/pull/19995>`_ Add support for 2d character arrays for sample name
* `20001 <https://github.com/mantidproject/mantid/pull/20001>`_ Fix type conversion warning on MSVC
* `20011 <https://github.com/mantidproject/mantid/pull/20011>`_ Copy units over logs

Summary of impact
-----------------

+-------+-------------------------------------------------------------+-----------------------------------------------------------+--------------+
| Issue | Impact                                                      | Solution                                                  | Side Effect  |
|       |                                                             |                                                           | Probability  |
+=======+=============================================================+===========================================================+==============+
| 19988 | The initial amplitude in the GausOsc functionis unrealistic | Change initial amplitude to a more realistic value is 0.2 | **low**      |
+-------+-------------------------------------------------------------+-----------------------------------------------------------+--------------+
| 19995 | SNS DAS now writes the sample name as a 2d character        | Add support for 2d character arrays for sample name       | **low**      |
+-------+-------------------------------------------------------------+-----------------------------------------------------------+--------------+
| 20001 | Type conversion warning on MSVC                             | Fix warning                                               | **low**      |
+-------+-------------------------------------------------------------+-----------------------------------------------------------+--------------+
| 20011 | The logs do not get their units copied over.                | Copy units over logs                                      | **low**      |
+-------+-------------------------------------------------------------+-----------------------------------------------------------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.10.1
