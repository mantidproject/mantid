=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ALF View
########

Improvements
------------
- Added unit tests to the GUI, this should ensure stability. 

Algorithms
##########

Improvements
------------

For ILL time-of-flight instruments, :ref:`LoadILLTOF <algm-LoadILLTOF-v2>` now allows bin edges determined by channel
number by default, but you can still convert to time-of-flight using the `ConvertToTOF` property.
The incident energy calibration has been fixed for PANTHER and IN6 in :ref:`DirectILLCollectData <algm-DirectILLCollectData-v1>`.

Bugfixes
--------

- A bug introduced in v5.0 causing error values to tend to zero on multiple instances of :ref:`Rebin2D <algm-Rebin2D>` on the same workspace has been fixed.

:ref:`Release 5.1.0 <v5.1.0>`
