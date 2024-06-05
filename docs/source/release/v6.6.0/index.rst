.. _v6.6.0:

===========================
Mantid 6.6.0 Release Notes
===========================

.. figure:: ../../images/6_6_release/MajorChanges.png
   :class: image
   :width: 600px
   :align: right

.. contents:: Table of Contents
   :local:

We are proud to announce version 6.6.0 of Mantid.

This release includes a wide array of updates, bugfixes, and new features, which build on the functionality of Mantid, including an upgrade to version 3.6 of matplotlib.

In addition to many improvements we are delighted to announce some new features including:


- A significant :ref:`ALFView-ref` upgrade, which enhances usability, streamlines the workflow, and delivers a more intuitive user experience.
- The addition of a :ref:`Reduction Preview <refl_preview>` tab to the ISIS Reflectometry interface.
- The new :ref:`interface-inelastic-data-processor` interface, which has been created from the old Data Reduction interface and two of the tabs from the Analysis interface.
- New :ref:`plots using matplotlib 3D<Mesh_Plots>` for viewing the Sample Shape of a workspace.


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

- *Mantid 6.6.0: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/SOFTWARE/MANTID6.6 <https://dx.doi.org/10.5286/SOFTWARE/MANTID6.6>`_

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

.. _GitHub milestone: https://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=is%3Apr+milestone%3A%22Release+6.6%22+is%3Amerged

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v6.6.0
