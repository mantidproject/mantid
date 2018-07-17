.. _v3.13.0:

===========================
Mantid 3.13.0 Release Notes
===========================

.. figure:: ../../images/Release3-13.png
   :class: screenshot
   :align: right
   
   Mantid introduces project recovery for Mantidplot, Mantid maintains a recovery script while running, in the event of a crash or unexpected shutdown of Mantidplot Mantid will attempt to recover to it's most recent recovery checkpoint.

.. contents:: Table of Contents
   :local:


We are proud to announce version 3.13.0 of Mantid.

This release includes a first implementation of Project Recovery for Mantidplot to allow Mantid to handle scanning workspaces, Mantid maintains a recovery script while running, in the event of a crash or unexpected shutdown of Mantidplot Mantid will attempt to recover to it's most recent recovery checkpoint.  This feature may not fully recover in all situations, but we will continue to improve it in future releases. There have also been some significant performance enhancements in the instrument view.

These are just some of the many improvements in this release, so please take a
look at the release notes, which are filled with details of the
important changes and improvements in many areas. The development team
has put a great effort into making all of these improvements within
Mantid, and we would like to thank all of our beta testers for their
time and effort helping us to make this another reliable version of Mantid.

Citation
--------

Please cite any usage of Mantid as follows: **TODO update with current version doi**

- *Mantid 3.13.0: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/SOFTWARE/MANTID3.13.0 <http://dx.doi.org/10.5286/SOFTWARE/MANTID3.13.0>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166 `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_ (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)


Changes
-------

.. toctree::
   :titlesonly:

   Framework <framework>
   User Interface & Usability <ui>
   Diffraction <diffraction>
   Muon Analysis <muon>
   Reflectometry <reflectometry>
   SANS <sans>
   Direct Inelastic <direct_inelastic>
   Indirect Inelastic <indirect_inelastic>
   Instrument Visualization <instrument_view>

Full Change Listings
--------------------

For a full list of all issues addressed during this release please see the `GitHub milestone`_.

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub milestone: http://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=is%3Apr+milestone%3A"Release+3.13"+is%3Amerged

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.13.0
