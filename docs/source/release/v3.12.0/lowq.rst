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

Small Angle Scattering
----------------------
- BeamCentreFinder has been implemented to work with the new backend in the python script window.
- Added functionality to specify q values between which merged data is used and outside of which pure HAB and LAB are used.
- Fixed a bug where specifying fit range was not working for merged reductions. Previously the user specified range was being ignored.
- Fixed a bug in the old GUI where loading files on UNIX systems would not work unless the file name was in uppercase letters.

:ref:`Release 3.12.0 <v3.12.0>`
