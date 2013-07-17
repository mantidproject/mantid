#from ReflectometerCors import *
from l2q import *
from mantid.simpleapi import *
#from mantid.simpleapi import *  # New API
#from mantidplot import *
from PyQt4 import QtCore, uic
import math
from mantid.api import WorkspaceGroup

def combineDataMulti(wkspList,outputwksp,begoverlap,endoverlap,Qmin,Qmax,binning,scalehigh=1,scalefactor=-1.0,whichPeriod=1,keep=0):
	'''
	keep=1: keep individual workspcaes in Mantid, otheriwse delete wkspList
	'''

	# check if overlaps have correct number of entries
	if type(begoverlap) != list:
		begoverlap = [begoverlap]
	if type(endoverlap) != list:
		endoverlap = [endoverlap]
	if len(wkspList) != len(begoverlap):
		print "Using default values!"
		defaultoverlaps = 1
	else:
		defaultoverlaps = 0
		
    #copy first workspace into temporary wksp 'currentSum'
	CropWorkspace(InputWorkspace=wkspList[0],OutputWorkspace='currentSum')
	print "Length: ",len(wkspList),wkspList
	
	for i in range(0,len(wkspList)-1):
		w1=getWorkspace('currentSum')
		w2=getWorkspace(wkspList[i+1])
		if defaultoverlaps:
			overlapLow = w2.readX(0)[0]
			overlapHigh = 0.5*max(w1.readX(0))
		else:
			overlapLow = begoverlap[i+1]
			overlapHigh = endoverlap[i]
			
        #check if multiperiod
		if isinstance(mtd['currentSum'], WorkspaceGroup):
			tempwksp,sf = combine2('currentSum_'+str(whichPeriod),wkspList[i+1]+'_'+str(whichPeriod),'_currentSum',overlapLow,overlapHigh,Qmin,Qmax,binning,scalehigh)
            #if (sum(mtd['currentSum_2'].dataY(0))):
			print tempwksp, sf
			
			DeleteWorkspace("_currentSum")
			#CropWorkspace(wkspList[0],'currentSum')
			print wkspList
			combine2('currentSum',wkspList[i+1],'temp',overlapLow,overlapHigh,Qmin,Qmax,binning,scalehigh,sf)
			CropWorkspace(InputWorkspace='temp',OutputWorkspace='currentSum')
			DeleteWorkspace("temp")
            #else:
			
			#	combine2('currentSum_1',wkspList[i+1]+'_1','currentSum',begoverlap,endoverlap,Qmin,Qmax,binning,sf)
			
		else:
			print "Iteration",i
			combine2('currentSum',wkspList[i+1],'currentSum',overlapLow,overlapHigh,Qmin,Qmax,binning,scalehigh)
	RenameWorkspace(InputWorkspace='currentSum',OutputWorkspace=outputwksp)
	if not keep:
		names = mtd.getObjectNames()
		for ws in wkspList:
			#print ws.rstrip("_binned")
			DeleteWorkspace(ws)
			DeleteWorkspace(ws.rstrip("_IvsQ_binned")+"_IvsLam")
			DeleteWorkspace(ws.rstrip("_IvsQ_binned")+"_IvsQ")
	return mtd[outputwksp]


def combine2(wksp1,wksp2,outputwksp,begoverlap,endoverlap,Qmin,Qmax,binning,scalehigh=1,scalefactor=-1.0):
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
				res = mtd[wksp + '_1'].getSampleDetails().getLogData(field).value				
			except RuntimeError:
				res = 0
				print "Block "+field+" not found."			
		else:
			try:
				res = mtd[wksp].getSampleDetails().getLogData(field).value
			except RuntimeError:		
				res = 0
				print "Block "+field+" not found."
		return res
	elif (whattoget == 'wksp'):
		if isinstance(mtd[wksp], WorkspaceGroup):
			return mtd[wksp+'_1'].getNumberHistograms()
		else:
			return mtd[wksp].getNumberHistograms()

def _testCombine():
	from quick import quick
	mtd.settings['default.instrument'] = "SURF"

	[w1lam,w1q,th] = quick(94511,theta=0.25,trans='94504')
	[w2lam,w2q,th] = quick(94512,theta=0.65,trans='94504')
	[w3lam,w3q,th] = quick(94513,theta=1.5,trans='94504')

	wksp=['94511_IvsQ','94512_IvsQ','94513_IvsQ']

	wcomb = combineDataMulti(wksp,'94511_13_IvsQ',0.0,0.1,0.001,0.3,binning=-0.02)

	plotSpectrum("94511_13_IvsQ",0)

def  _doAllTests():
	_testCombine()
	return True

if __name__ == '__main__':
	''' This is the debugging and testing area of the file.  The code below is run when ever the 
	   script is called directly from a shell command line or the execute all menu option in mantid. 
	'''
	#Debugging = True  # Turn the debugging on and the testing code off
	Debugging = False # Turn the debugging off and the testing on
	
	if Debugging == False:
		_doAllTests()  
	else:    #Debugging code goes below
		
		print "No debugging at the moment..."
	
