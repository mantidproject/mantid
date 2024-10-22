.. _05_fitting_exercises:

=========
Exercises
=========


Exercise 1
==========


In this exercise we will fit a simple Gaussian on a linear background.

1. Start with loading the data set (GEM38370_Focussed).
2. Plot Spectrum number 4.
3. Zoom into the peak around 5 angstroms.

|GemSinglePeak.png|

4. Click on the Fit Toolbar button to open the fit property browser.
5. Adjust the fitting range, defined by the dashed lines, if needed.
6. Make sure the fitting model is empty, Clear the model if necessary.

|ClearModel.png|

7. Add a background function. Select LinearBackground.

|AddBackgroundOption.png|

8. Change peak type to Gaussian, and add a Gaussian.
9. Click at peak's maximum point to set initial values for the centre
   and the height.
10. Adjust the width.

.. figure:: /images/PreparedToFitGaussian.png
   :alt: PreparedToFitGaussian.png
   :align: center

11. Run Fit.

.. figure:: /images/FittedGaussian.png
   :alt: FittedGaussian.png
   :align: center


Exercise 2
==========


1. Again, ensure that the Fit model is clear by clicking Setup >"Clear model"
2. Load the GEM63437_focussed.nxs data (different to the data in Exercise 1). Note the workspace created is a
   WorkspaceGroup, which has already been processed with
   Mantid. Click on the triangle to reveal the contained workspaces. Show the History to see how many algorithms have been applied to this dataset.
3. Plot the spectrum in *GEM63437_focussed_2* by double-clicking on this workspace, and zoom in on the area of the three peaks
4. Open the Fit Property Browser and set fitting range/StartX and EndX
   to be between approximately 2270 and 5000 microseconds
5. Right click on plot and select "Add background", then
   LinearBackground
6. Change the peak type to IkedaCarpenterPV.

   Remember, this is a peak function where
   some parameters are set based on the relevant instrument
   geometry. This is evident from the starting guess of the peak width
   but also by inspecting this function in the Fit Function panel.

7. Add an IkedaCarpenterPV peak to each of the three peaks, remembering to change the peak width (at least for the first one!)
8. Display > "Plot guess" and what you should see is something similar to

.. figure:: /images/ExerciseFittingMBCguess.png
   :alt: ExerciseFittingMBCguess.png
   :align: center

where the orange line is the guess

9. Remove plot guess
10. Fit the data with the model, where the output should be something similar to:

.. figure:: /images/ExerciseFittingMBCfit.png
   :alt: ExerciseFittingMBCfit.png
   :align: center

where the green line here is the Calculated fit

Optionally using a similar approach try to fit the spectrum in for example *GEM63437_focussed_3*


Exercise 3
==========

#. Load the MUSR00015189 data set, this is a group workspace
#. Click the triangle beside this workspace
#. From the second workspace (MUSR00015189_2), plot spectrum number 64
#. Open the Fit property browser
#. As described earlier, add a UserFunction with the with Formula = `h*exp(-a*x)`
#. Set `h = 5000` and **Fix** it to this value
#. Give `a` some positive intial value such as `1.0`
#. Fit the data

.. figure:: /images/ExerciseFittingMBCfit_3MUSR.png
   :alt: ExerciseFittingMBCfit_3MUSR.png
   :align: center

.. |GemSinglePeak.png| image:: /images/GemSinglePeak.png
   :width: 500px
.. |ClearModel.png| image:: /images/ClearModel.png
   :width: 300px
.. |AddBackgroundOption.png| image:: /images/AddBackgroundOption.png
   :width: 500px
