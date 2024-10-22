==================
MantidPlot Changes
==================

.. contents:: Table of Contents
   :local:

The vast majority of development effort for user interfaces is now directed towards the :doc:`Mantid workbench <mantidworkbench>`, for now only significant bugs will be fixed within MantidPlot.

Bugfixes
########

- The Show Instrument right click menu option is now disabled for workspaces that have had their spectrum axis converted to another axis using :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis>`.  Once this axis has been converetd the workspace loses it's link between the data values and the detectors they were recorded on so we cannot display it in the instrument view.
- Fixed an issue with Workspace History where unrolling consecutive workflow algorithms would result in only one of the algorithms being unrolled.
- Fixed an issue where adding specific functions to the multi-dataset fitting interface caused it to crash
- Fixed an issue where mantid crashed if you cleared the functions in the multi-dataset fitting interface
- Fixed an issue where adding a UserFunction to the multi-dataset fitting interface caused a crash
- Fixed an issue in the multi-dataset fitting interface where it crashed when doing a sequential fit with one spectra.


:ref:`Release 5.0.0 <v5.0.0>`
