============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Features
----------

- :ref:`CropToComponent <algm-CropToComponent>` allows for cropping a workspace to a list of component names.
- Detect missing Bench_Rot for LARMOR and provide meaningful error message.
- Enable the CanSAS1D algorithms to handle geometry inforamtion.
- Add sort option to :ref:`CropToComponent <algm-CropToComponent>`.
- Provide warning when users try to use a 2D reduction together with a merged reduction selection.
- Processing of LOQ M4 in the SANS reduction was added.
- :ref:`UnwrapMonitorsInTOF <algm-UnwrapMonitorsInTOF>` handles the data, which was collected beyond the end of a frame.


Bug Fixes
---------

- Fix for beam center finder.
- Fixed loading of multiperiod event files.
- Fixed period selection when loading multiperiod files.
- Fixed process note for SaveCanSAS1D.
- Fixed output names for batch processing.
- Fixed the loading of RKH files.
- Fixed wrong initial position of LARMOR data in the beam centre finder.
- Allow loading of CanSAS data without error data.
- Fixed saving CanSAS with transmission data from the ISIS SANS GUI.

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+SANS%22>`__
