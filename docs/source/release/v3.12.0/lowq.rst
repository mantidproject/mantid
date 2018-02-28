=============
Low-Q Changes
=============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Reflectometry
-------------

Features Removed
################

* The REFL Reduction, REFL SF Calculator and REFM Reduction graphical interface have been removed, they were not in active use, and were a source of bugs as well as using testing effort that is better directed elsewhere.

Small Angle Scattering
----------------------
- Grid lines are now displayed in ISIS SANS V2
- Added the option to hide the period selection columns in the SANS GUI V2
- BeamCentreFinder has been implemented to work with the new backend in the python script window.
- Added functionality to specify q values between which merged data is used and outside of which pure HAB and LAB are used.
- Have added the functionality to show diagnostic transmission workspaces to new GUI.
- Have added functionality to continually plot latest results to new GUI.
- Added find beam centre tab to SANS GUI V2.
- Fixed an issue where merged or all reductions were overwriting each other as they were being given the same name.
- Fixed a bug where specifying fit range was not working for merged reductions. Previously the user specified range was being ignored.
- Fixed a bug in the old GUI where loading files on UNIX systems would not work unless the file name was in uppercase letters.
- Fixed a bug in the old GUI where merged reductions of time sliced data was not working.
- Fixed a bug where 2D reductions were being run in 1D if a new user file was specified in a batch file.

:ref:`Release 3.12.0 <v3.12.0>`
