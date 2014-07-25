.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm ProcessBackground() provides several functions for user to
process background to prepare Le Bail Fit.


There are a few functional options for user to choose.
* SelectBackgroundPoints: selecting background points from diffraction data. Usually the output will be used to fit background;
* RemovePeaks: removing peaks from a given MatrixWorks from diffraction data;


Select Background Points
########################

This feature is designed to select many background points with user's
simple input. User is required to select only a few background points in
the middle of two adjacent peaks. Algorithm will fit these few points
(*BackgroundPoints*) to a background function of specified type.

The purpose of this option is to select as many background data points as possible
for future background function fitting. 

Prior information can be given by two modes.  Property 'SelectionMode' determines which modes to use.  
One (1)is from a list of X-values specified by users via property "BackgroundPoints". 
The other (2) is through a (background) function, whose type is specified by property "BackgroundType" and 
values are given via input table workspace "BackgroundTableWorkspace". 


Select background points from given X-values
============================================

Here is how it works.  Assume that the :math:`X^{(u)}` is the list of x values specified by users. 

* Create a data set (X, Y, E) from input workspace, where :math:`X_i` is the nearest value
  to :math:`X^{(u)}_i`;
* Fit the background function against the data set (X, Y, E);
* Select the data points, which are within a certain range above and below the fitted background function;
* The last step is to fit background function against the selected background points 

Select background points from given function
============================================


In this approach, the difference from the other apporach is to use the user given background function
to select data points within a range other than fitting the background function from given data points in the
other approach. 
Thus, it is just the last step of previous approach. 

Output workspaces
=================

- OutputWorkspace: It contains 3 spectra.  

  - spectrum 0: the selected background data points;
  - spectrum 1: the fitted background function against the selected data points;
  - spectrum 2: the difference of sepctrum 0 and 1

- OutputBackgroundParameterWorkspace: A table workspace containing the fitted parameter values including :math:`\chi^2`. 

- UserBackgroundWorkspace: a MatrixWorkspace to visualize by user. 
  
  - spectrum 0: background function (either given by user or fit from given data points) that is used to select background points;
  - spectrum 1: diffraction data with background (spectrum 0) removed;
  - spectrum 2: upper boundary on data points to be selected for spectrum 1;
  - spectrum 3: lower boundary on data points to be selected for spectrum 1


Algorithm properties
====================

Besides the common algorithm properties, below is the list of properties specific to this function option. 

- Inputs:

  - LowerBoundary
  - UpperBoundary
  - BackgroundType
  - SelectionMode
  - BackgroundOrder and BackgroundPoints / BackgroundTableWorkspace
  - NoiseTolerance
  - NegativeNoiseTolerance
  - OutputBackgroundType
  - OutputBackgroundOrder

- Outputs:
  
  - OutputBackgroundParameterWorkspace
  - UserBackgroundWorkspace


Simple Remove Peaks
###################

This algorithm is to remove peaks and output the backgrounds,
which can be used to fit an artibrary background function after calling this algorithm. 

It is assumed that the all peaks have been fitted reasonably well. 
Then by removing the peaks within range :math:`X_i^{(0)} \pm FWHM`,
and save the rest data points, which are very likely backgrounds, to an output workspace.  

Required and optional algorithm properties
==========================================

Besides the common algorithm properties, below is the list of properties specific to this function option. 

- Inputs: 

  - BraggPeakTableWorkspace
  - NumberOfFWHM

- Outputs:

  - UserBackgroundWorkspace: a dummy output for not raising trouble with python script


Add Region
##########

Replace a region, which is defined by 'LowerBoundary' and 'UpperBoundary', in a workspace
from another reference workspace. 


Required and optional algorithm properties
==========================================

- Inputs

  - LowerBoundary (required)
  - UpperBoundary (required)
  - ReferenceWorkspace (required)




Delete Region
#############

Required and optional algorithm properties
==========================================


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
