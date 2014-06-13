.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm ProcessBackground() provides several functions for user to
process background to prepare Le Bail Fit.

Simple Remove Peaks
###################

This algorithm is designed for refining the background based on the
assumption that the all peaks have been fitted reasonably well. Then by
removing the peaks by function 'X0 +/- n\*FWHM', the rest data points
are background in a very high probability.

An arbitrary background function can be fitted against this background
by standard optimizer.

Automatic Background Points Selection
#####################################

This feature is designed to select many background points with user's
simple input. User is required to select only a few background points in
the middle of two adjacent peaks. Algorithm will fit these few points
(*BackgroundPoints*) to a background function of specified type.

Examples
--------

Selecting background
####################

Here is a good example to select background points from a powder
diffraction pattern by calling ProcessBackground() in a self-consistent
manner.

| ``1. Select a set of background points (X values only), which can roughly describes the background, manually;``
| ``2. Call ProcessBackground with Option='SelectBackgroundPoints' and SelectionMode='UserSpecifyBackground'.``
| ``   A good choice for background function to enter is 6-th order polynomial;``
| ``3. Plot spectra 2 to 4 in UserBackgroundWorkspace to check whether 'Tolerance' is proper or not.``
| ``   If not then reset the tolerance and run ProcessBackground again with previous setup;``
| ``4. Fit OutputWorkspace (the selected background) with a background function;``
| ``5. Call ProcessBackground with Option='SelectBackgroundPoints' and SelectionMode='UserFunction'.``
| ``   Set the background parameter workspace as the output parameter table workspace obtained in the last step;``
| ``6. Repeat step 4 and 5 for a few times until the background plot by fitted background function``
| ``   from selected background points is close enough to real background.``

.. categories::
