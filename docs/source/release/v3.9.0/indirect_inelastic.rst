==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

- :ref:`LoadILLIndirect <algm-LoadILLIndirect>` now checks in the ``.nxs`` files which single detectors (SD) are enabled, and loads only those instead of all.
- :ref:`IndirectILLReduction <algm-IndirectILLReduction>` and :ref:`ILLIN16BCalibration <algm-ILLIN16BCalibration>` algorithms are refactored to accommodate new requirements. This includes now handling of multiple files, unmirroring logic, debug mode, etc.. Changes are incorporated in Indirect Data Reduction GUI.
- :ref:`IndirectILLFixedWindowScans <algm-IndirectILLFixedWindowScans>` algorithm treats the elastic and inelastic fixed-window scan data from IN16B instrument at ILL.

Data Analysis
#############

- :ref:`TeixeiraWaterSQE <func-TeixeiraWaterSQE>` models translation of water-like molecules (jump diffusion).

Jump Fit
~~~~~~~~

Improvements
------------


Bugfixes
--------

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
