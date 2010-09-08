from mantidsimple import *
from mantidplot import *

import re

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

def timeRegime(inWS, inWS_n='Rawfile', outWS_n='MonWS', Smooth=True):
	FirstMon, FirstDet = getFirstMonFirstDet(inWS)
	LRef = getReferenceLength(inWS, FirstDet)
	SpecMon = inWS.readX(FirstMon)[0]
	SpecDet = inWS.readX(FirstDet)[0]
	CropWorkspace(inWS_n, 'MonIn', StartWorkspaceIndex = FirstMon, EndWorkspaceIndex = FirstMon)
	if ( SpecMon == SpecDet ):
		alg = Unwrap('MonIn', outWS_n, LRef = LRef)
		join = float(alg.getPropertyValue('JoinWavelength'))
		RemoveBins(outWS_n, outWS_n, join-0.001, join+0.001, Interpolation='Linear')
		if Smooth:
			FFTSmooth(outWS_n, outWS_n, 0)
	else:
		ConvertUnits('MonIn', outWS_n, 'Wavelength')
	mantid.deleteWorkspace('MonIn')
	return outWS_n

def monitorEfficiency(inWS_n='MonWS', unt=1.276e-3, zz=0.025):
	CreateSingleValuedWorkspace('moneff', unt) #value 1.276e-3 (unt)- what is it?
	OneMinusExponentialCor(inWS_n, inWS_n, (8.3 * zz) ) #values 8.3 (?), 0.025 (zz) - what is it?
	Divide(inWS_n,'moneff',inWS_n)
	mantid.deleteWorkspace('moneff')
	return inWS_n

def getReferenceLength(workspace, fdi):
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
	if (inWS_n != outWS_n):
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

def groupData(mapfile, inWS_n = 'Energy', outWS_n = 'IconComplete'):
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

def convert_to_energy(rawfiles, mapfile, first, last, efixed, analyser = '', reflection = '', SumFiles=False, bgremove = [0, 0], tempK=-1, calib='', rebinParam='', CleanUp = True, instrument='', savesuffix='', saveFormats = [], savedir=''):
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
	mtd.sendLogMessage(">> Loading RAW Files: " + ", ".join(rawfiles))
	workspace, ws_name = loadData(rawfiles, Sum=SumFiles)
	mtd.sendLogMessage(">> Raw files loaded into workspaces: " + ", ".join(ws_name))
	for i in range(0, len(workspace)):
		(direct, filename) = os.path.split(rawfiles[i])
		runNo = workspace[i].getRun().getLogData("run_number").value()
		runNos.append(runNo)
		name = filename[:3].lower() + runNo + '_' + analyser + reflection
		MonitorWS_n = timeRegime(workspace[i], inWS_n = ws_name[i])
		MonWS_n = monitorEfficiency()
		mtd.sendLogMessage(">> Monitor Workspace for " +ws_name[i]+" is " + MonWS_n)
		CropWorkspace(ws_name[i], 'Time', StartWorkspaceIndex= (first - 1), EndWorkspaceIndex=( last - 1))
		mantid.deleteWorkspace(ws_name[i])
		if ( bgremove != [0, 0] ):
			backgroundRemoval(bgremove[0], bgremove[1])
		if ( calib != '' ):
			mtd.sendLogMessage('>> Applying Calibration File: ' + calib)
			calibrated = useCalib(calib)
		normalised = normToMon()
		mtd.sendLogMessage('>> Converting workspace ' + normalised + ' to units of deltaE.')
		cte = conToEnergy(efixed, outWS_n=name+'_intermediate')
		if ( rebinParam != ''):
			mtd.sendLogMessage('>> Rebinning workspace ' + cte + ' with parameters (' +rebinParam+ ')')
			rebin = rebinData(rebinParam, inWS_n=cte)
			if CleanUp:
				mtd.sendLogMessage('>> Removing intermediate workspace: ' + cte)
				mantid.deleteWorkspace(cte)
		else:
			if CleanUp:
				mtd.sendLogMessage('>> Removing intermediate workspace: ' + cte)
				RenameWorkspace(cte, 'Energy')
			else:
				CloneWorkspace(cte, 'Energy')
		if ( tempK != -1 ):
			mtd.sendLogMessage('>> Adjusting for "detailed balance" at temperature: ' + str(tempK) + 'K')
			db = detailedBalance(tempK)
		group = groupData(mapfile, outWS_n=name)
		output_workspace_names.append(group)
	if ( saveFormats != [] ):
		saveItems(output_workspace_names, runNos, saveFormats, instrument, savesuffix, directory = savedir)
		mtd.sendLogMessage(">> Saved workspaces: " + ", ".join(output_workspace_names) +" in formats: "+ ", ".join(saveFormats))
	else:
		mtd.sendLogMessage(">> Workspaces were not saved.")
	mtd.sendLogMessage(">> Convert to Energy completed with the following output workspaces: " + ", ".join(output_workspace_names))
	return output_workspace_names, runNos

def cte_rebin(mapfile, tempK, rebinParam, analyser, reflection, instrument, savesuffix, saveFormats, savedir, CleanUp=False):
	ws_list = mantid.getWorkspaceNames()
	energy = re.compile('_'+analyser+reflection+r'_intermediate$')
	int_list = []
	if ( len(int_list) == 0 ):
		message = "No intermediate workspaces were found. Run with 'Keep Intermediate Workspaces' checked."
		print message
		sys.exit(message)
	output_workspace_names = []
	runNos = []
	for workspace in ws_list:
		if energy.search(workspace):
			int_list.append(workspace)
	for cte in int_list:
		runNo = mantid.getMatrixWorkspace(cte).getRun().getLogData("run_number").value()
		runNos.append(runNo)
		if ( rebinParam != ''):
			rebin = rebinData(rebinParam, inWS_n=cte)
			if CleanUp:
				mantid.deleteWorkspace(cte)
		else:
			if CleanUp:
				RenameWorkspace(cte, 'Energy')
			else:
				CloneWorkspace(cte, 'Energy')
		if ( tempK != -1 ):
			db = detailedBalance(tempK)
		scale = scaleAndGroup(mapfile, outWS_n=cte[:-13])
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

def createCalibFile(rawfile, suffix, peakMin, peakMax, backMin, backMax, specMin, specMax, outWS_n = 'Calibration', PlotOpt=False):
	savepath = mantid.getConfigProperty('datasearch.directories')
	savesuffix = rawfile[:-4] + suffix
	savefile = os.path.join(savepath, savesuffix)
	try:
		LoadRaw(rawfile, 'Raw', SpectrumMin = specMin, SpectrumMax = specMax)
	except:
		sys.exit('Calib: Could not load raw file.')
	tmp = mantid.getMatrixWorkspace('Raw')
	nhist = tmp.getNumberHistograms()
	FlatBackground('Raw', 'Raw', StartX=backMin, EndX=backMax, Mode='Mean')
	Integration('Raw', outWS_n, peakMin, peakMax)
	mantid.deleteWorkspace('Raw')
	cal_ws = mantid.getMatrixWorkspace(outWS_n)
	sum = 0
	for i in range(0, nhist):
		sum += cal_ws.readY(i)[0]
	value = sum / nhist
	CreateSingleValuedWorkspace('avg', value)
	Divide(outWS_n, 'avg', outWS_n)
	mantid.deleteWorkspace('avg')	
	SaveNexusProcessed(outWS_n, savefile, 'Vanadium')
	if PlotOpt:
		graph = plotTimeBin(outWS_n, 0)
	else:
		mantid.deleteWorkspace(outWS_n)
	return savefile

def res(file, iconOpt, rebinParam, background, plotOpt = False, Res = True):
	(direct, filename) = os.path.split(file)
	(root, ext) = os.path.splitext(filename)
	nspec = iconOpt['last'] - iconOpt['first'] + 1
	mapping = createMappingFile('res.map', 1, nspec, iconOpt['first'])
	rawfiles = [file]
	workspace_list, runNos = convert_to_energy(rawfiles, mapping, iconOpt['first'], iconOpt['last'], iconOpt['efixed'])
	iconWS = workspace_list[0]
	if Res:
		Rebin(iconWS, iconWS, rebinParam)
		FFTSmooth(iconWS,iconWS,0)
		name = root[:3].lower() + mantid.getMatrixWorkspace(workspace_list[0]).getRun().getLogData("run_number").value() + '_res'
		FlatBackground(iconWS, name, background[0], background[1])
		mantid.deleteWorkspace(iconWS)
		SaveNexusProcessed(name, name+'.nxs')
		if plotOpt:
			graph = plotSpectrum(name, 0)
		else:
			mantid.deleteWorkspace(name)
		return name
	else:
		if plotOpt:
			graph = plotSpectrum(iconWS, 0)
		return iconWS

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

def getInstrumentDetails(instrument):
	idf_dir = mantid.getConfigProperty('instrumentDefinition.directory')
	idf = idf_dir + instrument + '_Definition.xml'
	LoadEmptyInstrument(idf, 'ins')
	workspace = mtd['ins']
	instrument = workspace.getInstrument()
	ana_list_split = instrument.getStringParameter('analysers')[0].split(',')
	reflections = []
	result = ''
	for i in range(0,len(ana_list_split)):
		list = []
		name = 'refl-' + ana_list_split[i]
		list.append( ana_list_split[i] )
		try:
			item = instrument.getStringParameter(name)[0]
		except IndexError:
			item = ''
		refl = item.split(',')
		list.append( refl )
		reflections.append(list)
	for i in range(0, len(reflections)):
		message = reflections[i][0] + '-'
		for j in range(0,len(reflections[i][1])):
			message += str(reflections[i][1][j])
			if j < ( len(reflections[i][1]) -1 ):
				message += ','
		result += message
		if ( i < ( len(reflections) - 1) ):
			result += '\n'
	mtd.deleteWorkspace('ins')
	return result

def getReflectionDetails(instrument, analyser, reflection):
	idf_dir = mantid.getConfigProperty('instrumentDefinition.directory')
	idf = idf_dir + instrument + '_Definition.xml'
	ipf = idf_dir + instrument + '_' + analyser + '_' + reflection + '_Parameters.xml'
	LoadEmptyInstrument(idf, 'ins')
	LoadParameterFile('ins', ipf)
	instrument = mtd['ins'].getInstrument()
	result = str( int(instrument.getNumberParameter('spectra-min')[0]) ) + '\n'
	result += str( int(instrument.getNumberParameter('spectra-max')[0]) ) + '\n'
	result += str( instrument.getNumberParameter('efixed-val')[0] ) + '\n'
	result += str( int(instrument.getNumberParameter('peak-start')[0]) ) + '\n'
	result += str( int(instrument.getNumberParameter('peak-end')[0]) ) + '\n'
	result += str( int(instrument.getNumberParameter('back-start')[0]) ) + '\n'
	result += str( int(instrument.getNumberParameter('back-end')[0]) )
	mantid.deleteWorkspace('ins')
	return result

def getSpectraRanges(instrument):
	idf_dir = mantid.getConfigProperty('instrumentDefinition.directory')
	idf = idf_dir + instrument + '_Definition.xml'
	LoadEmptyInstrument(idf, 'ins')
	workspace = mtd['ins']
	instrument = workspace.getInstrument()
	analyser = []
	analyser_final = []
	result = ''
	for i in range(0, instrument.nElements() ):
		if instrument[i].type() == 'ParCompAssembly':
			analyser.append(instrument[i])
	for i in range(0, len(analyser) ):
		analyser_final.append(analyser[i])
		for j in range(0, analyser[i].nElements() ):
			if analyser[i][j].type() == 'ParCompAssembly':
				try:
					analyser_final.remove(analyser[i])
				except ValueError:
					pass
				analyser_final.append(analyser[i][j])
	for i in range(0, len(analyser_final)):
		message = analyser_final[i].getName() + '-'
		message += str(analyser_final[i][0].getID()) + ','
		message += str(analyser_final[i][analyser_final[i].nElements()-1].getID())
		result += message + '\n'
	mtd.deleteWorkspace('ins')
	return result