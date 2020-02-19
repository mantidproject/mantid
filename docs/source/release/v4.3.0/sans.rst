============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New
###
- New EQ-SANS instrument definition file to adjust the position of the detector array accoring to log entry "detectorZ".
- New D16 instrument definition file and loader to manage SANS data.

Improved
########
- Detector numbering is fixed for the SANS instruments D22 and D33 at the ILL.
- :ref:`MaskBTP <algm-MaskBTP>` now handles both old and new instrument definitions for BIOSANS and GPSANS
- :ref:`SANSILLReduction <algm-SANSILLReduction>` and :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` are improved to better handle the absolute scale normalisation.
- :ref:`SANSILLIntegration <algm-SANSILLIntegration>` will now offer to produce I(Q) for separate detector components separately, which is useful for D33.
- :ref:`SANSILLReduction <algm-SANSILLReduction>` will now allow for correct absolute scale normalisation even in circumstances when, for example, there is no flux measurement for the water run configuration.
- Data with invalid proton charge logs will now be fixed before performing
  slicing. A warning is emitted when this happens.
- ISIS SANS history for top level algorithms now works correctly. A user
  can copy the history of a workspace to their clipboard or a file and the data
  will be reproduced on that machine without requiring editing of the script.
- Batch CSV files can have their columns in any order and will load into
  the SANS GUI correctly.

Fixed
#####
- Tabbing between columns has been improved in the data GUI table. Users
  can now single tab between unmodified columns, or double tab for modified.
- Saving a CSV with sample geometry enabled, then reloading works correctly.
- Hitting Shift+Enter on the top row no longer causes an exception

Changes
#######
- Sample periods and geometry inputs have been move to the right of the table,
  as part of the tabbing improvements.

Fixed
#####
- A zero monitor shift did not previously account for the position
  of the rear detector for Zoom. A 0.0mm offset now works correctly when
  processing data from the SANS GUI or command interface.
- The Beam Centre Finder will now prints the average error in X, Y and the
  number of points considered. This allows for direct comparisons of different
  radius limits, which previously summed the difference across different
  numbers of points.

:ref:`Release 4.3.0 <v4.3.0>`
