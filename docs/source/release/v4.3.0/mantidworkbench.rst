=======================
MantidWorkbench Changes
=======================

.. contents:: Table of Contents
   :local:

Improvements
############

.. figure:: ../../images/Notification_error.png
   :class: screenshot
   :width: 600px
   :align: right

- If you have ever found it hard to spot when errors appear in the Messages window, and perhaps miss them if there are lots of graphs on the screen, then you will like this.  We have added system notifications when Mantid enounters an error, and directs you to look at the Messages window for details.  You can enable or disable these notifications from the File->Settings window.

.. figure:: ../../images/Notifications_settings.png
   :class: screenshot
   :width: 500px
   :align: left

- Normalization options have been added to 2d plots and sliceviewer.
- The images tab in figure options no longer forces the max value to be greater than the min value.


Bugfixes
########

- Colorbar scale no longer vanish on colorfill plots with a logarithmic scale
- Figure options no longer causes a crash when using 2d plots created from a script.
- Running an algorithm that reduces the number of spectra on an active plot (eg SumSpectra) no longer causes an error
- Fix crash when loading a script with syntax errors

:ref:`Release 4.3.0 <v4.3.0>`
