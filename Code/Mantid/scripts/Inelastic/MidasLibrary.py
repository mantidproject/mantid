##############################################
# This module contains utility functions common to the MODES scripts
##############################################
from MantidFramework import *
from mantidsimple import *
from mantidplotpy import *
import math, os.path, datetime
from IndirectCommon import getEfixed

def PadArray(inarray,nfixed):
	npt=len(inarray)
	padding = nfixed-npt
	outarray=[]
	outarray.extend(inarray)
	outarray +=[0]*padding
	return outarray

def PadXY(Xin,Yin,array_len):
	X=PadArray(Xin,array_len)
	Y=PadArray(Yin,array_len)
	return X,Y

def PadXYE(Xin,Yin,Ein,array_len):
	X=PadArray(Xin,array_len)
	Y=PadArray(Yin,array_len)
	E=PadArray(Ein,array_len)
	return X,Y,E

def Array2List(n,Array):
	List = []
	for m in range(0,n):
		List.append(Array[m])
	return List

def Array2Fit(n,Array,nf):
	List = []
	for m in range(0,n):
		off = (nf-1)*n
		List.append(Array[m+off])
	return List

def ExtractFloat(a):                              #extract values from line of ascii
	extracted = []
	elements = a.split()							#split line on spaces
	for n in elements:
		extracted.append(float(n))
	return extracted                                 #values as list

def ExtractInt(a):                              #extract values from line of ascii
	extracted = []
	elements = a.split()							#split line on spaces
	for n in elements:
		extracted.append(int(n))
	return extracted                                 #values as list

def conjoincreated(input, name, unit):
	dataX = []
	dataY = []
	dataE = []
	NoSpectra = 0
	for ws in input:
		nSpec = mtd[ws].getNumberHistograms()
		NoSpectra += nSpec
		for s in range(0, nSpec):
			readX = mtd[ws].readX(s)
			readY = mtd[ws].readY(s)
			readE = mtd[ws].readE(s)
			for i in range(0, len(readX)):
				dataX.append(readX[i])
			for i in range(0, len(readY)):
				dataY.append(readY[i])
			for i in range(0, len(readE)):
				dataE.append(readE[i])
	CreateWorkspace(OutputWorkspace=name, DataX=dataX, DataY=dataY, DataE=dataE, 
		Nspec=NoSpectra, UnitX=unit)

def newconjoincreated(input, name, unit):
	dataX = []
	dataY = []
	dataE = []
	NoSpectra = 0
	specDet = []
	for ws in input:
		curWS = mtd[ws]
		nSpec = curWS.getNumberHistograms()
		NoSpectra += nSpec
		axis1 = curWS.getAxis(1)
		for s in range(0, nSpec):
			det = curWS.getDetector(s)
			specDet.append([axis1.spectraNumber(s), det.getID()])
			readX = curWS.readX(s)
			readY = curWS.readY(s)
			readE = curWS.readE(s)
			for i in range(0, len(readX)):
				dataX.append(readX[i])
			for i in range(0, len(readY)):
				dataY.append(readY[i])
			for i in range(0, len(readE)):
				dataE.append(readE[i])
	conjoined = CreateWorkspace(OutputWorkspace=name, DataX=dataX, DataY=dataY, DataE=dataE, 
		Nspec=NoSpectra, UnitX=unit).workspace() #Comes with a spectra axis from 1->NoSpectra
	newSpectraAxis = createSpectraAxis(len(specDet)) # We one that has the correct spectrum numbers as defined by the input workspace
	for i in range(len(specDet)):
		newSpectraAxis.setValue(i,specDet[i][0])
		spectra = conjoined.getSpectrum(i)
		spectra.setDetectorID(specDet[i][1])
	conjoined.replaceAxis(1, newSpectraAxis)
	# We need to do this each time because a band new workspace has been created with no instrument and hence no detector IDs
	# The first time it will take a bit of time but subsequent runs it will be very quick
	if name[0:3] == 'irs':
		instr = 'IRIS'
	if name[0:3] == 'osi':
		instr = 'OSIRIS'
	LoadInstrument(Workspace=conjoined, InstrumentName=instr, RewriteSpectraMap=False)

def WaveRange(inWS,efixed,delw,verbose=False):
# calculates the wavelengths for a defined bin size delw
	oWS='out'
	ExtractSingleSpectrum(InputWorkspace=inWS, OutputWorkspace=oWS, 
		SpectrumIndex=0)
	ConvertUnits(InputWorkspace=oWS, OutputWorkspace=oWS, Target="Wavelength",
		EMode="Indirect", EFixed=efixed)
	Xin = mtd[oWS].readX(0)
	npt = len(Xin)-1					# get no. points from length of x array
	xmin = mtd[oWS].readX(0)[0]			# get min & max values of input x
	xmax = mtd[oWS].readX(0)[npt]
	if delw == 0.0:
		ebin = 0.5
	else:
		ebin = delw
	nw1 = int(xmin/ebin)
	nw2 = int(xmax/ebin)+1
	w1 = nw1*ebin						# first new wave
	w2 = nw2*ebin						# last new wave
	wave = []
	if delw == 0.0:
		nw = 10
		ebin = (w2-w1)/(nw-1)
	else:
		nw = nw2-nw1+1
	for l in range(0,nw):
		wave.append(w1+l*ebin)
	if verbose:
		logger.information('Range : Input wavelength is ' + str(xmin) + ' to ' +str(xmax))
		logger.information('Range : '+str(nw)+' points from ' + str(wave[0]) + ' to ' +str(wave[nw-1]))
	DeleteWorkspace(oWS)
	return wave
