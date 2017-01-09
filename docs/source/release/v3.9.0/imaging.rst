<<<<<<< HEAD
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
||||||| merged common ancestors
<<<<<<< Temporary merge branch 1
=====================
Imaging Changes
=====================

.. contents:: Table of Contents
   :local:

Tomographic reconstruction graphical user interface
###################################################

- Running local reconstructions is now possible

Bug Fixes
---------
- The external interpreter and scripts paths are no longer ignored and are now correctly appended when running a local reconstruction

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`__
||||||| merged common ancestors
=======
=====================
Imaging Changes
=====================

.. contents:: Table of Contents
   :local:

Tomographic reconstruction graphical user interface
###################################################

- Running local reconstructions is now possible

Bug Fixes
---------
=======
=====================
Imaging Changes
=====================

.. contents:: Table of Contents
   :local:

Tomographic reconstruction graphical user interface
###################################################

- Running local reconstructions is now possible
- You can now use Auto-find button in the ROI tab to calculate the Center of Rotation for a sample using TomoPy. The calculated Center of Rotation is automatically loaded back into the GUI. Both the Center of Rotation relative to the crop and the full image are printed
- Tomography's python reconstruciton algorithms now uses 50% less memory when rotating the image, however the rotation only works with square images
- Reconstruction algorithms are a lot more verbose now, reporting on each step, for easy tracking of progress. Additionally they print how much time was spent in each step.
- Automatic finding of the Center of Rotation using TomoPy now works locally. Remote submission for finding the Center of Rotation can be done via the Custom Command tool.
- The calculated Center of Rotation is automatically read from the TomoPy output and loaded into the Graphical Interface, also placing the Center of Rotation indicator at the correct place

Bug Fixes
---------
<<<<<<< HEAD
>>>>>>> origin/17995_auto_find_cor
||||||| merged common ancestors
- The external interpreter and scripts paths are no longer ignored and are now correctly appended when running a local reconstruction
- Local reconstruction processes are now also updated in the reconstruction jobs list
- The reconstruction jobs list command line is now Read Only
- Clicking Cancel now cancels the running reconstruction process
- The reconstruction scripts will now work with TomoPy v1.x.x, however the results have yet to be tested
>>>>>>> Temporary merge branch 2
=======
- The external interpreter and scripts paths are no longer ignored and are now correctly appended when running a local reconstruction
- Local reconstruction processes are now also updated in the reconstruction jobs list
- The reconstruction jobs list command line is now Read Only
- Clicking Cancel now cancels the running reconstruction process
- The reconstruction scripts will now work with TomoPy v1.x.x, however the results have yet to be tested
>>>>>>> master
>>>>>>> 17995_auto_find_cor
- Selecting the Center of Rotation, Area for Normalsation and Region of Interest will now always follow the exact position of the mouse
- The coordinates for the Region of Interest and Area for Normalsation will now always be accurate to the picture
- Multiple success/warning/error messages will no longer be shown after an operation. 
<<<<<<< HEAD
<<<<<<< HEAD
||||||| merged common ancestors
<<<<<<< Temporary merge branch 1
- Fixed the reconstruction script sometimes not cropping the picture correctly

=======
<<<<<<< HEAD
- Fixed the reconstruction script sometimes not cropping the picture correctly

>>>>>>> 17995_auto_find_cor
||||||| merged common ancestors

=======
<<<<<<< HEAD
- Fixed the reconstruction script sometimes not cropping the picture correctly

>>>>>>> origin/17995_auto_find_cor
||||||| merged common ancestors
>>>>>>> Temporary merge branch 2
=======
>>>>>>> master
>>>>>>> 17995_auto_find_cor
|

<<<<<<< HEAD
<<<<<<< HEAD
`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`__
||||||| merged common ancestors
<<<<<<< Temporary merge branch 1
`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`
=======
<<<<<<< HEAD
`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`
>>>>>>> 17995_auto_find_cor
||||||| merged common ancestors
`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`__
>>>>>>> Temporary merge branch 2
=======
<<<<<<< HEAD
`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`
>>>>>>> origin/17995_auto_find_cor
||||||| merged common ancestors
`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`__
>>>>>>> Temporary merge branch 2
=======
`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`__
>>>>>>> master
>>>>>>> 17995_auto_find_cor
