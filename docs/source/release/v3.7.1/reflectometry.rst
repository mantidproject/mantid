=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Reflectometry Instruments
--------------------------

- An updated version of the OFFSPEC IDF is now being used in mantid
- CRISP and POLREF IDFs were causing problems in the ISIS Reflectometry (Polref) interface as they were using the `opening height` tag
  when defining their slits. This has now been changed to be uniform across all reflectometry instruments (INTER, POLREF, CRISP, SURF, OFFSPEC)
  to `vertical gap` such that the CalculateResolution algorithm invoked by the interface will now work correctly.

ConvertToReflectometryQ
-----------------------

- A bug producing dark regions in *QxQz* maps was fixed

ReflectometryReductionOne
-------------------------

- Transmission corrections options are now applicable with both PointDetectorAnalysis and MultiDetectorAnalysis modes as long as a first
  transmission workspace has been provided. `#15683 <https://github.com/mantidproject/mantid/pull/15683>`_

- In CreateTransmissionWorkspace and ReflectometryReductionOne the default value for the algorithm property WavelengthStep has been changed
  from 0.05 to 0.02.


Reflectometry Reduction Interface
---------------------------------

- The ReflTBL data handling algorithms (namely SaveReflTBL and LoadReflTBL) have been generalised to allow for any number of column headings and
  custom column heading titles. Any old ReflTBL files will still work with the new algorithms (SaveTBL and LoadTBL) however any new tables created
  using SaveTBL will now be made with the new format. In the new format, the first line of the TBL file will contain a comma-separated list of column headings
  and all subsequent lines will define the data for each row. For an example this new format see here `LoadTBL <http://docs.mantidproject.org/nightly/algorithms/LoadTBL-v1.html>`_ .

ISIS Reflectometry (Polref)
###########################

- Bugfix: When using the ICAT search in the interface, if the wrong user credentials were entered or the login dialog
  was closed before the details had been entered then mantid crashed. This has been fixed.
- Bugfix: When attempting to plot rows in the Processing Table, if the processing table contained a row without an associated Run Number
  Mantid will raise an unexpected exception. This has now been fixed resulting in a warning being shown to the user that a certain row does not
  contain a Run Number.
- Some changes were made to the interface as part of a code refactoring (functional behaviour remains the same).
  As a consequence there are some changes visible to users: a new progress bar that has been added to the *Search Runs* section, which shows the progress when
  transferring runs. The progress bar that existed before will only indicate the progress of processing that is
  in progress. A new section below the processing table has been added. It
  summarises the algorithms used in the reduction and allows users to specify global options for them. Options to be applied to individual rows can still
  be specified via the 'Options' column. User documentation has been updated accordingly.
  The aim of this code refactoring is to have a General Data Processor Framework shared across different technique areas that need to execute complex
  batch-processing via DataProcessorAlgorithms. Standardisation across similar user interfaces will help avoid the maintenance effort involved in looking
  after specific user interfaces, making the wider project more robust to changes thanks to the introduction of fast, automated testing.
  Some of the target user interfaces that can benefit from this new design are SANS (both at ANSTO and ISIS), Powder at SNS and SCD at SNS.

.. figure:: /images/ISISReflectometryPolref_newTableView.png

ISIS Reflectometry
##################

- A bug has been fixed regarding the calculation of the Q range for the ISIS Reflectometry interface where constants
  were being added to the values of Lambda min and max causing the calculation of the Qrange to be affected.


`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
