#from ReflectometerCors import *
from l2q import *
from mantid.simpleapi import *
#from mantid.simpleapi import *  # New API
#from mantidplot import *
from PyQt4 import QtCore, uic
import math
from mantid.api import WorkspaceGroup

def combineDataMulti(wksp_list,output_wksp,beg_overlap,end_overlap,Qmin,Qmax,binning,scale_high=1,scale_factor=-1.0,which_period=1,keep=0):
	"""
	Function stitches multiple workspaces together. Workspaces should have an X-axis in mod Q, and the Spectrum axis as I/I0
	
	wksp_list: A list of workspaces to stitch together
	ouput_wksp: output workspace name
	beg_overlap: The beginning of the overlap region. List argument, each entry matches the entry in the wksp_list
	end_overlap: The end of the overlap region. List argument, each entry matches the entry in the wksp_list
	Qmin: Q minimum of the final output workspace
	Qmax: Q maximum of the input workspace
	which_period: Which period to use if multiperiod workspaces are provided.
	keep=1: keep individual workspaces in Mantid, otherwise delete wksp_list
	"""

	# check if overlaps have correct number of entries
	defaultoverlaps = False
	if type(beg_overlap) != list:
		beg_overlap = [beg_overlap]
	if type(end_overlap) != list:
		end_overlap = [end_overlap]
	if len(wksp_list) != len(beg_overlap):
		print "Using default values!"
		defaultoverlaps = True
		
    #copy first workspace into temporary wksp 'currentSum'
	currentSum = mtd[wksp_list[0]]
	print "Length: ",len(wksp_list), wksp_list
	
	for i in range(0,len(wksp_list)-1):
		w1=currentSum
		w2=getWorkspace(wksp_list[i+1])
		if defaultoverlaps:
			overlapLow = w2.readX(0)[0]
			overlapHigh = 0.5*max(w1.readX(0))
		else:
			overlapLow = beg_overlap[i+1]
			overlapHigh = end_overlap[i]
			
        #check if multi-period
		if isinstance(currentSum, WorkspaceGroup):
			raise RuntimeError("combineMulti, does not support multi-period input workspaces")
		else:
			print "Iteration",i
			currentSum, scale_factor = stitch2(currentSum, mtd[wksp_list[i+1]], currentSum.name(), overlapLow, overlapHigh, Qmin, Qmax, binning, scale_high)
	RenameWorkspace(InputWorkspace=currentSum.name(),OutputWorkspace=output_wksp)
	
	# Remove any existing workspaces from the workspace list.
	if not keep:
		names = mtd.getObjectNames()
		for ws in wksp_list:
			candidate = ws
			if candidate in names: 
				DeleteWorkspace(candidate)
				
	return mtd[output_wksp]

def stitch2(ws1, ws2, output_ws_name, begoverlap,endoverlap,Qmin,Qmax,binning,scalehigh=True,scalefactor=-1.0):
	"""
	Function stitches two workspaces together and returns a stitched workspace along with the scale factor
	
	ws1: First workspace to stitch
	ws2: Second workspace to stitch
	output_ws_name: The name to give the outputworkspace
	begoverlap: The beginning of the overlap region
	endoverlap: The end of the overlap region
	Qmin: Final minimum Q in the Q range
	Qmax: Final maximum Q in the Q range
	binning: Binning stride to use
	scalehigh: if True, scale ws2, otherwise scale ws1
	scalefactor: Use the manual scaling factor provided if > 0
	"""
	out = combine2(ws1.name(),ws2.name(),output_ws_name,begoverlap,endoverlap,Qmin,Qmax,binning,scalehigh,scalefactor)
	return mtd[output_ws_name], out[1]
	

def combine2(wksp1,wksp2,outputwksp,begoverlap,endoverlap,Qmin,Qmax,binning,scalehigh=True,scalefactor=-1.0):
    """
	Function stitches two workspaces together and returns a stitched workspace name along with the scale factor
	
	wksp1: First workspace name to stitch
	wksp2: Second workspace name to stitch
	outputwksp: The name to give the outputworkspace
	begoverlap: The beginning of the overlap region
	endoverlap: The end of the overlap region
	Qmin: Final minimum Q in the Q range
	Qmax: Final maximum Q in the Q range
	binning: Binning stride to use
	scalehigh: if True, scale ws2, otherwise scale ws1
	scalefactor: Use the manual scaling factor provided if > 0
	"""
    
    import numpy as np
    print "OVERLAPS:", begoverlap, endoverlap
    w1 = Rebin(InputWorkspace=wksp1, Params=str(Qmin)+","+str(binning)+","+str(Qmax))
    w2 = RebinToWorkspace(WorkspaceToRebin=wksp2,WorkspaceToMatch=w1)
    nzind=np.nonzero(w1.dataY(0))[0]
    w2.dataY(0)[int(nzind[0])]=0.0	#set  edge of zeropadding to zero
    w2 = Rebin(InputWorkspace=wksp2, Params=str(Qmin)+","+str(binning)+","+str(Qmax))

    # find the bin numbers to avoid gaps
    a1=w1.binIndexOf(begoverlap)
    a2=w1.binIndexOf(endoverlap)
    wksp1len=len(w1.dataX(0))
    wksp2len=len(w1.dataX(0))

    if scalefactor <= 0.0:
        w1_overlap_int = Integration(InputWorkspace=w1, RangeLower=str(begoverlap), RangeUpper=str(endoverlap))
        w2_overlap_int = Integration(InputWorkspace=w2, RangeLower=str(begoverlap), RangeUpper=str(endoverlap))
        if scalehigh:
            w2 *= (w1_overlap_int/w2_overlap_int)
        else:
            w1 *= (w2_overlap_int/w1_overlap_int)
        
        y1=w1_overlap_int.readY(0)
        y2=w2_overlap_int.readY(0)
        scalefactor = y1[0]/y2[0]
        print "scale factor="+str(scalefactor)
    else:
        w2 = MultiplyRange(InputWorkspace=w2, StartBin=0, EndBin=wksp2len-2, Factor=scalefactor)
        
    # Mask out everything BUT the overlap region as a new workspace.
    overlap1 = MultiplyRange(InputWorkspace=w1, StartBin=0,EndBin=a1-1,Factor=0)
    overlap1 = MultiplyRange(InputWorkspace=overlap1,StartBin=a2,EndBin=wksp1len-2,Factor=0)
	
	# Mask out everything BUT the overlap region as a new workspace.
    overlap2 = MultiplyRange(InputWorkspace=w2,StartBin=0,EndBin=a1,Factor=0)#-1
    overlap2 = MultiplyRange(InputWorkspace=overlap2,StartBin=a2+1,EndBin=wksp1len-2,Factor=0)
	
	# Mask out everything AFTER the start of the overlap region
    w1=MultiplyRange(InputWorkspace=w1,StartBin=a1, EndBin=wksp1len-2,Factor=0)
    # Mask out everything BEFORE the end of the overlap region
    w2=MultiplyRange(InputWorkspace=w2,StartBin=0, EndBin=a2,Factor=0)
    
    # Calculate a weighted mean for the overlap region
    overlapave = WeightedMean(InputWorkspace1=overlap1,InputWorkspace2=overlap2)
    # Add the Three masked workspaces together to create a complete x-range
    result = w1 + overlapave + w2
    result = RenameWorkspace(InputWorkspace=result, OutputWorkspace=outputwksp)    
    
    # Cleanup
    DeleteWorkspace(w1)
    DeleteWorkspace(w2)
    DeleteWorkspace(w1_overlap_int)
    DeleteWorkspace(w2_overlap_int)
    DeleteWorkspace(overlap1)
    DeleteWorkspace(overlap2)
    DeleteWorkspace(overlapave)
    return result.name(), scalefactor

		
def getWorkspace(wksp):
	"""
	Get the workspace if it is not a group workspace. If it is a group workspace, get the first period.
	"""
	if isinstance(mtd[wksp], WorkspaceGroup):
		wout = mtd[wksp+'_1']
	else:
		wout = mtd[wksp]
	return wout

def groupGet(wksp,whattoget,field=''):
	'''
	Auxiliary function.
	returns information about instrument or sample details for a given workspace wksp,
	also if the workspace is a group (info from first group element)
	'''
	if (whattoget == 'inst'):
		if isinstance(mtd[wksp], WorkspaceGroup):
			return mtd[wksp+'_1'].getInstrument()
		else:
			return mtd[wksp].getInstrument()
			
	elif (whattoget == 'samp' and field != ''):
		if isinstance(mtd[wksp], WorkspaceGroup):
			try:
				res = mtd[wksp + '_1'].getRun().getLogData(field).value				
			except RuntimeError:
				res = 0
				print "Block "+field+" not found."			
		else:
			try:
				res = mtd[wksp].getRun().getLogData(field).value
			except RuntimeError:		
				res = 0
				print "Block "+field+" not found."
		return res
	elif (whattoget == 'wksp'):
		if isinstance(mtd[wksp], WorkspaceGroup):
			return mtd[wksp+'_1'].getNumberHistograms()
		else:
			return mtd[wksp].getNumberHistograms()

	
