=====================
Imaging Changes
=====================

.. contents:: Table of Contents
   :local:

Tomographic reconstruction graphical user interface
###################################################

- Running local reconstructions is now possible, only a single reconstruction process can be run at any one time

Bug Fixes
---------
- The external interpreter and scripts paths are no longer ignored and are now correctly appended when running a local reconstruction
- Local reconstruction processes are now also updated in the reconstruction jobs list
- The reconstruction jobs list command line is now Read Only
- Clicking Cancel now cancels the running reconstruction process
- The reconstruction scripts will now work with TomoPy v1.x.x, however the results have yet to be tested
- Selecting the Center of Rotation, Area for Normalsation and Region of Interest will now always follow the exact position of the mouse
- Multiple success/warning/error messages will no longer be shown after an operation. 
|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`__
