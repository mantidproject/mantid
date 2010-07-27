"""Conversion module defined for 'indirect' excitations conversions. Better as a module than a class, cleaner to access.
"""

import ConvertToEnergy
import CommonFunctions as common
from mantidsimple import *


def loadData(rawfiles, outWS='RawFile', Sum=False):
	try:
		LoadRaw(rawfiles[0], outWS)
	except ValueError, message:
		print message
		sys.exit(message)
	if ( len(rawfiles) > 1 and Sum ):
		for i in range(1, len(rawfiles)):
			tmp_ws = outWS + str(i)
			LoadRaw(rawfiles[i], tmp_ws)
			try:
				Plus(outWS, tmp_ws, outWS)
			except:
				print 'Rawfiles do not match, not suitable for summing.'
				sys.exit('Rawfiles not suitable for summing.')
			mantid.deleteWorkspace(tmp_ws)
		workspace = mtd.getMatrixWorkspace(outWS)
		return [workspace], [outWS]
	else:
		workspace_list = []
		ws_name_list = [outWS]
		if ( len(rawfiles) > 1 ):
			for i in range(1, len(rawfiles)):
				ws_name = outWS + str(i)
				LoadRaw(rawfiles[i], ws_name)
				ws_name_list.append(ws_name)
		for i in ws_name_list:
			workspace_list.append(mtd.getMatrixWorkspace(i))
		return workspace_list, ws_name_list
		

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
	'''
	This function normalises the detectors to the monitor.
	'''
	ConvertUnits(inWS_n,outWS_n, 'Wavelength')
	RebinToWorkspace(outWS_n,monWS_n,outWS_n)
	Divide(outWS_n,monWS_n,outWS_n)
	mantid.deleteWorkspace(monWS_n)
	mantid.deleteWorkspace(inWS_n)
	return outWS_n

def conToEnergy(efixed, inWS_n = 'Energy', outWS_n = 'ConvertedToEnergy'):
	'''
	This function is the actual "convert to energy" step.
	'''
	ConvertUnits(inWS_n, outWS_n, 'DeltaE', 'Indirect', efixed)
	return outWS_n

def rebinData(rebinParam, inWS_n = 'ConvertedToEnergy', outWS_n = 'Energy'):
	'''
	This function rebings the data, where rebinParams is a string of form: "ELow, EWidth, EHigh"
	'''
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

def backgroundRemoval(tofStart, tofEnd, inWS_n = 'Time', outWS_n = 'Time'):
	ConvertToDistribution(inWS_n)
	FlatBackground(inWS_n, outWS_n, tofStart, tofEnd, Mode = 'Mean')
	ConvertFromDistribution(inWS_n)
	if ( inWS_n != outWS_n ):
		ConvertFromDistribution(outWS_n)
	return outWS_n

def convert_to_energy(rawfiles, mapfile, first, last, efixed, SumFiles=False, bgremove = [0, 0], tempK=-1, calib='', rebinParam='', CleanUp = True, instrument='', savesuffix='', saveFormats = [], savedir=''):
	'''
	This function, when passed the proper arguments, will run through the steps of convert to energy
	for the indirect instruments and put out a workspace title IconCompleted.
	This "optional" steps can be avoided by not passing in the tempK (for detailed balance), calib (for calibration)
	rebinParam (for rebinning), or bgremove (for background removal) arguments.
	NOTE: Unlike the MantidPlot GUI, this will not create a map file. You can create a map file either by hand
	or with the createMappingFile function.
	'''
	output_workspace_names = []
	runNos = []
	workspace, ws_name = loadData(rawfiles, Sum=SumFiles)
	for i in range(0, len(workspace)):
		MonitorWS_n = timeRegime(workspace[i], inWS_n = ws_name[i])
		MonWS_n = monitorEfficiency()
		runNo = workspace[i].getRun().getLogData("run_number").value()
		runNos.append(runNo)
		CropWorkspace(ws_name[i], 'Time', StartWorkspaceIndex= (first - 1), EndWorkspaceIndex=( last - 1))
		mantid.deleteWorkspace(ws_name[i])
		if ( bgremove != [0, 0] ):
			backgroundRemoval(bgremove[0], bgremove[1])
		if ( calib != '' ):
			calibrated = useCalib(calib)
		normalised = normToMon()
		cte = conToEnergy(efixed, outWS_n='EnergyRebinned' + str(i))
		if ( rebinParam != ''):
			rebin = rebinData(rebinParam, inWS_n=cte)
		else:
			if CleanUp:
				RenameWorkspace(cte, 'Energy')
			else:
				CloneWorkspace(cte, 'Energy')
		if ( tempK != -1 ):
			db = detailedBalance(tempK)
		scale = scaleAndGroup(mapfile, outWS_n='IconComplete' + str(i) + '_' + runNo)
		output_workspace_names.append(scale)
	if ( saveFormats != [] ):
		saveItems(output_workspace_names, runNos, saveFormats, instrument, savesuffix, directory = savedir)


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

def saveItems(workspaces, runNos, fileFormats, ins, suffix, directory = ''):
	for i in range(0, len(workspaces)):
		filename = ins + runNos[i] + '_' + suffix
		if directory != '':
			filename = os.path.join(directory, filename)
		for j in fileFormats:
			if j == 'spe':
				SaveSPE(workspaces[i], filename + '.spe')
			elif j == 'nxs':
				SaveNexusProcessed(workspaces[i], filename + '.nxs')
			else:
				print 'Save: unknown file type.'
				system.exit('Save: unknown file type.')