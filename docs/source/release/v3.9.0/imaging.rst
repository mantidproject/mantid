=====================
Imaging Changes
=====================

.. contents:: Table of Contents
   :local:

Tomographic reconstruction graphical user interface
###################################################

- Running local reconstructions is now possible, only a single reconstruction process can be run at any one time.
- You can now use Auto-find button in the ROI tab to calculate the Center of Rotation for a sample using TomoPy. The calculated Center of Rotation is automatically loaded back into the GUI. Both the Center of Rotation relative to the crop and the full image are printed. Remote submission for finding the Center of Rotation can be done via the Custom Command tool.
- Run and Setup tabs have now been merged together. Functionality for loading an image in the Run tab has been removed.

Bug Fixes
---------
- The external interpreter and scripts paths are no longer ignored and are now correctly appended when running a local reconstruction.
- Local reconstruction processes are now also updated in the reconstruction jobs list.
- The reconstruction jobs list command line is now Read Only.
- Clicking Cancel now cancels the running reconstruction process.
- The reconstruction scripts will now run with TomoPy v1.x.x, however the output is not tested with versions newer than 0.10.x.
- Selecting the Center of Rotation, Area for Normalsation and Region of Interest will now always follow the exact position of the mouse.
- Multiple success/warning/error messages will no longer be shown after an operation. 

Work done on imaging has received funding from the Horizon 2020 Framework 
Programme of the European Union under the SINE2020 project Grant No 654000.


`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`__
