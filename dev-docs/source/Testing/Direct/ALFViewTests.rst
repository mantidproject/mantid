.. _alfview_testing:

ALFView GUI Testing
===================


Introduction
------------

The ALFView interface is a graphical front end used by the ISIS Excitations group for crystal alignment. The ALF instrument requires frequent manual adjustments after each (< 1 minute) run to realign the single crystal(s). The purpose of this interface is to calculate the rotation angle required for this adjustment.

Set up
------

- These instructions assume use of MantidWorkbench 7.0 or higher.
- Ensure you have the ISIS facility and ALF instrument selected in your settings.
- Ensure you have the ISIS archive enabled.
- Open the ``Direct->ALFView`` interface.

Loading Runs
------------

- Type 82301 in the Sample box, press the enter key and wait for it to load.
- The instrument view embedded in the interface should be populated with data.
- Type 82301 (the same run) into the Vanadium box, press the enter key and wait for it to load.
- The instrument view should have a flat color because we have normalised (divided) by the same dataset.
- Remove the Vanadium run number.
- The instrument view should look how it did before loading the Vanadium Run.

Rebinning the Data
------------------

- Type 5.5,0.01,6 into the Rebin parametersbox. Then click Rebin.
- The instrument view data should change to a darker color.

Selecting tubes
---------------

- Left click on the central tube (the one with the yellow square) in the instrument view.
- The entire tube should be highlighted, indicating it is selected.
- The plot on the right-hand side should update to show the selected data.
- The Two theta value on the right-hand side should also update.
- Press the Add ROI button.
- Left click to drag and resize rectangular shape to select some more tubes.
- This tool should select all tubes that it overlaps with.
- Both plots should be updated on every mouse release.
- Click on Add ROI again to deactivate rectangular shape.
- Left mouse click on selected tubes to unselect them.
- The plot on the right-hand side should change each time the selected tubes change.

Calculating the Rotation angle
------------------------------

- Ensure that only the central two tubes on the instrument view are selected (the tube with the bright yellow square, and the one to its left).
- The number of tubes on the right-hand side should reflect the number of selected tubes i.e. 2 tubes.
- The Two theta value should read 40.4584.
- Click the Fit button.
- The peak centre should read approximately 0.877092.
- A red line labelled Fitted Data should appear on the plot above.
- The rotation angle should read 1.26829.

Clear when new load happens
---------------------------

- Type 82301 into the Vanadium box, press the enter key and wait for it to load.
- The Two theta value should stay the same 40.4584.
- The Rotation angle should be cleared.
- The red line labelled Fitted Data should disappear.
- The black line labelled Extracted Data should be a flat line due to us normalising by the same dataset.

Check Vanadium run is saved between sessions
--------------------------------------------

- Ensure the Vanadium run is still 82301.
- Close the ALFView interface.
- Close Mantid.
- Re-open Mantid.
- Re-open the ALFView interface.
- The Vanadium run should be automatically populated in the GUI.
