.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to create input workspaces to ConvertCWSDExpToMomentum for 
HB3A (four-circle single crystal diffractometer in HFIR). 


Inputs
======


OutputWorkspaces
================

Two TableWorkspaces, which contain experiment information, are returned. 

**InputWorkspace** is a TableWorkspace containing the data files names to be loaded for the experiment. 
It is required to have 4 columns.  
They are *Scan*, *Pt*, *Filename* and *StartDetID* respectively. 

A typical HB3A experiment consists of multiple scans, each of which contains multiple measurement point (i.e., Pt). 
*FileName* is the XML data file for 2D detector information for a specific Scan/Pt pair. 
*StartDetID* is the starting detector ID of a specific Scan/Pt mapped to the virtual instrument. 

**DetectorTableWorkspace** is a TableWorkspace that list the parameters of all detector pixels belonged 
to the virtual instrument. 
The parameters include detector ID in virtual instrument, detector's position in cartesian coordinate,
and detector's original detector ID. 




How to use algorithm with other algorithms
------------------------------------------

Algorithm *CollectHB3AExperimentInfo* is designed to provide input information for algorithm
*ConvertCWSDExpToMomentum*, whose output will be used to calculated UB matrix and integrate
single cystal peaks. 


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
