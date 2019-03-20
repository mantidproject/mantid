======================
UI & Usability Changes
======================

.. contents:: Table of Contents
   :local:


Installation
------------

The following changes have been made **only** on Windows, bringing it in line with Mantid's installation naming on Linux.

- The Mantid Nightly build will now be installed in a different directory by default, to avoid overwriting the release Mantid installation.
- The Mantid Nightly build desktop and start menu shortcuts will have the ``Nightly`` suffix appended, to distinguish it from the release Mantid installation.

MantidWorkbench
---------------

This is the first release containing the new GUI, MantidWorkbench, that will eventually replace MantidPlot.

See :doc:`mantidworkbench` for more details.

MantidPlot
----------

See :doc:`mantidplot` for more details.

Documentation
-------------

- The entry pages for the Mantid documentation have been improved, both the welcome page, and the algorithms reference section.
- The algorithms references section is now based around the algorithm categories, rather than the complete alphabetical list. The full list is still available for those that remember it with fond memories.


Instrument View
---------------

- The miniplot on the pick tab of the instrument view now shows the HKL values for peaks when viewing a summed collection of detectors.

:ref:`Release 4.0.0 <v4.0.0>`
