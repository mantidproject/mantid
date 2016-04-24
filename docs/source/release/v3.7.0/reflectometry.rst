=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Reflectometry Instruments
--------------------------

An updated version of the OFFSPEC IDF is now being used in mantid `#15561 <https://github.com/mantidproject/mantid/pull/15561>`_
   
ConvertToReflectometryQ
-----------------------

- A bug producing dark regions in *QxQz* maps was fixed `#15321 <https://github.com/mantidproject/mantid/pull/15321>`_

ReflectometryReductionOne
-------------------------

- Transmission corrections options are now applicable with both PointDetectorAnalysis and MultiDetectorAnalysis modes as long as a first 
  transmission workspace has been provided. `#15683 <https://github.com/mantidproject/mantid/pull/15683>`_

Reflectometry Reduction Interface
---------------------------------

ISIS Reflectometry (Polref)
###########################

- Bugfix: When using the ICAT search in the interface, if the wrong user credentials were entered or the login dialog
  was closed before the details had been entered then mantid crashed. This has been fixed.
  `#15410 <https://github.com/mantidproject/mantid/pull/15410>`_
- Bugfix: When attempting to plot rows in the Processing Table, if the processing table contained a row without an associated Run Number
  Mantid will raise an unexpected exception. This has now been fixed resulting in a warning being shown to the user that a certain row does not
  contain a Run Number. `#15820 <https://github.com/mantidproject/mantid/pull/15820>`_
- Some changes were made to the interface as part of a code refactoring (functional behaviour remains the same). 
  The only change visible to user is a new progress bar that has been added to the *Search Runs* section. It show the progress when
  transferring runs. The progress bar that existed before will only indicate the progress of processing that is
  in progress. `#15670 <https://github.com/mantidproject/mantid/pull/15670>`_

.. figure:: /images/ISISReflectometryPolref_newTableView.png

ISIS Reflectometry
##################

- A bug has been fixed regarding the calculation of the Q range for the ISIS Reflectometry interface where constants
  were being added to the values of Lambda min and max causing the calculation of the Qrange to be affected.
  `#15143 <https://github.com/mantidproject/mantid/pull/15143>`_


`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__