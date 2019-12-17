=======================
MantidWorkbench Changes
=======================

.. contents:: Table of Contents
   :local:

Improvements
############

- Normalization options have been added to 2d plots and sliceviewer.

Bugfixes
########

- Colorbar scale no longer vanish on colorfill plots with a logarithmic scale
- Figure options no longer causes a crash when using 2d plots created from a script.
- Running an algorithm that reduces the number of spectra on an active plot (eg SumSpectra) no longer causes an error
- Fix crash when loading a script with syntax errors

:ref:`Release 4.3.0 <v4.3.0>`