=====================
Imaging Changes
=====================

.. contents:: Table of Contents
   :local:

Tomographic reconstruction graphical user interface
###################################################

- Running local reconstructions is now possible
- You can now use Auto-find button in the ROI tab to calculate the Center of Rotation for a sample using TomoPy. The calculated Center of Rotation is automatically loaded back into the GUI. Both the Center of Rotation relative to the crop and the full image are printed
- Tomography's python reconstruciton algorithms now uses 50% less memory, due to not copying the data when rotating

Bug Fixes
---------
- The external interpreter and scripts paths are no longer ignored and are now correctly appended when running a local reconstruction
- Selecting the Center of Rotation, Area for Normalsation and Region of Interest will now always follow the exact position of the mouse
- The coordinates for the Region of Interest and Area for Normalsation will now always be accurate to the picture
- Multiple success/warning/error messages will no longer be shown after an operation. 
- Fixed the reconstruction script sometimes not cropping the picture correctly

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`