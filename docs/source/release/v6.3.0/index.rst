.. _v6.3.0:

.. include:: <isonum.txt>

===========================
Mantid 6.3.0 Release Notes
===========================

.. figure:: ../../images/engggui_texture.png
   :class: screenshot
   :width: 350px
   :align: right

.. contents:: Table of Contents
   :local:

Summary
-------

We are proud to announce version 6.3.0 of Mantid.

Much of this version provides improvements for users, building on exisiting algorithms and GUIs.
Plotting has also seen a number of updates and users will benefit from improved documentation too.

In addition to many improvements we are delighted to announce some new features including:

- Full cross-platform support for Conda packages. See Conda_ for further details.
- The :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` now supports two texture groupings, ``Texture20`` and ``Texture30`` (see image above).
- The Error Reporter can now remember and prefill the user's name and e-mail address.
- The :ref:`InstrumentViewer` :ref:`Pick Tab<instrumentviewer_pick_tab>` has a new panel for allowing the rebinning of workspaces.
- In the :ref:`Frequency Domain Analysis <Frequency_Domain_Analysis-ref>` and the :ref:`Muon Analysis <Muon_Analysis-ref>` GUIs the confidence interval of a fit, previously shown as an error bar, is now represented by a shaded region.
- Several new fitting functions for Muons have been provided.

Conda
-----

.. figure:: ../../images/conda_logo.svg
   :align: center
   :width: 400px

   |copy| 2017 Continuum Analytics, Inc. (dba Anaconda, Inc.). https://www.anaconda.com. All Rights Reserved

We are excited to announce the release of brand-new packages built for the `Conda package manager <https://docs.conda.io/en/latest/>`__.
Up until this release only a single ``mantid-framework`` package has been available for Linux but
this release sees full support for our standard platforms (Linux, Windows, macOS).
We have also renamed the package from ``mantid-framework`` to ``mantid`` to align with the package name used in scripts.

For instructions on how to access the packages please see
the `Conda installation instructions <https://download.mantidproject.org/conda.html>`__.

Future Packaging Changes
------------------------

In future releases we will be reworking how our installer packages are constructed so that we can replace an
aging system that is hard to maintain and keep up to date.

Windows & macOS users should see no change in how these packages function and are used. However, the version of Python bundled
internally will start to change more regularly, at a frequency yet undefined, to keep up with external developments.

We plan to push Linux users towards the Conda distribution packages and dropping package (.deb and .rpm) builds in the future.
This would Mantid easier to install on different distros (though support would be unofficial).
We're open to feedback and suggestions on this direction.

Further Information
-------------------

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

- *Mantid 6.3.0: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/SOFTWARE/MANTID6.3 <https://dx.doi.org/10.5286/SOFTWARE/MANTID6.3>`_

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

.. _GitHub milestone: https://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=is%3Apr+milestone%3A"Release+6.3"+is%3Amerged

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v6.3.0
