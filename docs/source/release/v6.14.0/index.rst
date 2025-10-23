.. _v6.14.0:

===========================
Mantid 6.14.0 Release Notes
===========================

.. figure:: ../../images/texture-direction-peaks.gif
   :class: screenshot
   :width: 425px
   :align: right

.. contents:: Table of Contents
   :local:

We are proud to announce version 6.14.0 of Mantid.

Key features of this release include:

- This is the final release of Mantid to include a package built for Intel Macs. There will continue to be packages built for Apple Silicon (M series processors).
- The ``mslice`` package is now an optional dependency of ``mantidworkbench``. To install it alongside ``mantidworkbench`` run ``mamba install -c mantid mantidworkbench mslice``. Full/standalone installers and IDAaaS installations remain unchanged; mslice will be automatically provided there.
- An experimental version of the new Instrument View has been made available in this release. To access this, right-click on a workspace in the ADS in Workbench, then click ``(Experimental) Show Instrument``. This feature is still in an early stage of development with further functionality being added over the v6.15 development period.
- The final pieces of work to support the polarised SANS technique at ISIS have been finished, see :doc:`the SANS release notes <sans>` for more details.
- In the :ref:`SliceViewer <sliceviewer>`, a masking feature for Matrix Workspaces with a non-numeric y-axis has been added.
- :ref:`Texture Analysis <TextureAnalysis>` can now be performed using the logic included in ``Engineering.texture.TextureUtils`` and a collection of scripts that can be found in ``diffraction/ENGINX/Texture`` within the :ref:`mantid script repository <WorkbenchScriptRepository>`.
- A collection of improvements have been made to :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>`, see :doc:`the Diffraction release notes <diffraction>` for more details.

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

- *Mantid 6.14.0: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/SOFTWARE/MANTID6.14 <https://dx.doi.org/10.5286/SOFTWARE/MANTID6.14>`_

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

Full Change Listings
--------------------

For a full list of all issues addressed during this release please see the `GitHub milestone`_.

.. _download page: https://download.mantidproject.org

.. _forum: https://forum.mantidproject.org

.. _GitHub milestone: https://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=is%3Apr+milestone%3A%22Release+6.14%22+is%3Amerged

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v6.14.0
