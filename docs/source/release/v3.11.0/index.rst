.. _v3.11.0:

===========================
Mantid 3.11.0 Release Notes
===========================

.. figure:: ../../images/Mantid_10_years.png
   :class: screenshot
   :width: 385px
   :align: right

   Mantid celebrates 10 years of supporting Neutron and Muon research

.. contents:: Table of Contents
   :local:

We are proud to announce version 3.11.0 of Mantid, celebrating 10 years of the Mantid project supporting Neutron and Muon research.

This release includes improvements to allow Mantid to handle scanning workspaces, where detectors may be in several locations for different time periods, and updates to the VSI to improve the performance of slicing regularly binned data. There have also been some significant performance enhancements for various algorithms within the framework due to the ongoing efforts to improving our handling of instrument geometry.

These are just some of the many improvements in this release, so please take a
look at the release notes, which are filled with details of the
important changes and improvements in many areas. The development team
has put a great effort into making all of these improvements within
Mantid, and we would like to thank all of our beta testers for their
time and effort helping us to make this another reliable version of Mantid.

Thank you to everyone that has
reported any issues to us. Please keep on reporting any problems you
have, or crashes that occur on our `forum`_.

Installation packages can be found on our `download page`_
which now links to sourceforge to mirror our download files around the world, you can also
access the source code on `GitHub release page`_.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.11: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. doi: http://dx.doi.org/10.5286/SOFTWARE/MANTID3.11

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166 `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_ (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)

Changes
-------

.. toctree::
   :titlesonly:

   User Interface & Usability <ui>
   Framework <framework>
   Direct Inelastic <direct_inelastic>
   Indirect Inelastic <indirect_inelastic>
   SANS <sans>
   Diffraction <diffraction>
   Muon Analysis <muon>
   Reflectometry <reflectometry>

Full Change Listings
--------------------

For a full list of all issues addressed during this release please see the `GitHub milestone`_.

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub milestone: http://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Aclosed

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.11.0
