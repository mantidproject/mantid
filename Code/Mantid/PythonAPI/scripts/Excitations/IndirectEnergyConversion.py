"""Conversion module defined for 'indirect' excitations conversions. Better as a module than a class, cleaner to access.
"""

import ConvertToEnergy
import CommonFunctions as common
from mantidsimple import *


def loadData(path, outWS='RawFile', Sum=False):
	try:
		LoadRaw(path,outWS)
	except ValueError, message:
		print message
		sys.exit(message)
	workspace = mtd.getMatrixWorkspace(outWS)
	return workspace, outWS

def getFirstMonFirstDet(workspace):
	'''
	This function finds the index of the first monitor and the first
	detector in the instrument data, when a workspace reference.
	'''
	#workspace = mtd.getMatrixWorkspace(workspace)
	FirstDet = FirstMon = -1
	nhist = workspace.getNumberHistograms()
	for counter in range(0, nhist):
		detector = workspace.getDetector(counter)
		if detector.isMonitor():
			if (FirstMon == -1):
				FirstMon = counter
		else:
			if (FirstDet == -1):
				FirstDet = counter
		if ( FirstMon != -1 and FirstDet != -1 ):
			break
	return FirstMon, FirstDet

def timeRegime(inWS, inWS_n='Rawfile', outWS_n='MonWS'):
	'''
	This function determines whether or not we need to Unwrap to the Monitor, and then
	performs the appropriate action.
	'''
	FirstMon, FirstDet = getFirstMonFirstDet(inWS)
	LRef = getReferenceLength(inWS, FirstDet)
	SpecMon = inWS.readX(FirstMon)[0]
	SpecDet = inWS.readX(FirstDet)[0]
	CropWorkspace(inWS_n, 'MonIn', StartWorkspaceIndex = FirstMon, EndWorkspaceIndex = FirstMon)
	if ( SpecMon == SpecDet ):
		alg = Unwrap('MonIn', outWS_n, LRef = LRef)
		join = float(alg.getPropertyValue('JoinWavelength'))
		RemoveBins(outWS_n, outWS_n, join-0.001, join+0.001, Interpolation='Linear')
		FFTSmooth(outWS_n, outWS_n, 0)
	else:
		ConvertUnits('MonIn', outWS_n, 'Wavelength')
	mantid.deleteWorkspace('MonIn')
	return outWS_n

def monitorEfficiency(inWS_n='MonWS', unt=1.276e-3, zz=0.025):
	'''
	This function corrects for monitor efficiency.
	unt and zz are some monitor-specific values. The defaults here are those used in the
	ISIS indirect instruments (IRIS and OSIRIS).
	'''
	CreateSingleValuedWorkspace('moneff', unt) #value 1.276e-3 (unt)- what is it?
	OneMinusExponentialCor(inWS_n, inWS_n, (8.3 * zz) ) #values 8.3 (?), 0.025 (zz) - what is it?
	Divide(inWS_n,'moneff',inWS_n)
	mantid.deleteWorkspace('moneff')
	return inWS_n

def getReferenceLength(workspace, fdi):
	'''
	This function determined the reference length to use when "Unwrapping" monitors.
	This is the length of the neutron flight path from source -> sample -> detector
	'''
	instrument = workspace.getInstrument()
	sample = instrument.getSample()
	source = instrument.getSource()
	detector = workspace.getDetector(fdi)
	sample_to_source = sample.getPos() - source.getPos()
	r = detector.getDistance(sample)
	x = sample_to_source.getZ()
	LRef = x + r
	return LRef

def useCalib(path, inWS_n='Time', outWS_n='Time'):
	'''
	This step corrects for detector efficiency using the provided calibration file.
	'''
	try:
		LoadNexusProcessed(path, 'calib')
	except ValueError, message:
	   print message
	   sys.exit(message)
	tmp = mantid.getMatrixWorkspace(inWS_n)
	shist = tmp.getNumberHistograms()
	tmp = mantid.getMatrixWorkspace('calib')
	chist = tmp.getNumberHistograms()
	if chist != shist:
		print 'Number of spectra in calibration file does not match data file.'
		mantid.deleteWorkspace('calib')
		sys.exit('Number of spectra in calibration file does not match data file.')
	else:
		Divide(inWS_n,'calib',outWS_n)
		mantid.deleteWorkspace('calib')
	return outWS_n

def normToMon(inWS_n = 'Time', outWS_n = 'Energy', monWS_n = 'MonWS'):
	ConvertUnits(inWS_n,outWS_n, 'Wavelength')
	RebinToWorkspace(outWS_n,monWS_n,outWS_n)
	Divide(outWS_n,monWS_n,outWS_n)
	mantid.deleteWorkspace(monWS_n)
	mantid.deleteWorkspace(inWS_n)
	return outWS_n

def conToEnergy(efixed, inWS_n = 'Energy', outWS_n = 'ConvertedToEnergy'):
	ConvertUnits(inWS_n, outWS_n, 'DeltaE', 'Indirect', efixed)
	return outWS_n

def rebinData(rebinParam, inWS_n = 'ConvertedToEnergy', outWS_n = 'Energy'):
	Rebin(inWS_n, outWS_n, rebinParam)
	return outWS_n

def detailedBalance(tempK, inWS_n = 'Energy', outWS_n = 'Energy'):
	ExponentialCorrection(inWS_n, outWS_n, 1.0, ( 11.606 / ( 2 * tempK ) ) )
	return outWS_n

def scaleAndGroup(mapfile, inWS_n = 'Energy', outWS_n = 'IconComplete'):
	CreateSingleValuedWorkspace('scale', 1e9)
	Multiply(inWS_n, 'scale', inWS_n)
	mantid.deleteWorkspace('scale')
	GroupDetectors(inWS_n, outWS_n, MapFile = mapfile)
	mantid.deleteWorkspace(inWS_n)
	return outWS_n

def convert_to_energy(rawfile, calib, mapfile, first, last, efixed, tempK):
	workspace, ws_name = loadData(rawfile)
	MonitorWS_n = timeRegime(workspace)
	MonWS_n = monitorEfficiency()
	CropWorkspace(ws_name, 'Time', StartWorkspaceIndex= (first - 1), EndWorkspaceIndex=( last - 1))
	mantid.deleteWorkspace(ws_name)
	calibrated = useCalib(calib)
	normalised = normToMon()
	cte = conToEnergy(efixed)
	db = detailedBalance(tempK)
	scale = scaleAndGroup(mapfile)
	return scale

def createMappingFile(groupFile, ngroup, nspec, first):
	filename = mtd.getConfigProperty('defaultsave.directory')
	filename += groupFile
	handle = open(filename, 'w')
	handle.write(str(ngroup) +  "\n" )
	for n in range(0, ngroup):
		n1 = n * nspec + first
		handle.write(str(n+1) +  '\n' )
		handle.write(str(nspec) +  '\n')
		for i in range(1, nspec+1):
			n3 = n1 + i - 1
			handle.write(str(n3).center(4) + ' ')
		handle.write('\n')
	handle.close()
	return filename

def createCalibFile(rawfile, savefile, peakMin, peakMax, backMin, backMax, specMin, specMax, outWS_n = 'Calibration'):
	try:
		LoadRaw(rawfile, 'Raw', SpectrumMin = specMin, SpectrumMax = specMax)
	except:
		sys.exit('Could not load raw file.')
	tmp = mantid.getMatrixWorkspace('Raw')
	nhist = tmp.getNumberHistograms() - 1
	Integration('Raw', 'CalA', peakMin, peakMax, 0, nhist)
	Integration('Raw', 'CalB', backMin, backMax, 0, nhist)
	Minus('CalA', 'CalB', outWS_n)
	mantid.deleteWorkspace('Raw')
	mantid.deleteWorkspace('CalA')
	mantid.deleteWorkspace('CalB')
	SaveNexusProcessed(outWS_n, savefile, 'Vanadium')
	return outWS_n