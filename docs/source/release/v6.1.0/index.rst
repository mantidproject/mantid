.. _v6.1.0:

===========================
Mantid 6.1.0 Release Notes
===========================

.. figure:: ../../images/ReleaseNotesIndex_v6_1.png
   :class: screenshot
   :width: 440px
   :align: right

.. contents:: Table of Contents
   :local:


We are proud to announce version 6.1.0 of Mantid.

This release includes several improvements to the user experience, notably a new widget to display total memory usage
and an additional setting to the window behaviour of plots and interfaces.

Other highlights from this release include:

- A new LeanElasticPeak object for constant wavelength instruments that is supported in existing algorithms for e.g. peak prediction and integration.
- Added setting to allow user to select Floating/On Top behaviour.
- Sectors drawn in the instrument viewer can now be used to define wedges for azimuthal averaging of 2D SANS data using :ref:`Q1DWeighted <algm-Q1DWeighted>`.
- :ref:`ConvertUnits <algm-ConvertUnits>` algorithm has been extended to use diffractometer constants from instrument calibration.
- New algorithm :ref:`GeneratePythonFitScript <algm-GeneratePythonFitScript>` allows the creation of a python script for sequential fitting.
- New algorithm :ref:`CalculateMultipleScattering <algm-DiscusMultipleScatteringCorrection>` to calculate multiple scattering corrections.

These are just some of the many improvements in this release, so please take a
look at the release notes, which are filled with details of the
important changes and improvements in many areas. The development team
has put a great effort into making all of these improvements within
Mantid, and we would like to thank all of our beta testers for their
time and effort helping us to make this another reliable version of Mantid.

Throughout the Mantid project we put a lot of effort into ensuring
Mantid is a robust and reliable product. Thank you to everyone that has
reported any issues to us. Please keep on reporting any problems you
have, or crashes that occur on our `forum`_.

Installation packages can be found on our `download page`_
which now links to sourceforge to mirror our download files around the world. You can also
access the source code on `GitHub release page`_.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 6.1.0: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/SOFTWARE/MANTID6.1 <https://dx.doi.org/10.5286/SOFTWARE/MANTID6.1>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments
  and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166
  `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_
  (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)


Changes
-------

.. toctree::
   :hidden:
   :glob:

   *

- :doc:`Framework <framework>`
- :doc:`Mantid Workbench <mantidworkbench>`
- :doc:`Diffraction <diffraction>`
- :doc:`Muon Analysis <muon>`
- Low Q

  - :doc:`Reflectometry <reflectometry>`

  - :doc:`SANS <sans>`
- Spectroscopy

  - :doc:`Direct Geometry <direct_geometry>`

  - :doc:`Indirect Geometry <indirect_geometry>`

Full Change Listings
--------------------

For a full list of all issues addressed during this release please see the `GitHub milestone`_.

.. _download page: https://download.mantidproject.org

.. _forum: https://forum.mantidproject.org

.. _GitHub milestone: https://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=is%3Apr+milestone%3A"Release+6.1"+is%3Amerged

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v6.1.0
