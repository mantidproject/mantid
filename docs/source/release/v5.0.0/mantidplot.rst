==================
MantidPlot Changes
==================

.. contents:: Table of Contents
   :local:

Improvements
############

Bugfixes
########
- The Show Instruments right click menu option is now disabled for workspaces that have had their spectrum axis converted to another axis using :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis>`.  Once this axis has been converetd the workspace loses it's link between the data values and the detectors they were recorded on so we cannot display it in the instrument view.
- Fixed an issue with Workspace History where unrolling consecutive workflow algorithms would result in only one of the algorithms being unrolled.
- Fixed an issue where adding specific functions to the multi-dataset fitting interface caused it to crash
- Fixed an issue where mantid crashed if you cleared the functions in the multi-dataset fitting interface
- Fixed an issue where adding a UserFunction to the multi-dataset fitting interface caused a crash


:ref:`Release 5.0.0 <v5.0.0>`