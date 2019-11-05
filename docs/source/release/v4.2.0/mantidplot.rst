==================
MantidPlot Changes
==================

.. contents:: Table of Contents
   :local:

.. figure:: ../../images/MantidPlotMeme.jpeg
   :class: screenshot
   :width: 500px
   :align: right

Improvements
############

The vast majority of development effort for user interfaces is now directed towards the :doc:`Mantid workbench <mantidworkbench>`, for now only significant bugs will be fixed within MantidPlot.

- Saving a project larger than 10GB produces a pop-up to inform it may take a long time and gives the opportunity to cancel.

Bugfixes
########
- Algorithm progress bar now shows correct units for time remaining.
- Sample Transmission Calculator no longer has the option to change the Y-axis of the plot to logarithmic.
- Sample Transmission Calculator no longer accepts negative wavelength values.
- Fixes an issue where the * to indicate an invalid property would appear in the wrong place in the Load dialog.
- Fixes an issue where the Load dialog would not resize correctly after clicking Run.
- Useless Help [?] buttons removed from multiple dialogs.
- Dragging and dropping workspaces onto the Spectrum Viewer no longer causes a crash.

:ref:`Release 4.2.0 <v4.2.0>`
