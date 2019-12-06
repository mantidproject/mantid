============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New
###
- New EQ-SANS instrument definition file to adjust the position of the detector array accoring to log entry "detectorZ".


Improved
########
- :ref:`MaskBTP <algm-MaskBTP>` now handles both old and new instrument definitions for BIOSANS and GPSANS
- Data with invalid proton charge logs will now be fixed before performing
  slicing. A warning is emitted when this happens.
- Multiple runs with any identical run numbers (such as can scattering runs)
  now process faster whilst using the search data archive feature to find
  those runs. Testing shows that typical user batch files process up to
  55% faster.

:ref:`Release 4.3.0 <v4.3.0>`
