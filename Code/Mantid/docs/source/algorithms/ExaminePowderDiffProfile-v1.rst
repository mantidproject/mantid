.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to examine peak profile values for powder diffractometry by LeBailFit.

It is a workflow algorithm including loading files (including data file, profile parameter file and reflections list file),
processing background and calculating peaks by Le Bail algorithm. 


Usage
-----

**Example - Calcualte peaks from input table workspaces**

.. testcode:: ExExaminePG3Profile

  # Generate inputs
  LoadAscii(Filename='PG3_15035-3.dat', OutputWorkspace='PG3_15035-3', Unit='TOF')

  ProcessBackground(InputWorkspace='PG3_15035-3', 
        OutputWorkspace='PG3_15035-3_BkgdPts', 
        Options='SelectBackgroundPoints', 
        LowerBound=9726, 
        UpperBound=79000, 
        SelectionMode='FitGivenDataPoints', 
        BackgroundPoints='10082,10591,11154,12615,13690,13715,15073,16893,17764,19628,21318,24192,35350, 44212,50900,60000,69900,79000', 
        UserBackgroundWorkspace='Bank3Background', 
        OutputBackgroundParameterWorkspace='Bank3BackgroundParamsTable')

  CreateLeBailFitInput(FullprofParameterFile='2013A_HR60b3.irf', 
        ReflectionsFile='LB4854b3.hkl',
	Bank=3, LatticeConstant=4.156890, 
        InstrumentParameterWorkspace='PG3_Bank3_ParTable',
	BraggPeakParameterWorkspace='LaB6_HKL_Table')

  # Examine profile quality
  ExaminePowderDiffProfile(InputWorkspace='PG3_15035-3', StartX=20000, EndX=100000, 
        ProfileWorkspace='PG3_Bank3_ParTable', BraggPeakWorkspace='LaB6_HKL_Table', 
        BackgroundParameterWorkspace='Bank3BackgroundParamsTable', OutputWorkspace='PG3_15035B3_Cal')

  # Output result
  ws = mtd['PG3_15035B3_Cal']
  print 'Output workspace has %d spectra' % (ws.getNumberHistograms())

.. testcleanup:: ExExaminePG3Profile

  DeleteWorkspace(Workspace='PG3_15035B3_Cal')
  DeleteWorksapce(Workspace='PG3_15035-3')


Output:

.. testoutput:: ExExaminePG3Profile

  Output workspace has 6 spectra

.. categories::
