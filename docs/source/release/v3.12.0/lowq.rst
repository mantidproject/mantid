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
- Fixed a bug where specifying fit range was not working for merged reductions. Previously the user specified range was being ignored.
- Fixed a bug in the old GUI where loading files on UNIX systems would not work unless the file name was in uppercase letters.


:ref:`Release 3.12.0 <v3.12.0>`
