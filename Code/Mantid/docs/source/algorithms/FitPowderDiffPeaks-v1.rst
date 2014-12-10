.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm fits a certain set of single diffraction peaks in a powder
diffraction pattern.

The assumption is that all the peaks in the diffraction patter can be described by a single peak type. 
A specific peak parameter will have its values plotted by an analytical function among all the peaks. 

It serves as the first step to fit/refine instrumental parameters that
will be introduced in `Le Bail Fit <Le Bail Fit>`__. The second step is
realized by algorithm RefinePowderInstrumentParameters.

Peak Fitting Algorithms
-----------------------

Peak Fitting Mode
#################

Fitting mode determines the approach (or algorithm) to fit diffraction
peaks.  2 modes are supported:

1. Confident: User is confident on the input peak parameters. Thus the fitting will be a one-step minimizer by Levenberg-Marquardt.
2. Robust: The given starting values of peak parameters may be too far away from true values.  A trial-and-error approach is used. 


Starting Values of Peaks' Parameters
####################################

1. "(HKL) & Calculation": the starting values are calculated from each peak's miller index and thermal neutron peak profile formula;
2. "From Bragg Peak Table": the starting values come from the Bragg Peak Parameter table.

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

Usage
-----

**Example - Fit diffraction peaks by single peak fitting:**

.. testcode:: ExFitSingleDiffPeaks

  # Load reduced powder diffraction data
  LoadAscii(Filename='PG3_11487-3.dat', 
        OutputWorkspace='PG3_11487', Unit='TOF')

  # Create table workspaces used by Le Bail fit algorithms
  CreateLeBailFitInput(ReflectionsFile='LB4854b3.hkl', FullprofParameterFile='2011B_HR60b3.irf', 
        Bank=3, LatticeConstant=4.1568899999999998, InstrumentParameterWorkspace='Bank3InstrumentParameterTable1', 
        BraggPeakParameterWorkspace='BraggPeakParameterTable1')

  # Fit background of the powder diffraction data
  ProcessBackground(InputWorkspace='PG3_11487', OutputWorkspace='PG3_11487_Background', Options='SelectBackgroundPoints', 
        LowerBound=10080, UpperBound=72000, SelectionMode='FitGivenDataPoints', 
        BackgroundPoints='10082,10591,11154,12615,13690,13715,15073,16893,17764,19628,21318,24192,35350,44212,50900,60000,69900,79000', 
        NoiseTolerance=0.10000000000000001, UserBackgroundWorkspace='dummy0', OutputBackgroundParameterWorkspace='dummy1')

  Fit(Function='name=Polynomial,n=6,A0=0.473391,A1=-3.8911e-05,A2=1.7206e-09,A3=-3.21291e-14,A4=9.31264e-20,A5=3.90465e-24,A6=-3.28688e-29', 
        InputWorkspace='PG3_11487_Background', MaxIterations=1000, OutputStatus='success', 
        OutputChi2overDoF=2.0078239589764837, Minimizer='Levenberg-MarquardtMD', CreateOutput=True, 
        Output='PG3_11487_Background', StartX=10080, EndX=72000, 
        OutputNormalisedCovarianceMatrix='PG3_11487_Background_NormalisedCovarianceMatrix',
        OutputParameters='PG3_11487_Background_Parameters', OutputWorkspace='PG3_11487_Background_Workspace', Version=1)

  # Fit individual peaks in the diffraction pattern
  FitPowderDiffPeaks(InputWorkspace='PG3_11487', OutputWorkspace='Bank3FittedPeaks', 
        BraggPeakParameterWorkspace='BraggPeakParameterTable1', 
        InstrumentParameterWorkspace='Bank3InstrumentParameterTable1', 
        OutputBraggPeakParameterWorkspace='BraggPeakParameterTable2_0', 
        OutputBraggPeakParameterDataWorkspace='BraggPeakParameterTable2_P', 
        OutputZscoreWorkspace='BraggPeakParameterTable2_Zscore', 
        MinTOF=16866, MaxTOF=70000, UseGivenPeakCentreTOF=False, MinimumPeakHeight=0.29999999999999999, 
        PeaksCorrelated=True, MinimumHKL='12,12,12', RightMostPeakHKL='1,1,0', RightMostPeakLeftBound=65800, RightMostPeakRightBound=67000)

  # Print result
  resultws = mtd["BraggPeakParameterTable2_P"]
  for i in xrange(10):
      print "Peak @ d = %.5f, TOF_0 = %.5f, A = %.5f, B = %.5f, Sigma = %.5f" % (resultws.readX(0)[i], 
          resultws.readY(0)[i], resultws.readY(1)[i], resultws.readY(2)[i], resultws.readY(3)[i])

.. testcleanup:: ExFitSingleDiffPeaks

  DeleteWorkspace(Workspace="Bank3FittedPeaks")
  DeleteWorkspace(Workspace="Bank3InstrumentParameterTable1")
  DeleteWorkspace(Workspace="BraggPeakParameterTable1")
  DeleteWorkspace(Workspace="BraggPeakParameterTable2_0")
  DeleteWorkspace(Workspace="BraggPeakParameterTable2_P")
  DeleteWorkspace(Workspace="BraggPeakParameterTable2_Zscore")
  DeleteWorkspace(Workspace="PG3_11487")
  DeleteWorkspace(Workspace="PG3_11487_Background")
  DeleteWorkspace(Workspace="PG3_11487_Background_NormalisedCovarianceMatrix")
  DeleteWorkspace(Workspace="PG3_11487_Background_Parameters")
  DeleteWorkspace(Workspace="PG3_11487_Background_Workspace")
  DeleteWorkspace(Workspace="dummy0")
  DeleteWorkspace(Workspace="dummy1")

Output:

.. testoutput:: ExFitSingleDiffPeaks

  GeneraateHKL? =  False
  Peak @ d = 0.75894, TOF_0 = 17142.91137, A = 0.16167, B = 0.11573, Sigma = 6.86825
  Peak @ d = 0.77192, TOF_0 = 17435.14732, A = 0.17094, B = 0.10819, Sigma = 6.86444
  Peak @ d = 0.79999, TOF_0 = 18069.76185, A = 0.13567, B = 0.09103, Sigma = 6.13250
  Peak @ d = 0.81523, TOF_0 = 18414.02584, A = 0.14392, B = 0.09840, Sigma = 6.91726
  Peak @ d = 0.83138, TOF_0 = 18778.52418, A = 0.12731, B = 0.08893, Sigma = 6.25243
  Peak @ d = 0.84852, TOF_0 = 19165.40416, A = 0.16111, B = 0.10175, Sigma = 7.82205
  Peak @ d = 0.88625, TOF_0 = 20017.84221, A = 0.12438, B = 0.08556, Sigma = 7.33640
  Peak @ d = 0.90711, TOF_0 = 20488.90788, A = 0.11708, B = 0.07951, Sigma = 7.13853
  Peak @ d = 0.92951, TOF_0 = 20994.50960, A = 0.14109, B = 0.08963, Sigma = 8.87438
  Peak @ d = 0.95366, TOF_0 = 21540.56129, A = 0.12770, B = 0.08669, Sigma = 8.26518

.. categories::
