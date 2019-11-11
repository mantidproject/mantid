.. _v4.2.0:

===========================
Mantid 4.2.0 Release Notes
===========================

.. figure:: ../../images/fish.jpeg
   :class: screenshot
   :width: 500px
   :align: right

.. contents:: Table of Contents
   :local:

We are proud to announce version 4.2.0 of Mantid.

**THIS IS THE LAST RELEASE THAT WILL SUPPORT Python 2**

| From v4.3 onwards Mantid will no longer support Python 2 and scripts run in Mantid will have to change to reflect this. `Time is Ticking <https://pythonclock.org/>`_! 
| NB. Mantid v4.3 will be released near the End of February 2020.

| *Interfaces, interfaces, interfaces*! Hard to say but Easy to use!
| One of the major changes for this release is the new Direct :ref:`ALF View <ALF_View-ref>` GUI for aligning samples on the ALF Instrument. There have also been serious developments in the
:doc:`Indirect <indirect_geometry>` Interfaces: 
| The Analysis and Bayes Interfaces have been introduced to Mantid Workbench and all interfaces will accept lists to plot multiple spectra. 
| Generally usuability has been improved, from the cleaner :doc:`ISIS SANS <sans>` Interface or the multiple improvements in Workbench. The Script Repository has even been added into :doc:`Workbench <mantidworkbench>`!

These are just a few of the many improvements in this release, so please take a
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
which now links to sourceforge to mirror our download files around the world, you can also
access the source code on `GitHub release page`_.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 4.2.0: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/SOFTWARE/MANTID4.2 <http://dx.doi.org/10.5286/SOFTWARE/MANTID4.2>`_

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
- User Interfaces

  - :doc:`MantidPlot <mantidplot>`

  - :doc:`MantidWorkbench <mantidworkbench>`
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

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub milestone: http://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=is%3Apr+milestone%3ARelease 4.2+is%3Amerged

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v4.2.0
