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
- ISIS SANS history for top level algorithms now works correctly. A user
  can copy the history of a workspace to their clipboard or a file and the data
  will be reproduced on that machine without requiring editing of the script.

:ref:`Release 4.3.0 <v4.3.0>`
