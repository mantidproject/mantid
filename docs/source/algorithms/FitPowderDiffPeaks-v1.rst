.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm fits a certain set of single diffraction peaks in a powder
diffraction pattern.

The assumption is that all the peaks in the diffraction patter can be described by a single peak type.
A specific peak parameter will have its values plotted by an analytical function among all the peaks.

It serves as the first step to fit/refine instrumental parameters that
will be introduced in :ref:`Le Bail fit <Le Bail Fit>`. The second step is
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
*RefinePowderInstrumentParameters*, and *LeBailFit*. See :ref:`Le Bail
*fit concept page <Le Bail Fit>` for full list of such algorithms.

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
  for i in range(10):
      print("Peak @ d = {:.5f}, TOF_0 = {:.5f}, A = {:.5f}, B = {:.5f}, Sigma = {:.5f}".
            format(resultws.readX(0)[i], resultws.readY(0)[i], resultws.readY(1)[i], resultws.readY(2)[i], resultws.readY(3)[i]))

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
    Peak @ d = 0.75894, TOF_0 = 17142.23075, A = 0.15403, B = 0.10084, Sigma = 6.03789
    Peak @ d = 0.77192, TOF_0 = 17435.01857, A = 0.16520, B = 0.10221, Sigma = 6.63462
    Peak @ d = 0.79999, TOF_0 = 18069.21642, A = 0.15039, B = 0.09358, Sigma = 6.41801
    Peak @ d = 0.81523, TOF_0 = 18413.34201, A = 0.14661, B = 0.09132, Sigma = 6.67055
    Peak @ d = 0.83138, TOF_0 = 18778.30857, A = 0.13545, B = 0.09075, Sigma = 6.44981
    Peak @ d = 0.84852, TOF_0 = 19164.95217, A = 0.14572, B = 0.08599, Sigma = 7.30674
    Peak @ d = 0.88625, TOF_0 = 20017.59170, A = 0.12676, B = 0.08361, Sigma = 7.24717
    Peak @ d = 0.90711, TOF_0 = 20488.70341, A = 0.12200, B = 0.08131, Sigma = 7.27715
    Peak @ d = 0.92951, TOF_0 = 20994.85671, A = 0.12909, B = 0.08600, Sigma = 8.46692
    Peak @ d = 0.95366, TOF_0 = 21540.74042, A = 0.10656, B = 0.07592, Sigma = 7.61463

.. categories::

.. sourcelink::
