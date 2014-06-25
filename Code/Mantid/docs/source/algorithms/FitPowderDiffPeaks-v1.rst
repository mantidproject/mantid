.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm fits a certain set of single peaks in a powder
diffraction pattern.

It serves as the first step to fit/refine instrumental parameters that
will be introduced in `Le Bail Fit <Le Bail Fit>`__. The second step is
realized by algorithm RefinePowderInstrumentParameters.

Peak Fitting Algorithms
-----------------------

Peak Fitting Mode
#################

Fitting mode determines the approach (or algorithm) to fit diffraction
peaks.

1. Robust

2. Confident: User is confident on the input peak parameters. Thus the
fitting will be a one-step minimizer by Levenberg-Marquardt.

Starting Values of Peaks' Parameters
####################################

1. "(HKL) & Calculation": the starting values are calculated from each
peak's miller index and thermal neutron peak profile formula;

2. "From Bragg Peak Table": the starting values come from the Bragg Peak
Parameter table.

Peak-fitting sequence
#####################

Peaks are fitted from high d-spacing, i.e., lowest possible Miller
index, to low d-spacing values. If MinimumHKL is specified, then peak
will be fitted from maximum d-spacing/TOF, to the peak with Miller index
as MinimumHKL.

Correlated peak profile parameters
##################################

If peaks profile parameters are correlated by analytical functions, then
the starting values of one peak will be the fitted peak profile
parameters of its right neighbour.

Use Cases
---------

Several use cases are listed below about how to use this algorithm.

Use case 1: robust fitting
##########################

#. User wants to use the starting values of peaks parameters from input thermal neutron peak parameters such as Alph0, Alph1, and etc;
#. User specifies the right most peak range and its Miller index;
#. "FitPowderDiffPeaks" calculates Alpha, Beta and Sigma for each peak from parameter values from InstrumentParameterTable;
#. "FitPowderDiffPeaks" fits peak parameters of each peak from high TOF to low TOF.


Use Case 2: Fitting Peak Parameters From Scratch
################################################

This is the extreme case such that

#. Input instrumental geometry parameters, including Dtt1, Dtt1t, Dtt2t, Zero, Zerot, Tcross and Width, have roughly-guessed values;
#. There is no pre-knowledge for each peak's peak parameters, including Alpha, Beta, and Sigma.

How to use algorithm with other algorithms
------------------------------------------

This algorithm is designed to work with other algorithms to do Le Bail
fit. The introduction can be found in the wiki page of
:ref:`algm-LeBailFit`.

Example of Working With Other Algorithms
########################################

*FitPowderDiffPeaks* is designed to work with other algorithms, such
*RefinePowderInstrumentParameters*, and *LeBailFit*. See `Le Bail
Fit <http://www.mantidproject.org/Le_Bail_Fit>`_ for full list of such algorithms.

A common scenario is that the starting values of instrumental geometry
related parameters (Dtt1, Dtt1t, and etc) are enough far from the real
values.


#. "FitPowderDiffPeaks" fits the single peaks from high TOF region in robust mode;
#. "RefinePowderInstrumentParameters" refines the instrumental geometry related parameters by using the d-TOF function;
#. Repeat step 1 and 2 for  more single peaks incrementally. The predicted peak positions are more accurate in this step.


.. categories::
