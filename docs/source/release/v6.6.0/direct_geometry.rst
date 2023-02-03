=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

General
-------

New features
############
- The FunctionBrowser in ALFView has been replaced with a box for entering the PeakCentre parameter.
- By default on ALFView, the mini plot on the pick tab will now select the 'Out of plane angle' axis and 'Integrate' when the ``Select whole tube`` button is pressed.
- Added :ref:`LagrangeILLReduction <algm-LagrangeILLReduction>`, which handles loading and data reduction from IN1 - Lagrange at ILL.
- The ability to load an optional vanadium run into ALFView for normalisation is now possible. This vanadium run is saved between sessions.
- The tools in the PickTab of ALFView which are not useful for the main workflow have been hidden.
- The 'Select whole tube' tool on the Pick tab can now be used to select individual tubes.
- The 'Draw a rectangle' tool on the Pick tab can now be used to select one or more neighbouring tubes.
- The 'Edit a shape' tool on the Pick tab can now be used to select, move, resize and delete highlighted tubes.
- The 'Update Estimate' button has been removed in ALFView. An estimate value for the peak centre is automatically re-calculated whenever the tube selection changes.
- An option to export the right hand side plot to a workspace has been added (top right).
- An option to open the right hand side plot in a new window to allow more plotting customizations has been added (top right).
- An option to reset the extracted data in the right hand side plot has been added (top right).
- The average two theta value is now displayed in ALFView when a tube is selected (bottom right).
- The number of selected tubes is now displayed in ALFView when the tube selection is changed (bottom right).
- The FunctionBrowser widget has been replaced with a box to specify the peak centre of a Flat Background + Gaussian in ALFView.
- The 'Fit' button will now trigger the calculation of the Rotation angle, if the Fit is successful.
- Updated the documentation for the ALFView interface.
- :ref:`LagrangeILLReduction <algm-LagrangeILLReduction>` now allows the user to select normalisation approach, loads time and temperature metadata from the ASCII files, and properly handles interpolation and energy range of the water correction depending on the user's choice of `UseIncidentEnergy` property value.

Bugfixes
############
- Corrected a repeated call to the loader while merging runs in the :ref:`PelicanReduction <algm-PelicanReduction>` algorithm.
- A crash is now avoided when extracting a tube in ALFView while an invalid axis is selected.
- The plotting panel on the ALFView pick tab is now expanded by default when first opening the interface.
- It is now possible to delete the workspace loaded into ALFView and then load in a new dataset without issue


CrystalField
-------------

New features
############


Bugfixes
############
- Fixed transcription error in Table 3 of :ref:`Crystal Field Theory` documentation


MSlice
------

New features
############


Bugfixes
############



:ref:`Release 6.6.0 <v6.6.0>`