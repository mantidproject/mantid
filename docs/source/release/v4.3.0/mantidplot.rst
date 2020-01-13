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

:ref:`Release 4.3.0 <v4.3.0>`