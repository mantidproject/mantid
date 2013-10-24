''' SVN Info:  	The variables below will only get subsituted at svn checkout if
		the repository is configured for variable subsitution. 

	$Id$
	$HeadURL$
|=============================================================================|=======|	
1                                                                            80   <tab>
'''

import math
from mantid.simpleapi import *  # New API

def l2q(ws,whichDet,theta):
	'''	
	call signature::call signature::

	q=l2q(ws,theta)
	
	Convert from lambda to qz.

	ToDo:
		1) This is currently a point detector only function. This will either need 
		to be expanded or a new one written for the multidetector.
	'''
	''' Notes for developers:


	   SVN Info:  	The variables below will only get subsituted at svn checkout if the repository is
	   		configured for variable subsitution. 

	   		$Id$
			$HeadURL$
	'''
	#ws = mantid.getMatrixWorkspace(ws)
	
	# pick up the sample to detector distance
	from mantid.api import WorkspaceGroup
	if isinstance(ws, WorkspaceGroup):
		wsg = ws.getName() + '_1'
		inst = mtd[wsg].getInstrument()
	else:
		inst = ws.getInstrument()
#	inst=ws.getInstrument()
	sampleLocation=inst.getComponentByName('some-surface-holder').getPos()
	detLocation=inst.getComponentByName(whichDet).getPos()
	sample2detector=detLocation-sampleLocation    # meters

	theta=theta*math.pi/180.0 	# convert to raidains

	# calculate new detector position based on angle theta in degrees:
	x=0.0							# Across the beam	(side to side)
	y=sample2detector.getZ()*math.sin(2.0*theta)		# Normal to the beam 	(up)	
	#z=sample2detector.getZ()*(math.cos(2.0*theta)-1.0)+detLocation.getZ() 	# Along the beam
	z=detLocation.getZ()
	print x, y, z
	print whichDet
	# Move the detector ianto the correct spot
	MoveInstrumentComponent(ws,ComponentName=whichDet,X=x,Y=y,Z=z,RelativePosition=False)
			
	# Now convert to momentum transfer
	ConvertUnits(InputWorkspace=ws,OutputWorkspace="IvsQ",Target="MomentumTransfer")
	return mtd["IvsQ"]


def _testL2q():
	r3015 = quick("C:/Documents and Settings/polref_mgr/Desktop/MantidTesting/raw/POLREF00003015.raw")
	ws = l2q(r3015,2.0)
	return True

def  _doAllTests():
	_testL2q()
	return True
if __name__ == '__main__':
	''' This is the debugging and testing area of the file.  The code below is run when ever the 
	   script is called directly from a shell command line or the execute all menu option in mantid. 
	'''
	Debugging = True  # Turn the debugging on and the testing code off
	#Debugging = False # Turn the debugging off and the testing on
	
	if Debugging == False:
		_doAllTests()  
	else:    #Debugging code goes below
		pass

