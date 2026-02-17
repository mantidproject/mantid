.. _v6.15.0:

===========================
Mantid 6.15.0 Release Notes
===========================

.. figure:: ../../images/6_15_release/mantid_6_15.png
   :class: screenshot
   :width: 385px
   :align: right

.. contents:: Table of Contents
   :local:

We are proud to announce version 6.15.0 of Mantid.

Key features of this release include:

- The new Instrument View has had several updates; read about them in the :doc:`Mantid Workbench <mantidworkbench>` release notes. This feature is still being developed and new features will be coming to the nightly.
- The :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` has two new tabs, ``Absorption Correction`` and ``Texture``. Read more about the work on texture in the :doc:`Diffraction <diffraction>` release notes.
- Lots up updates have been made to the :ref:`algm-AlignAndFocusPowderSlim`; see the :doc:`Diffraction <diffraction>` and :doc:`Framework <framework>` release notes for details.
- The :ref:`Bayes Fitting interface <interface-inelastic-bayes-fitting>` has a new combo box to swap between using the ``quasielasticbayes`` (old Fortran library) backend and the ``quickbayes`` (new python library) backend.
- The :ref:`ISIS_Sans_interface_contents` Interface has a new option for selecting a range of phi values in the masking tab to generate multiple phi slices from a single 1D reduction.

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
which now links to the assets on our `GitHub release page`_, where you can also
access the source code for the release.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 6.15.0: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/SOFTWARE/MANTID6.15 <https://dx.doi.org/10.5286/SOFTWARE/MANTID6.15>`_

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
- Low Q

  - :doc:`Reflectometry <reflectometry>`

  - :doc:`SANS <sans>`
- Spectroscopy

  - :doc:`Direct Geometry <direct_geometry>`

  - :doc:`Indirect Geometry <indirect_geometry>`

  - :doc:`Inelastic <inelastic>`

Full Change Listings
--------------------

For a full list of all issues addressed during this release please see the `GitHub milestone`_.

.. _download page: https://download.mantidproject.org

.. _forum: https://forum.mantidproject.org

.. _GitHub milestone: https://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=is%3Apr+milestone%3A%22Release+6.15%22+is%3Amerged

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v6.15.0
