from mantidsimple import *
from mantidplot import *
from IndirectEnergyConversion import *
import math

def abscyl(inWS_n, outWS_n, efixed, sample, can):
	ConvertUnits(inWS_n, 'wavelength', 'Wavelength', 'Indirect', efixed)
	CylinderAbsorption('wavelength', outWS_n, sample[0], sample[1], sample[2], can[0], can[1], EMode='Indirect', EFixed=efixed, NumberOfSlices=can[2], NumberOfAnnuli=can[3])
	mantid.deleteWorkspace('wavelength')

def absflat(inWS_n, outWS_n, efixed, sample, can):
	ConvertUnits(inWS_n, 'wavelength', 'Wavelength', 'Indirect', efixed)
	FlatPlateAbsorption('wavelength', outWS_n, sample[0], sample[1], sample[2], can[0], can[1], can[2], EMode='Indirect', EFixed=efixed, ElementSize=can[3])
	mantid.deleteWorkspace('wavelength')

def absorption(input, mode, sample, can, efixed, Save=False):
	(direct, filename) = os.path.split(input)
	(root, ext) = os.path.splitext(filename)
	LoadNexusProcessed(input, root)
	outWS_n = root[:-3] + 'abs'
	if mode == 'Flat Plate':
		absflat(root, outWS_n, efixed, sample, can)
	if mode == 'Cylinder':
		abscyl(root, outWS_n, efixed, sample, can)
	if Save:
		SaveNexus(outWS_n, outWS_n+'.nxs')
	mantid.deleteWorkspace(root)

def demon(rawFiles, first, last, Smooth=False, SumFiles=False, CleanUp=True, plotOpt=False):
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
	(direct, filename) = os.path.split(rawFiles[0])
	(root, ext) = os.path.splitext(filename)
	for i in range(0, len(ws_names)):
		# Get Monitor WS
		MonitorWS = timeRegime(ws_list[i], inWS_n=ws_names[i], Smooth=Smooth)
		monitorEfficiency(inWS_n=MonitorWS)
		# Get Run no, crop file
		runNo = ws_list[i].getRun().getLogData("run_number").value()
		runNos.append(runNo)
		savefile = root[:3] + runNo + '_dem'
		CropWorkspace(ws_names[i], ws_names[i], StartWorkspaceIndex = (first-1), EndWorkspaceIndex = (last-1) )
		# Normalise to Monitor
		normalised = normToMon(inWS_n=ws_names[i], outWS_n=ws_names[i], monWS_n=MonitorWS)
		# Convert to dSpacing
		ConvertUnits(ws_names[i], savefile, 'dSpacing')
		workspaces.append(savefile)
		if CleanUp:
			mantid.deleteWorkspace(ws_names[i])
		SaveNexusProcessed(savefile, savefile+'.nxs')
	if plotOpt:
		for demon in workspaces:
			nspec = mantid.getMatrixWorkspace(demon).getNumberHistograms()
			plotSpectrum(demon, range(0, nspec))
	return workspaces, runNos

def elwin(inputFiles, eRange, efixed, iconOpt = {}, Save=False):
	outWS_list = []
	for file in inputFiles:
		(direct, filename) = os.path.split(file)
		(root, ext) = os.path.splitext(filename)
		if ext == '.nxs':
			LoadNexus(file, root)
			savefile = root[:3] + mantid.getMatrixWorkspace(root).getRun().getLogData("run_number").value() + '_elw'
			Elwin(root, savefile, eRange, efixed)
			if Save:
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

def fury(sam_file, res_file, rebinParam, outWS_n = '', Save=False):
	(direct, filename) = os.path.split(sam_file)
	(root, ext) = os.path.splitext(filename)

	LoadNexus(sam_file, 'irs_sam_data') # SAMPLE
	LoadNexus(res_file, 'irs_res_data') # RES

	Rebin('irs_sam_data', 'irs_sam_data', rebinParam)
	Rebin('irs_res_data', 'irs_res_data', rebinParam)

	ExtractFFTSpectrum('irs_sam_data', 'irs_sam_fft', 2)
	ExtractFFTSpectrum('irs_res_data', 'irs_res_fft', 2)

	Integration('irs_sam_data', 'irs_sam_int')
	Integration('irs_res_data', 'irs_res_int')

	Divide('irs_sam_fft', 'irs_sam_int', 'irs_sam')
	Divide('irs_res_fft', 'irs_res_int', 'irs_res')

	# Create save file name.
	runNo = mantid.getMatrixWorkspace('irs_sam_data').getRun().getLogData("run_number").value()
	savefile = root[:3] + runNo + '_iqt'
	if outWS_n == '':
		outWS_n = savefile

	# CleanUp A
	mantid.deleteWorkspace('irs_sam_data')
	mantid.deleteWorkspace('irs_res_data')
	mantid.deleteWorkspace('irs_sam_int')
	mantid.deleteWorkspace('irs_sam_fft')
	mantid.deleteWorkspace('irs_res_int')
	mantid.deleteWorkspace('irs_res_fft')

	DivideBySpectrum('irs_sam', 'irs_res', outWS_n)

	# Final Cleanup
	mantid.deleteWorkspace('irs_sam')
	mantid.deleteWorkspace('irs_res')

	if Save:
		SaveNexusProcessed(outWS_n, savefile + '.nxs')
	return outWS_n

def msdfit(file, startX, endX, Save=False, Plot=False):
	(direct, filename) = os.path.split(file)
	(root, ext) = os.path.splitext(filename)
	LoadNexusProcessed(file, root)
	outWS_n = root[:-3] + 'msd'
	fit_alg = Linear(root, outWS_n, WorkspaceIndex=0, StartX=startX, EndX=endX)
	A0 = fit_alg.getPropertyValue("FitIntercept")
	A1 = fit_alg.getPropertyValue("FitSlope")
	title = 'Intercept: '+A0+' ; Slope: '+A1
	if Plot:
		graph=plotSpectrum([root,outWS_n],0, 1)
		graph.activeLayer().setTitle(title)
	if Save:
		SaveNexusProcessed(outWS_n, outWS_n+'.nxs', Title=title)

def mut(inWS_n, deltaW, filename, efixed):
	file_handle = open(filename, 'w') # Open File
	sigs = 5.0 # sigs ?
	siga = 1.0 # siga ?
	we = math.sqrt(81.787/efixed) # 81.787 ?
	tempWS = '_tmp_indirect_mut_file_'
	ConvertUnits(inWS_n, tempWS, 'Wavelength', 'Indirect', efixed)
	xValues = mantid.getMatrixWorkspace(tempWS).readX(0)
	nbins = len(xValues)
	xMin = xValues[0]
	xMax = xValues[nbins-1]
	nw = int(xMax/deltaW) - int(xMin/deltaW) + 1
	file_handle.write(str(nw)+' '+str(we)+' '+str(siga)+' '+str(sigs)+'\n')
	for i in range(0, nw):
		wavelength = (int(xMin/deltaW) + i) * deltaW
		sigt = sigs + siga*wavelength/1.8
		file_handle.write(str(wavelength) + ' ' + str(sigt) + '\n')
	file_handle.close()
	mantid.deleteWorkspace(tempWS)

def plotFury(inWS_n, spec):
	inWS = mantid.getMatrixWorkspace(inWS_n)
	nbins = inWS.getNumberBins()
	lastValueZero = False
	for i in range(0, nbins):
		if (inWS.readY(spec[0])[i] == 0.0):
			if lastValueZero:
				xBoundary = inWS.readX(spec[0])[i]
				break
			else:
				lastValueZero = True
		else:
			lastValueZero = False
	graph = plotSpectrum(inWS_n, spec)
	layer = graph.activeLayer()
	layer.setScale(0, 0, 1.0)
	layer.setScale(2, 0, xBoundary)

def slice(inputfiles, calib, tofXRange, spectra = [0,0], Save=True):
	if  not ( ( len(tofXRange) == 2 ) or ( len(tofXRange) == 4 ) ):
		mantid.sendLogMessage('TOF Range inputs must contain either 2 or 4 numbers.')
		sys.exit(1)
	for file in inputfiles:
		(direct, filename) = os.path.split(file)
		(root, ext) = os.path.splitext(filename)
		if spectra == [0, 0]:
			LoadRaw(file, root)
		else:
			LoadRaw(file, root, SpectrumMin = spectra[0], SpectrumMax = spectra[1])
		nhist = mantid.getMatrixWorkspace(root).getNumberHistograms()
		if calib != '':
			useCalib(calib, inWS_n=root, outWS_n=root)
		savefile = root[:3] + mantid.getMatrixWorkspace(root).getRun().getLogData("run_number").value() + '_slt'
		if (len(tofXRange) == 2):
			Integration(root, savefile, tofXRange[0], tofXRange[1], 0, nhist-1)
		else:
			Integration(root, 'Unit1', tofXRange[0], tofXRange[1], 0, nhist-1)
			Integration(root, 'Unit2', tofXRange[2], tofXRange[3], 0, nhist-1)
			Minus('Unit1', 'Unit2', savefile)
			mantid.deleteWorkspace('Unit1')
			mantid.deleteWorkspace('Unit2')
		if Save:
			SaveNexusProcessed(savefile, savefile+'.nxs')
		mantid.deleteWorkspace(root)

def plotRaw(inputfiles,spectra=[]):
	if len(spectra) != 2:
		sys.exit(1)
	workspaces = []
	for file in inputfiles:
		(direct, filename) = os.path.split(file)
		(root, ext) = os.path.splitext(filename)
		LoadRaw(file, root, SpectrumMin=spectra[0], SpectrumMax = spectra[1])
		GroupDetectors(root,root,DetectorList=range(spectra[0],spectra[1]+1))
		workspaces.append(root)
	if len(workspaces) > 0:
		graph = plotSpectrum(workspaces,0)
		layer = graph.activeLayer().setTitle(", ".join(workspaces))
		