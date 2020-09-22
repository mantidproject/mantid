============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Algorithms and instruments
--------------------------

New
###

- A new EQ-SANS instrument definition file has been added to adjust the position of the detector array
  according to log entry "detectorZ".
- A new D16 instrument definition file and loader has been added to manage SANS data.

Improved
########

- Detector numbering is fixed for the SANS instruments D22 and D33 at the ILL.
- The :ref:`MaskBTP <algm-MaskBTP>` algorithm now handles both old and new instrument definitions for BIOSANS and GPSANS.
- The :ref:`SANSILLReduction <algm-SANSILLReduction>` and :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`
  algorithms are improved to better handle the absolute scale normalisation.
- The :ref:`SANSILLReduction <algm-SANSILLReduction>` algorithm will now allow for correct absolute scale
  normalisation even in circumstances when, for example, there is no flux measurement for the water run configuration.
- The :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` algorithm now supports gamma scans for D16 at the ILL.
- The :ref:`SANSILLIntegration <algm-SANSILLIntegration>` algorithm will now offer to produce I(Q) for each detector
  component separately, which is useful for D33.
- Data with invalid proton charge logs will now be fixed before performing slicing. A warning is emitted when this happens.
- ISIS history for top level algorithms now works correctly. The history of a workspace can be copied
  to the clipboard or a file and the data will be reproduced without requiring editing of the script.


ISIS SANS Interface
-------------------

Improved
########
- The Beam Centre Finder will now print the average error in X, Y and the
  number of points considered. This allows for direct comparisons of different
  radius limits, which previously summed the difference across different
  numbers of points.
- The beam centre finder correctly centres HAB and LAB banks; previously it
  only worked for LAB.
- Batch CSV files can have their columns in any order and will load into
  the SANS GUI correctly.
- Tabbing between columns has been improved in the data GUI table. Users
  can now single tab between unmodified columns, or double tab for modified.
- Sample periods and geometry inputs have been moved to the right of the table,
  as part of the tabbing improvements.


Fixed
#####
- A zero monitor shift did not previously account for the position
  of the rear detector for Zoom. A 0.0mm offset now works correctly when
  processing data from the SANS GUI or command interface.
- The help documentation now loads correctly
- Saving a CSV with sample geometry enabled, then reloading works correctly.
- Hitting Shift+Enter on the top row no longer causes an error.


ORNL SANS Interface
-------------------

- A bug has been fixed where only facilities with applicable instruments can now be selected.
- A bug has been fixed which was causing the interface to crash when selecting HFIR instruments.


:ref:`Release 5.0.0 <v5.0.0>`
