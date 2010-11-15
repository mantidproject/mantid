from mantidsimple import *
from mantidplot import *
from IndirectEnergyConversion import *
import math

def abscyl(inWS_n, outWS_n, efixed, sample, can):
    ConvertUnits(inWS_n, 'wavelength', 'Wavelength', 'Indirect', efixed)
    CylinderAbsorption('wavelength', outWS_n, sample[0], sample[1], sample[2],
        can[0],can[1], EMode='Indirect', EFixed=efixed, NumberOfSlices=can[2],
        NumberOfAnnuli=can[3])
    mantid.deleteWorkspace('wavelength')

def absflat(inWS_n, outWS_n, efixed, sample, can):
    ConvertUnits(inWS_n, 'wlength', 'Wavelength', 'Indirect', efixed)
    FlatPlateAbsorption('wlength', outWS_n, sample[0], sample[1], sample[2],
        can[0], can[1], can[2], EMode='Indirect', EFixed=efixed,
        ElementSize=can[3])
    mantid.deleteWorkspace('wlength')

def absorption(input, mode, sample, can, Save=False, Verbose=False,
        Plot=False):
    (direct, filename) = os.path.split(input)
    (root, ext) = os.path.splitext(filename)
    LoadNexusProcessed(input, root)
    ws = mtd[root]
    det = ws.getDetector(0)
    efixed = 0.0
    try:
        efixed = det.getNumberParameter('Efixed')[0]
    except AttributeError: # detector group
        ids = det.getDetectorIDs()
        det = ws.getInstrument().getDetector(ids[0])
        efixed = det.getNumberParameter('Efixed')[0]
    outWS_n = root[:-3] + 'abs'
    if mode == 'Flat Plate':
        absflat(root, outWS_n, efixed, sample, can)
    if mode == 'Cylinder':
        abscyl(root, outWS_n, efixed, sample, can)
    if Save:
        SaveNexus(outWS_n, outWS_n+'.nxs')
    mantid.deleteWorkspace(root)
    if Plot:
        graph = plotSpectrum(outWS_n,0)

def concatWSs(workspaces, unit, name):
    dataX = []
    dataY = []
    dataE = []
    for ws in workspaces:
        readX = mtd[ws].readX(0)
        readY = mtd[ws].readY(0)
        readE = mtd[ws].readE(0)
        for i in range(0, len(readX)):
            dataX.append(readX[i])
        for i in range(0, len(readY)):
            dataY.append(readY[i])
        for i in range(0, len(readE)):
            dataE.append(readE[i])
    CreateWorkspace(name, dataX, dataY, dataE, NSpec = len(workspaces),
        UnitX=unit)

def demon(rawFiles, first, last, Smooth=False, SumFiles=False, CleanUp=True,
        Verbose=False, Plot=False, Save=True):
    ws_list, ws_names = loadData(rawFiles, Sum=SumFiles)
    runNos = []
    workspaces = []
    (direct, filename) = os.path.split(rawFiles[0])
    (root, ext) = os.path.splitext(filename)
    try:
        inst = mtd[ws_names[0]].getInstrument()
        area = inst.getNumberParameter('mon-area')[0]
        thickness = inst.getNumberParameter('mon-thickness')[0]
    except IndexError:
        sys.exit('Monitor area and thickness (unt and zz) are not defined \
                in the Instrument Parameter File.')
    for i in range(0, len(ws_names)):
        # Get Monitor WS
        MonitorWS = timeRegime(inWS=ws_names[i], Smooth=Smooth)
        monitorEfficiency(MonitorWS, area, thickness)
        # Get Run no, crop file
        runNo = ws_list[i].getRun().getLogData("run_number").value
        runNos.append(runNo)
        savefile = root[:3] + runNo + '_dem'
        CropWorkspace(ws_names[i], ws_names[i], StartWorkspaceIndex=(first-1),
                EndWorkspaceIndex = (last-1) )
        # Normalise to Monitor
        normalised = normToMon(inWS_n=ws_names[i], outWS_n=ws_names[i],
            monWS_n=MonitorWS)
        # Convert to dSpacing
        ConvertUnits(ws_names[i], savefile, 'dSpacing')
        workspaces.append(savefile)
        if CleanUp:
            mantid.deleteWorkspace(ws_names[i])
        if Save:
            SaveNexusProcessed(savefile, savefile+'.nxs')
    if Plot:
        for demon in workspaces:
            nspec = mtd[demon].getNumberHistograms()
            plotSpectrum(demon, range(0, nspec))
    return workspaces, runNos

def elwin(inputFiles, eRange, Save=False, Verbose=False, Plot=False):
    eq1 = [] # output workspaces with units in Q
    eq2 = [] # output workspaces with units in Q^2
    for file in inputFiles:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        LoadNexus(file, root)
        run = mtd[root].getRun().getLogData("run_number").value
        savefile = root[:3] + run + root[8:-3]
        if ( len(eRange) == 4 ):
            ElasticWindow(root, savefile+'eq1', savefile+'eq2',eRange[0],
                eRange[1], eRange[2], eRange[3])
        elif ( len(eRange) == 2 ):
            ElasticWindow(root, savefile+'eq1', savefile+'eq2', 
            eRange[0], eRange[1])
        if Save:
            SaveNexusProcessed(savefile+'eq1', savefile+'eq1.nxs')
            SaveNexusProcessed(savefile+'eq2', savefile+'eq2.nxs')
        eq1.append(savefile+'eq1')
        eq2.append(savefile+'eq2')
        mantid.deleteWorkspace(root)
    if Plot:
        graph1 = plotSpectrum(eq1, 0)
        graph2 = plotSpectrum(eq2, 0)
    return eq1, eq2

def fury(sam_files, res_file, rebinParam, RES=True, Save=False, Verbose=False,
        Plot=False):
    outWSlist = []
    # Process RES Data Only Once
    LoadNexus(res_file, 'res_data') # RES
    Rebin('res_data', 'res_data', rebinParam)
    ExtractFFTSpectrum('res_data', 'res_fft', 2)
    Integration('res_data', 'res_int')
    Divide('res_fft', 'res_int', 'res')
    for sam_file in sam_files:
        (direct, filename) = os.path.split(sam_file)
        (root, ext) = os.path.splitext(filename)
        if (ext == '.nxs'):
            LoadNexus(sam_file, 'sam_data') # SAMPLE
            Rebin('sam_data', 'sam_data', rebinParam)
        else: #input is workspace
            Rebin(sam_file, 'sam_data', rebinParam)
        ExtractFFTSpectrum('sam_data', 'sam_fft', 2)
        Integration('sam_data', 'sam_int')
        Divide('sam_fft', 'sam_int', 'sam')
        # Create save file name.
        runNo = mtd['sam_data'].getRun().getLogData("run_number").value
        savefile = root[:3] + runNo + root[8:-3] + 'iqt'
        outWSlist.append(savefile)
        Divide('sam', 'res', savefile)
        #Cleanup Sample Files
        mantid.deleteWorkspace('sam_data')
        mantid.deleteWorkspace('sam_int')
        mantid.deleteWorkspace('sam_fft')
        mantid.deleteWorkspace('sam')
        # Crop nonsense values off workspace
        bin = int(math.ceil(mtd[savefile].getNumberBins()/ 2.0))
        binV = mtd[savefile].dataX(0)[bin]
        CropWorkspace(savefile, savefile, XMax=binV)
        if Save:
            SaveNexusProcessed(savefile, savefile + '.nxs')
    # Clean Up RES files
    mantid.deleteWorkspace('res_data')
    mantid.deleteWorkspace('res_int')
    mantid.deleteWorkspace('res_fft')
    mantid.deleteWorkspace('res')
    if Plot:
        specrange = range(0,mtd[outWSlist[0]].getNumberHistograms())
        plotFury(outWSlist, specrange)
    return outWSlist

def furyfitSeq(inputWS,func,startx,endx):
    input = inputWS+',i0'
    nHist = mtd[inputWS].getNumberHistograms()
    for i in range(1,nHist):
        input += ';'+inputWS+',i'+str(i)
    outNm = inputWS + '_fitParameters'
    PlotPeakByLogValue(input, outNm, func, StartX=startx, EndX=endx)
    procSeqParToWS(outNm)

def procSeqParToWS(inputWS):
    ws = mtd[inputWS]
    dataX = []
    dataY = []
    dataE = []
    cCount = ws.getColumnCount()
    rCount = ws.getRowCount()
    cName =  ws.getColumnNames()
    nSpec = ( cCount - 1 ) / 2
    xAxis = cName[0]
    for spec in range(0,nSpec):
        yAxis = cName[(spec*2)+1]
        eAxis = cName[(spec*2)+2]
        for row in range(0, rCount):
            dataX.append(ws.getDouble(xAxis,row))
            dataY.append(ws.getDouble(yAxis,row))
            dataE.append(ws.getDouble(eAxis,row))
    CreateWorkspace(inputWS+'_matrix', dataX, dataY, dataE, nSpec)

def msdfit(inputs, startX, endX, Save=False, Verbose=False, Plot=False):
    output = []
    for file in inputs:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        LoadNexusProcessed(file, root)
        outWS_n = root[:-3] + 'msd'
        fit_alg = Linear(root, outWS_n, WorkspaceIndex=0, StartX=startX, 
            EndX=endX)
        output.append(outWS_n)
        A0 = fit_alg.getPropertyValue("FitIntercept")
        A1 = fit_alg.getPropertyValue("FitSlope")
        title = 'Intercept: '+A0+' ; Slope: '+A1
        if Plot:
            graph=plotSpectrum([root,outWS_n],0, 1)
            graph.activeLayer().setTitle(title)
        if Save:
            SaveNexusProcessed(outWS_n, outWS_n+'.nxs', Title=title)
    return output

def mut(inWS_n, deltaW, filename, efixed):
    file_handle = open(filename, 'w') # Open File
    sigs = 5.0 # sigs ?
    siga = 1.0 # siga ?
    we = math.sqrt(81.787/efixed) # 81.787 ?
    tempWS = '_tmp_indirect_mut_file_'
    ConvertUnits(inWS_n, tempWS, 'Wavelength', 'Indirect', efixed)
    xValues = mtd[tempWS].readX(0)
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
    inWS = mtd[inWS_n[0]]
    nbins = inWS.getNumberBins()
    lastValueZero = False
    graph = plotSpectrum(inWS_n, spec)
    layer = graph.activeLayer()
    layer.setScale(0, 0, 1.0)

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

def plotInput(inputfiles,spectra=[]):
    if len(spectra) != 2:
        sys.exit(1)
    workspaces = []
    for file in inputfiles:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        LoadNexusProcessed(file, root)
        GroupDetectors(root,root,DetectorList=range(spectra[0],spectra[1]+1))
        workspaces.append(root)
    if len(workspaces) > 0:
        graph = plotSpectrum(workspaces,0)
        layer = graph.activeLayer().setTitle(", ".join(workspaces))
