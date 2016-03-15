=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Reflectometry Instruments
--------------------------

An updated version of the OFFSPEC IDF is now being used in mantid `#15561 <https://github.com/mantidproject/mantid/pull/15561>_`
   
ConvertToReflectometryQ
-----------------------

- A bug producing dark regions in *QxQz* maps was fixed `#15321 <https://github.com/mantidproject/mantid/pull/15321>`_

Reflectometry Reduction Interface
---------------------------------

ISIS Reflectometry (Polref)
###########################

- Bugfix: When using the ICAT search in the interface, if the wrong user credentials were entered or the login dialog
  was closed before the details had been entered then mantid crashed. This has been fixed.
  `#15410 <https://github.com/mantidproject/mantid/pull/15410>`_
- Some changes were made to the interface (functional behaviour remains the same). Specifically, options related to
  the processing table have been moved to the *Process Runs* section. Therefore, there is only one menu left,
  *Reflectometry*, to open the Slit Calculator. All table options are still accesible from the tool bar.
  A new progress bar has been added to the *Search Runs* section, it show the progress when
  transferring runs. The progress bar that existed before will only indicate the progress of processing that is
  in progress.

.. figure:: /images/ISISReflectometryPolref_newTableView.png

ISIS Reflectometry
##################

- A bug has been fixed regarding the calculation of the Q range for the ISIS Reflectometry interface where constants
  were being added to the values of Lambda min and max causing the calculation of the Qrange to be affected.
  `#15143 <https://github.com/mantidproject/mantid/pull/15143>`_

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
