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
	if type(beg_overlap) != list:
		beg_overlap = [beg_overlap]
	if type(end_overlap) != list:
		end_overlap = [end_overlap]
	if len(wksp_list) != len(beg_overlap):
		print "Using default values!"
		defaultoverlaps = 1
	else:
		defaultoverlaps = 0
		
    #copy first workspace into temporary wksp 'currentSum'
	currentSum = CropWorkspace(InputWorkspace=wksp_list[0])
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
			
        #check if multiperiod
		if isinstance(currentSum, WorkspaceGroup):
			tempwksp,sf = combine2('currentSum_'+str(which_period),wksp_list[i+1]+'_'+str(which_period),'_currentSum',overlapLow,overlapHigh,Qmin,Qmax,binning,scale_high)
            #if (sum(mtd['currentSum_2'].dataY(0))):
			print tempwksp, sf
			
			DeleteWorkspace("_currentSum")
			#CropWorkspace(wksp_list[0],'currentSum')
			print wksp_list
			combine2(currentSum.name(),wksp_list[i+1],'temp',overlapLow,overlapHigh,Qmin,Qmax,binning,scale_high,sf)
			currentSum=CropWorkspace(InputWorkspace='temp')
			DeleteWorkspace("temp")
		else:
			print "Iteration",i
			currentSum, scale_factor = stitch2(currentSum, mtd[wksp_list[i+1]], currentSum.name(), overlapLow, overlapHigh, Qmin, Qmax, binning, scale_high)
	RenameWorkspace(InputWorkspace=currentSum.name(),OutputWorkspace=output_wksp)
	if not keep:
		names = mtd.getObjectNames()
		for ws in wksp_list:
			#print ws.rstrip("_binned")
			candidate = ws
			if candidate in names: 
				DeleteWorkspace(candidate)
			candidate = ws.rstrip("_IvsQ_binned")+"_IvsLam"
			if  candidate in names:
				DeleteWorkspace(candidate)
			candidate = ws.rstrip("_IvsQ_binned")+"_IvsQ"
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
    Rebin(InputWorkspace=wksp1,OutputWorkspace=wksp1+"reb",Params=str(Qmin)+","+str(binning)+","+str(Qmax))
    w1=getWorkspace(wksp1+"reb")
    #nzind=np.nonzero(w1.dataY(0))[0]
    #w1.dataY(0)[int(nzind[0])]=0.0	#set  edge of zeropadding to zero
    
    RebinToWorkspace(WorkspaceToRebin=wksp2,WorkspaceToMatch=wksp1+"reb",OutputWorkspace=wksp2+"reb")
    w2=getWorkspace(wksp2+"reb")
    nzind=np.nonzero(w1.dataY(0))[0]
    w2.dataY(0)[int(nzind[0])]=0.0	#set  edge of zeropadding to zero
    Rebin(InputWorkspace=wksp2,OutputWorkspace=wksp2+"reb",Params=str(Qmin)+","+str(binning)+","+str(Qmax))

    # find the bin numbers to avoid gaps
    a1=getWorkspace(wksp1+"reb").binIndexOf(begoverlap)
    a2=getWorkspace(wksp1+"reb").binIndexOf(endoverlap)
    wksp1len=len(getWorkspace(wksp1+"reb").dataX(0))
    wksp2len=len(getWorkspace(wksp2+"reb").dataX(0))
    #begoverlap = getWorkspace(wksp1+"reb").binIndexOf(endoverlap)w2.readX(0)[1]


    if scalefactor <= 0.0:
        Integration(InputWorkspace=wksp1+"reb",OutputWorkspace="i1temp", RangeLower=str(begoverlap), RangeUpper=str(endoverlap))
        Integration(InputWorkspace=wksp2+"reb",OutputWorkspace="i2temp", RangeLower=str(begoverlap), RangeUpper=str(endoverlap))
        if scalehigh:
            Multiply(LHSWorkspace=wksp2+"reb",RHSWorkspace="i1temp",OutputWorkspace=wksp2+"reb")
            Divide(LHSWorkspace=wksp2+"reb",RHSWorkspace="i2temp",OutputWorkspace=wksp2+"reb")
        else:
            Multiply(LHSWorkspace=wksp1+"reb",RHSWorkspace="i2temp",OutputWorkspace=wksp1+"reb")
            Divide(LHSWorkspace=wksp1+"reb",RHSWorkspace="i1temp",OutputWorkspace=wksp1+"reb")
        
        y1=getWorkspace("i1temp").readY(0)
        y2=getWorkspace("i2temp").readY(0)
        scalefactor = y1[0]/y2[0]
        print "scale factor="+str(scalefactor)
    else:
        MultiplyRange(wksp2+"reb",wksp2+"reb","0",str(wksp2len-2),str(scalefactor))
        
    
    MultiplyRange(InputWorkspace=wksp1+"reb",OutputWorkspace="overlap1",StartBin=0,EndBin=a1-1)
    MultiplyRange(InputWorkspace="overlap1", OutputWorkspace="overlap1",StartBin=a2,EndBin=wksp1len-2)
	
    MultiplyRange(InputWorkspace=wksp2+"reb",OutputWorkspace="overlap2",StartBin=0,EndBin=a1)#-1
    MultiplyRange(InputWorkspace="overlap2",OutputWorkspace="overlap2",StartBin=a2+1,EndBin=wksp1len-2)
	
    MultiplyRange(InputWorkspace=wksp1+"reb",OutputWorkspace=wksp1+"reb",StartBin=a1, EndBin=wksp1len-2)
    MultiplyRange(InputWorkspace=wksp2+"reb",OutputWorkspace=wksp2+"reb",StartBin=0, EndBin=a2)
    WeightedMean(InputWorkspace1="overlap1",InputWorkspace2="overlap2",OutputWorkspace="overlapave")
    Plus(LHSWorkspace=wksp1+"reb",RHSWorkspace="overlapave",OutputWorkspace='temp1')
    Plus(LHSWorkspace='temp1',RHSWorkspace=wksp2+"reb",OutputWorkspace=outputwksp)    
    
    DeleteWorkspace("temp1")
    DeleteWorkspace(wksp1+"reb")
    DeleteWorkspace(wksp2+"reb")
    DeleteWorkspace("i1temp")
    DeleteWorkspace("i2temp")
    DeleteWorkspace("overlap1")
    DeleteWorkspace("overlap2")
    DeleteWorkspace("overlapave")
    return outputwksp, scalefactor

		
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

	
