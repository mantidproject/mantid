"""Conversion module defined for 'indirect' excitations conversions. Better as a module than a class, cleaner to access.
"""

import ConvertToEnergy
from mantidsimple import *
from mantidplot import *

def loadData(rawfiles, outWS='RawFile', Sum=False):
	( dir, file ) = os.path.split(rawfiles[0])
	( name, ext ) = os.path.splitext(file)
	try:
		LoadRaw(rawfiles[0], name)
	except ValueError, message:
		print message
		sys.exit(message)
	if ( len(rawfiles) > 1 and Sum ):
		for i in range(1, len(rawfiles)):
			tmp_ws = outWS + str(i)
			LoadRaw(rawfiles[i], tmp_ws)
			try:
				Plus(name, tmp_ws, name)
			except:
				print 'Rawfiles do not match, not suitable for summing.'
				sys.exit('Rawfiles not suitable for summing.')
			mantid.deleteWorkspace(tmp_ws)
		workspace = mtd.getMatrixWorkspace(name)
		return [workspace], [name]
	else:
		workspace_list = []
		ws_name_list = [name]
		if ( len(rawfiles) > 1 ):
			for i in range(1, len(rawfiles)):
				( dir, file ) = os.path.split(rawfiles[i])
				( name, ext ) = os.path.splitext(file)
				LoadRaw(rawfiles[i], name)
				ws_name_list.append(name)
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
	if (inWS_n != outWS_n):
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
	return output_workspace_names, runNos


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
		sys.exit('Calib: Could not load raw file.')
	tmp = mantid.getMatrixWorkspace('Raw')
	nhist = tmp.getNumberHistograms()
	Integration('Raw', 'CalA', peakMin, peakMax, 0, nhist -1)
	Integration('Raw', 'CalB', backMin, backMax, 0, nhist -1)
	Minus('CalA', 'CalB', outWS_n)
	mantid.deleteWorkspace('Raw')
	mantid.deleteWorkspace('CalA')
	mantid.deleteWorkspace('CalB')
	cal_ws = mantid.getMatrixWorkspace(outWS_n)
	sum = 0
	for i in range(0, nhist):
		sum += cal_ws.readY(i)[0]
	value = sum / nhist
	CreateSingleValuedWorkspace('avg', value)
	Divide(outWS_n, 'avg', outWS_n)
	mantid.deleteWorkspace('avg')	
	SaveNexusProcessed(outWS_n, savefile, 'Vanadium')
	return outWS_n

def res(file, nspec, iconOpt, rebinParam, background):
	''' ? '''
	(direct, filename) = os.path.split(file)
	(root, ext) = os.path.splitext(filename)
	mapping = createMappingFile('res.map', 1, nspec, iconOpt['first'])
	rawfiles = [file]
	workspace_list, runNos = convert_to_energy(rawfiles, mapping, iconOpt['first'], iconOpt['last'], iconOpt['efixed'])
	iconWS = workspace_list[0]
	Rebin(iconWS, iconWS, rebinParam)
	FFTSmooth(iconWS,iconWS,0)
	name = root[:3] + mantid.getMatrixWorkspace(workspace_list[0]).getRun().getLogData("run_number").value() + '_res'
	FlatBackground(iconWS, name, background[0], background[1])
	mantid.deleteWorkspace(iconWS)
	SaveNexusProcessed(name, name+'.nxs')
	return name

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

def demon(rawFiles, calFile, first, last, SumFiles=False, CleanUp=True, plotOpt=False):
	'''
	DEMON function for unit conversion on diffraction backs of IRIS/OSIRIS.
	MANDATORY PARAMS:
	@param rawFiles list of files to load (list[string])
	@param calFile CalFile/Grouping file (string)
	@param first first spectra number of diffraction block (integer)
	@param last last spectra number of diffraction block (integer)
	OPTIONAL PARAMS:
	@param SumFiles whether to sum input files, or run through them sequentially (boolean)
	@param CleanUp whether to remove intermediate workspaces from memory (boolean)
	@param plotOpt whether to plot the spectra of the result (boolean)
		-- WARNING - with large numbers of spectra and/or lots of input files plotOpt will slow down the script significantly
	'''
	ws_list, ws_names = loadData(rawFiles, Sum=SumFiles)
	runNos = []
	workspaces = []
	for i in range(0, len(ws_names)):
		# Get Monitor WS
		MonitorWS = timeRegime(ws_list[i], inWS_n=ws_names[i])
		monitorEfficiency(inWS_n=MonitorWS)
		# Get Run no, crop file
		runNo = ws_list[i].getRun().getLogData("run_number").value()
		runNos.append(runNo)
		savefile = rawFiles[0][:3] + runNo + '_dem'
		CropWorkspace(ws_names[i], ws_names[i], StartWorkspaceIndex = (first-1), EndWorkspaceIndex = (last-1) )
		# Normalise to Monitor
		normalised = normToMon(inWS_n=ws_names[i], outWS_n=ws_names[i], monWS_n=MonitorWS)
		# Convert to dSpacing
		ConvertUnits(ws_names[i], savefile, 'dSpacing')
		# DiffractionFocussing(ws_names[i], savefile, calFile) -- not needed?
		workspaces.append(savefile)
		if CleanUp:
			mantid.deleteWorkspace(ws_names[i])
		SaveNexusProcessed(savefile, savefile+'.nxs')
	if plotOpt:
		for demon in workspaces:
			nspec = mantid.getMatrixWorkspace(demon).getNumberHistograms()
			plotSpectrum(demon, range(0, nspec))
	return workspaces, runNos

def elwin(inputFiles, eRange, iconOpt = {}):
	outWS_list = []
	for file in inputFiles:
		(direct, filename) = os.path.split(file)
		(root, ext) = os.path.splitext(filename)
		if ext == '.nxs':
			LoadNexus(file, root)
			savefile = root[:3] + mantid.getMatrixWorkspace(root).getRun().getLogData("run_number").value() + '_elw'
			nhist = mantid.getMatrixWorkspace(root).getNumberHistograms()
			Integration(root, savefile, eRange[0], eRange[1], 0, nhist-1)
			SaveNexusProcessed(savefile, savefile+'.nxs')
			outWS_list.append(savefile)
			mantid.deleteWorkspace(root)
		elif ext == '.raw':
			if ( len(iconOpt) != 8 ):
				message = 'Elwin: Number of values for iconOpt parameter do not match expectation. Please view function definition for details.'
				print message
				sys.exit(message)
			inWS_l, runNos = convert_to_energy([file], iconOpt['map'], iconOpt['first'], iconOpt['last'], iconOpt['efixed'], bgremove = iconOpt['bgremove'], tempK=iconOpt['tempK'], calib=iconOpt['calib'], rebinParam=iconOpt['rebin'])
			inWS = inWS_l[0]
			savefile = file[:3] + runNos[0] + '_elw'
			nhist = mantid.getMatrixWorkspace(inWS).getNumberHistograms()
			Integration(inWS, savefile, eRange[0], eRange[1], 0, nhist-1)
			SaveNexusProcessed(savefile, savefile+'.nxs')
			outWS_list.append(savefile)
			mantid.deleteWorkspace(inWS)
		else:
			print 'Unrecognised file type.'
			sys.exit('Elwin: unrecognised input file type (' +ext+ ')')
	return outWS_list

def slice(inputfiles, calib, enXRange = [], tofXRange = [], inWS_n='Energy', outWS_n='Time', spectra = [0,0]):
	'''
	This function does the "Slice" part of modes. To be passed in a list of input files (either .raw or .nxs),
	and a (2) 4-element list of X-values to integrate over for the files passed in, one for Energy and one for TOF.
	If your files only deal with one of these, you can omit the other.
	'''
	if ( enXRange == [] and tofXRange == [] ):
		message = 'Slice: no x-ranges for integration were provided.'
		print message
		sys.exit(message)
	for file in inputfiles:
		(direct, filename) = os.path.split(file)
		(root, ext) = os.path.splitext(filename)
		if ext == '.nxs':
			if (enXRange == []):
				message = 'Slice: values for integration over energy have not been supplied.'
				print message
				sys.exit(message)
			root = root[:-3]
			LoadNexus(file, root)
			nhist = mantid.getMatrixWorkspace(root).getNumberHistograms()
			savefile = root[:3] + mantid.getMatrixWorkspace(root).getRun().getLogData("run_number").value() + '_sle'
			Integration(root, 'Unit1', enXRange[0], enXRange[1], 0, nhist-1)
			Integration(root, 'Unit2', enXRange[2], enXRange[3], 0, nhist-1)
			Minus('Unit1', 'Unit2', savefile)
			SaveNexusProcessed(savefile, savefile+'.nxs')
			mantid.deleteWorkspace(root)
		elif ext == '.raw':
			if (tofXRange == []):
				message = 'Slice: values for integration over time of flight have not been supplied.'
				print message
				sys.exit(message)
			unit = 'Time'
			if spectra == [0, 0]:
				LoadRaw(file, root)
			else:
				LoadRaw(file, root, SpectrumMin = spectra[0], SpectrumMax = spectra[1])
			nhist = mantid.getMatrixWorkspace(root).getNumberHistograms()
			if calib != '':
				useCalib(calib, inWS_n=root, outWS_n=root)
			savefile = root[:3] + mantid.getMatrixWorkspace(root).getRun().getLogData("run_number").value() + '_slt'
			Integration(root, 'Unit1', tofXRange[0], tofXRange[1], 0, nhist-1)
			Integration(root, 'Unit2', tofXRange[2], tofXRange[3], 0, nhist-1)
			Minus('Unit1', 'Unit2', savefile)
			SaveNexusProcessed(savefile, savefile+'.nxs')
			mantid.deleteWorkspace(root)
		else:
			message = 'Slice: Unrecognised file extension ('+ext+')'
			print message
			sys.exit(message)
	mantid.deleteWorkspace('Unit1')
	mantid.deleteWorkspace('Unit2')

def fury(sam_file, res_file, rebinParam, outWS_n = ''):
	(direct, filename) = os.path.split(sam_file)
	(root, ext) = os.path.splitext(filename)

	workspace_res = 'res_file'
	LoadNexus(res_file, workspace_res)

	Rebin(workspace_res, workspace_res, rebinParam)
	Integration(workspace_res, workspace_res+'_sv')
	FFT(workspace_res, workspace_res+'_fft')
	ExtractSingleSpectrum(workspace_res+'_fft', workspace_res+'_fft', 2)
	Divide(workspace_res+'_fft', workspace_res+'_sv', workspace_res+'_ssr')

	mantid.deleteWorkspace(workspace_res+'_sv')
	mantid.deleteWorkspace(workspace_res+'_fft')
	mantid.deleteWorkspace(workspace_res)

	workspace_sam = 'sample'
	LoadNexus(sam_file, workspace_sam)
	Rebin(workspace_sam, workspace_sam, rebinParam)
	Integration(workspace_sam, workspace_sam+'_int')
	nhist = mantid.getMatrixWorkspace(workspace_sam).getNumberHistograms()
	runNo = mantid.getMatrixWorkspace(workspace_sam).getRun().getLogData("run_number").value()
	for n in range(0, nhist):
		tmpWS = 'tmp_ws_fury_' + str(n)
		FFT(workspace_sam, tmpWS, n)
		ExtractSingleSpectrum(tmpWS, tmpWS, 2)
		if (n == 0):
			RenameWorkspace(tmpWS, workspace_sam+'_fft')
		else:
			ConjoinWorkspaces(workspace_sam+'_fft', tmpWS)

	Divide(workspace_sam+'_fft', workspace_sam+'_int',  workspace_sam+'_result')

	mantid.deleteWorkspace(workspace_sam)
	mantid.deleteWorkspace(workspace_sam+'_fft')
	mantid.deleteWorkspace(workspace_sam+'_int')

	res_result = workspace_res +'_result'
	for n in range(0, nhist):
		tmpWS = 'tmp_res_conjoining' + str(n)
		ExtractSingleSpectrum(workspace_res+'_ssr', tmpWS, 0)
		if (n == 0):
			RenameWorkspace(tmpWS, res_result)
		else:
			ConjoinWorkspaces(res_result, tmpWS)

	mantid.deleteWorkspace(workspace_res+'_ssr')

	savefile = root[:3] + runNo + '_iqt'
	if outWS_n == '':
		outWS_n = savefile

	Divide(workspace_sam+'_result', workspace_res+'_result', outWS_n)
	mantid.deleteWorkspace(workspace_sam+'_result')
	mantid.deleteWorkspace(workspace_res+'_result')
