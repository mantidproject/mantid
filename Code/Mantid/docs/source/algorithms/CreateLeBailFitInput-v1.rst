.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to import Fullprof .irf file (peak parameters) and
.hkl file (reflections) and record the information to TableWorkspaces,
which serve as the inputs for algorithm LeBailFit.

Format of Instrument parameter TableWorkspace
#############################################

Instrument parameter TableWorkspace contains all the peak profile
parameters imported from Fullprof .irf file.

Presently these are the peak profiles supported

``* Thermal neutron back to back exponential convoluted with pseudo-voigt (profile No. 10 in Fullprof)``

Each row in TableWorkspace corresponds to one profile parameter.

Columns include Name, Value, FitOrTie, Min, Max and StepSize.

Format of reflection TableWorkspace
###################################

Each row of this workspace corresponds to one diffraction peak. The
information contains the peak's Miller index and (local) peak profile
parameters of this peak. For instance of a back-to-back exponential
convoluted with Gaussian peak, the peak profile parameters include
Alpha, Beta, Sigma, centre and height.

How to use algorithm with other algorithms
------------------------------------------

This algorithm is designed to work with other algorithms to do Le Bail
fit. The introduction can be found in the wiki page of `Le Bail
Fit <Le Bail Fit>`__.

Usage
-----

**Example - create inputs for LeBail fit of Pg3:**

.. testcode:: ExCreateLBInputs

  CreateLeBailFitInput(ReflectionsFile=r'LB4854b3.hkl',		
	FullprofParameterFile=r'2013A_HR60b3.irf',	
	Bank='3',
	LatticeConstant='4.1568899999999998',		
	InstrumentParameterWorkspace='PG3_Bank3_ParTable',	
	BraggPeakParameterWorkspace='LaB6_HKL_Table')	

  # Examine 
  partablews = mtd["PG3_Bank3_ParTable"]
  braggtablews = mtd["LaB6_HKL_Table"]
  print "Number Bragg peaks from .hkl file is %d.  Number of peak profile parameters is %d." % (braggtablews.rowCount(), partablews.rowCount())


.. testcleanup:: ExCreateLBInputs

  DeleteWorkspace(partablews)
  DeleteWorkspace(braggtablews)


Output:

.. testoutput:: ExCreateLBInputs

  GeneraateHKL? =  False
  Number Bragg peaks from .hkl file is 76.  Number of peak profile parameters is 30.


.. categories::
