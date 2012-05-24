from mantidsimple import *
import mantidplot as mp
from IndirectCommon import *
from mantid import config, logger
import math, re, os.path

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
            dataE.append(readE[i])
    CreateWorkspace(OutputWorkspace=name, DataX=dataX, DataY=dataY, DataE=dataE, 
        NSpec=len(workspaces), UnitX=unit)

def split(l, n):
    #Yield successive n-sized chunks from l.
    for i in xrange(0, len(l), n):
        yield l[i:i+n]

def segment(l, fromIndex, toIndex):
    for i in xrange(fromIndex, toIndex + 1):
        yield l[i]

def trimData(nSpec, vals, min, max):
    result = []
    chunkSize = len(vals) / nSpec
    assert min >= 0, 'trimData: min is less then zero'
    assert max <= chunkSize - 1, 'trimData: max is greater than the number of spectra'
    assert min <= max, 'trimData: min is greater than max'
    chunks = split(vals,chunkSize)
    for chunk in chunks:
        seg = segment(chunk,min,max)
        for val in seg:
            result.append(val)
    return result

def confitParsToWS(Table, Data, BackG='FixF', specMin=0, specMax=-1):
    if ( specMax == -1 ):
        specMax = mtd[Data].getNumberHistograms() - 1
    dataX = createQaxis(Data)
    xAxisVals = []
    xAxisTrimmed = []
    dataY = []
    dataE = []
    names = ''
    ws = mtd[Table]
    cName =  ws.getColumnNames()
    nSpec = ( ws.getColumnCount() - 1 ) / 2
    for spec in range(0,nSpec):
        yAxis = cName[(spec*2)+1]
        if re.search('HWHM$', yAxis) or re.search('Height$', yAxis):
            xAxisVals += dataX
            if (len(names) > 0):
                names += ","
            names += yAxis
            eAxis = cName[(spec*2)+2]
            for row in range(0, ws.getRowCount()):
                dataY.append(ws.getDouble(yAxis,row))
                dataE.append(ws.getDouble(eAxis,row))
        else:
            nSpec -= 1
    suffix = str(nSpec / 2) + 'L' + BackG
    outNm = Table + suffix
    xAxisTrimmed = trimData(nSpec, xAxisVals, specMin, specMax)
    CreateWorkspace(OutputWorkspace=outNm, DataX=xAxisTrimmed, DataY=dataY, DataE=dataE, 
        Nspec=nSpec, UnitX='MomentumTransfer', VerticalAxisUnit='Text',
        VerticalAxisValues=names)
    return outNm

def confitPlotSeq(inputWS, plot):
    nhist = mtd[inputWS].getNumberHistograms()
    if ( plot == 'All' ):
        mp.plotSpectrum(inputWS, range(0, nhist), True)
        return    
    plotSpecs = []
    if ( plot == 'Intensity' ):
        res = 'Height$'
    elif ( plot == 'HWHM' ):
        res = 'HWHM$'
    for i in range(0,nhist):
        title = mtd[inputWS].getAxis(1).label(i)
        if re.search(res, title):
            plotSpecs.append(i)
    mp.plotSpectrum(inputWS, plotSpecs, True)

def confitSeq(inputWS, func, startX, endX, save, plot, bg, specMin, specMax):
    StartTime('ConvFit')
    Verbose = True
    workdir = config['defaultsave.directory']
    input = inputWS+',i' + str(specMin)
    if (specMax == -1):
        specMax = mtd[inputWS].getNumberHistograms() - 1
    for i in range(specMin + 1, specMax + 1):
        input += ';'+inputWS+',i'+str(i)
    outNm = getWSprefix(inputWS) + 'conv_'
    if Verbose:
        logger.notice(func)  
    PlotPeakByLogValue(Input=input, OutputWorkspace=outNm, Function=func, 
	    StartX=startX, EndX=endX)
    wsname = confitParsToWS(outNm, inputWS, bg, specMin, specMax)
    if save:
            SaveNexusProcessed(InputWorkspace=wsname, Filename=wsname+'.nxs')
    if plot != 'None':
        confitPlotSeq(wsname, plot)
    EndTime('ConvFit')

def elwin(inputFiles, eRange, Save=False, Verbose=True, Plot=False):
    StartTime('ElWin')
    Verbose = True
    workdir = config['defaultsave.directory']
    eq1 = [] # output workspaces with units in Q
    eq2 = [] # output workspaces with units in Q^2
    tempWS = '__temp'
    if Verbose:
        range1 = str(eRange[0])+' to '+str(eRange[1])
        if ( len(eRange) == 4 ): 
            range2 = str(eRange[2])+' to '+str(eRange[3])
            logger.notice('Using 2 energy ranges from '+range1+' & '+range2)
        elif ( len(eRange) == 2 ):
            logger.notice('Using 1 energy range from '+range1)
    for file in inputFiles:
        LoadNexus(Filename=file, OutputWorkspace=tempWS)
        if Verbose:
            logger.notice('Reading file : '+file)
        savefile = getWSprefix(tempWS)
        if ( len(eRange) == 4 ):
            ElasticWindow(InputWorkspace=tempWS, Range1Start=eRange[0], Range1End=eRange[1], 
                Range2Start=eRange[2], Range2End=eRange[3],
	            OutputInQ=savefile+'eq1', OutputInQSquared=savefile+'eq2')
        elif ( len(eRange) == 2 ):
            ElasticWindow(InputWorkspace=tempWS, Range1Start=eRange[0], Range1End=eRange[1],
                OutputInQ=savefile+'eq1', OutputInQSquared=savefile+'eq2')
        if Save:
            q1_path = os.path.join(workdir, savefile+'eq1.nxs')					# path name for nxs file
            SaveNexusProcessed(InputWorkspace=savefile+'eq1', Filename=q1_path)
            q2_path = os.path.join(workdir, savefile+'eq2.nxs')					# path name for nxs file
            SaveNexusProcessed(InputWorkspace=savefile+'eq2', Filename=q2_path)
            if Verbose:
                logger.notice('Creating file : '+q1_path)
                logger.notice('Creating file : '+q2_path)
        eq1.append(savefile+'eq1')
        eq2.append(savefile+'eq2')
        DeleteWorkspace(tempWS)
    if Plot:
        nBins = mtd[eq1[0]].blocksize()
        lastXeq1 = mtd[eq1[0]].readX(0)[nBins-1]
        graph1 = mp.plotSpectrum(eq1, 0)
        layer = graph1.activeLayer()
        layer.setScale(mp.Layer.Bottom, 0.0, lastXeq1)
        nBins = mtd[eq2[0]].blocksize()
        lastXeq2 = mtd[eq2[0]].readX(0)[nBins-1]
        graph2 = mp.plotSpectrum(eq2, 0)
        layer = graph2.activeLayer()
        layer.setScale(mp.Layer.Bottom, 0.0, lastXeq2)
    EndTime('Elwin')
    return eq1, eq2

def fury(sam_files, res_file, rebinParam, RES=True, Save=False, Verbose=False,
        Plot=False):
    StartTime('Fury')
    Verbose = True
    workdir = config['defaultsave.directory']
    outWSlist = []
    # Process RES Data Only Once
    if Verbose:
        logger.notice('Reading RES file : '+res_file)
    LoadNexus(Filename=res_file, OutputWorkspace='res_data') # RES
    Rebin(InputWorkspace='res_data', OutputWorkspace='res_data', Params=rebinParam)
    ExtractFFTSpectrum(InputWorkspace='res_data', OutputWorkspace='res_fft', FFTPart=2)
    Integration(InputWorkspace='res_data', OutputWorkspace='res_int')
    Divide(LHSWorkspace='res_fft', RHSWorkspace='res_int', OutputWorkspace='res')
    for sam_file in sam_files:
        (direct, filename) = os.path.split(sam_file)
        (root, ext) = os.path.splitext(filename)
        if (ext == '.nxs'):
            if Verbose:
                logger.notice('Reading sample file : '+sam_file)
            LoadNexus(Filename=sam_file, OutputWorkspace='sam_data') # SAMPLE
            Rebin(InputWorkspace='sam_data', OutputWorkspace='sam_data', Params=rebinParam)
        else: #input is workspace
            Rebin(InputWorkspace=sam_file, OutputWorkspace='sam_data', Params=rebinParam)
        ExtractFFTSpectrum(InputWorkspace='sam_data', OutputWorkspace='sam_fft', FFTPart=2)
        Integration(InputWorkspace='sam_data', OutputWorkspace='sam_int')
        Divide(LHSWorkspace='sam_fft', RHSWorkspace='sam_int', OutputWorkspace='sam')
        # Create save file name
        savefile = getWSprefix('sam_data') + 'iqt'
        outWSlist.append(savefile)
        Divide(LHSWorkspace='sam', RHSWorkspace='res', OutputWorkspace=savefile)
        #Cleanup Sample Files
        DeleteWorkspace('sam_data')
        DeleteWorkspace('sam_int')
        DeleteWorkspace('sam_fft')
        DeleteWorkspace('sam')
        # Crop nonsense values off workspace
        bin = int(math.ceil(mtd[savefile].getNumberBins()/ 2.0))
        binV = mtd[savefile].dataX(0)[bin]
        CropWorkspace(InputWorkspace=savefile, OutputWorkspace=savefile, XMax=binV)
        if Save:
            opath = os.path.join(workdir, savefile+'.nxs')					# path name for nxs file
            SaveNexusProcessed(InputWorkspace=savefile, Filename=opath)
            if Verbose:
                logger.notice('Output file : '+opath)  
    # Clean Up RES files
    DeleteWorkspace('res_data')
    DeleteWorkspace('res_int')
    DeleteWorkspace('res_fft')
    DeleteWorkspace('res')
    if Plot:
        specrange = range(0,mtd[outWSlist[0]].getNumberHistograms())
        plotFury(outWSlist, specrange)
    EndTime('Fury')
    return outWSlist
	
def furyfitParsToWS(Table, Data):
    dataX = createQaxis(Data)
    dataY = []
    dataE = []
    names = ""
    xAxisVals = []
    ws = mtd[Table]
    cCount = ws.getColumnCount()
    rCount = ws.getRowCount()
    cName =  ws.getColumnNames()
    nSpec = ( cCount - 1 ) / 2
    xAxis = cName[0]
    stretched = 0
    for spec in range(0,nSpec):
        yAxis = cName[(spec*2)+1]
        if ( re.search('Intensity$', yAxis) or re.search('Tau$', yAxis) or
            re.search('Beta$', yAxis) ):
            xAxisVals += dataX
            if (len(names) > 0):
                names += ","
            names += yAxis
            eAxis = cName[(spec*2)+2]
            for row in range(0, rCount):
                dataY.append(ws.getDouble(yAxis,row))
                dataE.append(ws.getDouble(eAxis,row))
            if ( re.search('Beta$', yAxis) ): # need to know how many of curves
                stretched += 1                # are stretched exponentials
        else:
            nSpec -= 1
    suffix = ''
    nE = ( nSpec / 2 ) - stretched
    if ( nE > 0 ):
        suffix += str(nE) + 'E'
    if ( stretched > 0 ):
        suffix += str(stretched) + 'S'
    wsname = Table + suffix
    CreateWorkspace(OutputWorkspace=wsname, DataX=xAxisVals, DataY=dataY, DataE=dataE, 
        Nspec=nSpec, UnitX='MomentumTransfer', VerticalAxisUnit='Text',
        VerticalAxisValues=names)
    return wsname

def furyfitPlotSeq(inputWS, Plot):
    nHist = mtd[inputWS].getNumberHistograms()
    if ( Plot == 'All' ):
        mp.plotSpectrum(inputWS, range(0, nHist), True)
        return
    plotSpecs = []
    if ( Plot == 'Intensity' ):
        res = 'Intensity$'
    if ( Plot == 'Tau' ):
        res = 'Tau$'
    elif ( Plot == 'Beta' ):
        res = 'Beta$'    
    for i in range(0, nHist):
        title = mtd[inputWS].getAxis(1).label(i)
        if ( re.search(res, title) ):
            plotSpecs.append(i)
    mp.plotSpectrum(inputWS, plotSpecs, True)

def furyfitSeq(inputWS, func, startx, endx, Save, Plot):
    StartTime('FuryFit')
    Verbose = True
    workdir = config['defaultsave.directory']
    input = inputWS+',i0'
    nHist = mtd[inputWS].getNumberHistograms()
    for i in range(1,nHist):
        input += ';'+inputWS+',i'+str(i)
    outNm = getWSprefix(inputWS) + 'fury_'
    if Verbose:
        logger.notice(func)  
    PlotPeakByLogValue(Input=input, OutputWorkspace=outNm, Function=func, 
        StartX=startx, EndX=endx)
    wsname = furyfitParsToWS(outNm, inputWS)
    if Save:
        opath = os.path.join(workdir, wsname+'.nxs')					# path name for nxs file
        SaveNexusProcessed(InputWorkspace=wsname, Filename=opath)
        if Verbose:
            logger.notice('Output file : '+opath)  
    if ( Plot != 'None' ):
        furyfitPlotSeq(wsname, Plot)
    EndTime('FuryFit')

def furyfitMultParsToWS(Table, Data):
    dataX = []
    dataY1 = []
    dataE1 = []
    dataY2 = []
    dataE2 = []
    dataY3 = []
    dataE3 = []
    ws = mtd[Table+'_Parameters']
    rCount = ws.getRowCount()
    nSpec = ( rCount - 1 ) / 5
    for spec in range(0,nSpec):
        n1 = spec*5
        rowi = n1 + 2                   #intensity
        rowt = n1 + 3                   #tau
        rowb = 4                        #beta
        dataX.append(spec)
        dataY1.append(ws.getDouble('Value',rowi))
        dataE1.append(ws.getDouble('Error',rowi))
        dataY2.append(ws.getDouble('Value',rowt))
        dataE2.append(ws.getDouble('Error',rowt))
        dataY3.append(ws.getDouble('Value',rowb))
        dataE3.append(ws.getDouble('Error',rowb))
    suffix = 'S'
    wsname = Table + '_' + suffix
    CreateWorkspace(OutputWorkspace=wsname, DataX=dataX, DataY=dataY1, DataE=dataE1, 
        Nspec=1, UnitX='MomentumTransfer', VerticalAxisUnit='Text',
        VerticalAxisValues='Intensity')
    CreateWorkspace(OutputWorkspace='__multmp', DataX=dataX, DataY=dataY2, DataE=dataE2, 
        Nspec=1, UnitX='MomentumTransfer', VerticalAxisUnit='Text',
        VerticalAxisValues='Tau')
    ConjoinWorkspaces(InputWorkspace1=wsname, InputWorkspace2='__multmp', CheckOverlapping=False)
    CreateWorkspace(OutputWorkspace='__multmp', DataX=dataX, DataY=dataY3, DataE=dataE3, 
        Nspec=1, UnitX='MomentumTransfer', VerticalAxisUnit='Text',
        VerticalAxisValues='Beta')
    ConjoinWorkspaces(InputWorkspace1=wsname, InputWorkspace2='__multmp', CheckOverlapping=False)
    return wsname

def furyfitPlotMult(inputWS, Plot):
    nHist = mtd[inputWS].getNumberHistograms()
    if ( Plot == 'All' ):
        mp.plotSpectrum(inputWS, range(0, nHist))
        return
    plotSpecs = []
    if ( Plot == 'Intensity' ):
        mp.plotSpectrum(inputWS, 0, True)
    if ( Plot == 'Tau' ):
        mp.plotSpectrum(inputWS, 1, True)
    elif ( Plot == 'Beta' ):
        mp.plotSpectrum(inputWS, 2, True)   

def furyfitMult(inputWS, func, startx, endx, Save, Plot):
    StartTime('FuryFit Mult')
    Verbose = True
    workdir = config['defaultsave.directory']
    input = inputWS+',i0'
    nHist = mtd[inputWS].getNumberHistograms()
    for i in range(1,nHist):
        input += ';'+inputWS+',i'+str(i)
    outNm = getWSprefix(inputWS) + 'fury'
    f1 = """(
        composite=CompositeFunctionMW,Workspace=$WORKSPACE$,WSParam=(WorkspaceIndex=$INDEX$);
        name=LinearBackground,A0=0,A1=0,ties=(A1=0);
        name=UserFunction,Formula=Intensity*exp(-(x/Tau)^Beta),Intensity=1.0,Tau=0.1,Beta=1;ties=(f1.Intensity=1-f0.A0)
    );
    """.replace('$WORKSPACE$',inputWS)
    func= 'composite=MultiBG;'
    ties='ties=('
    for i in range(0,nHist):
        func+=f1.replace('$INDEX$',str(i))
        if i > 0:
            ties += 'f' + str(i) + '.f1.Beta=f0.f1.Beta'
            if i < nHist-1:
                ties += ','
    ties+=')'
    func += ties
    logger.notice(func)
    Fit(InputWorkspace=inputWS,Function=func,Output=outNm)
    wsname = furyfitMultParsToWS(outNm, inputWS)
    if Save:
        opath = os.path.join(workdir, wsname+'.nxs')					# path name for nxs file
        SaveNexusProcessed(InputWorkspace=wsname, Filename=opath)
        if Verbose:
            logger.notice('Output file : '+opath)  
    if ( Plot != 'None' ):
        furyfitPlotMult(wsname, Plot)
    EndTime('FuryFit')

def msdfitParsToWS(Table, xData):
    dataX = xData
    dataY = []
    dataE = []
    ws = mtd[Table+'_Table']
    cCount = ws.getColumnCount()
    rCount = ws.getRowCount()
    cName =  ws.getColumnNames()
    yAxis = cName[3]
    eAxis = cName[4]
    for row in range(0,rCount):
        dataY.append(-ws.getDouble(yAxis,row))
        dataE.append(ws.getDouble(eAxis,row))
    wsname = Table
    CreateWorkspace(OutputWorkspace=wsname, DataX=dataX, DataY=dataY, DataE=dataE,
        Nspec=1, UnitX='')
    return wsname

def msdfitPlotSeq(inputWS, Plot, xlabel):
    msd_plot = mp.plotSpectrum(inputWS,0,True)
    msd_layer = msd_plot.activeLayer()
    msd_layer.setAxisTitle(mp.Layer.Bottom,xlabel)
    msd_layer.setAxisTitle(mp.Layer.Left,'<u2>')

def msdfit(inputs, startX, endX, Save=False, Verbose=True, Plot=False):
    StartTime('msdFit')
    Verbose = 'True'
    Plot = 'True'
    workdir = config['defaultsave.directory']
    log_type = 'sample'
    runs = sorted(inputs)
    file_list = []
    x_list = []
    np = 0
    for file in runs:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        file_list.append(root)
        if Verbose:
            logger.notice('Reading Run : '+file)
        LoadNexusProcessed(FileName=file, OutputWorkspace=root)
        inX = mtd[root].readX(0)
        inY = mtd[root].readY(0)
        inE = mtd[root].readE(0)
        logy = []
        loge = []
        for i in range(0, len(inY)):
            if(inY[i] == 0):
                ly = math.log(0.000000000001)
            else:
                ly = math.log(inY[i])
            logy.append(ly)
            if( inY[i]+inE[i] == 0 ):
                le = math.log(0.000000000001)-ly
            else:
                le = math.log(inY[i]+inE[i])-ly
            loge.append(le)
        lnWS = root[:-3] + 'lnI'
        CreateWorkspace(OutputWorkspace=lnWS, DataX=inX, DataY=logy, DataE=loge,
            Nspec=1)
        log_name = root[0:8]+'_'+log_type
        log_file = log_name+'.txt'
        log_path = FileFinder.getFullPath(log_file)
        if (log_path == ''):
            logger.notice(' Run : '+root[0:8] +' ; Temperature file not found')
            xval = int(root[5:8])
            xlabel = 'Run'
        else:			
            logger.notice('Found '+log_path)
            LoadLog(Workspace=root, Filename=log_path)
            run_logs = mtd[root].getRun()
            tmp = run_logs[log_name].value
            temp = tmp[len(tmp)-1]
            logger.notice(' Run : '+root[0:8] +' ; Temperature = '+str(temp))
            xval = temp
            xlabel = 'Temp'
        if (np == 0):
            first = root[0:8]
            run_list = lnWS
        else:
            last = root[0:8]
            run_list += ';'+lnWS
        x_list.append(xval)
        DeleteWorkspace(root)
        np += 1
    if Verbose:
       logger.notice('Fitting Runs '+first+' to '+last)
    function = 'name=LinearBackground, A0=0, A1=0'
    mname = first[0:8]+'_to_'+last[3:8]
    fitWS = mname+'_msd'
    PlotPeakByLogValue(Input=run_list, OutputWorkspace=fitWS+'_Table', Function=function,
        StartX=startX, EndX=endX, FitType = "Sequential")
    msdfitParsToWS(fitWS, x_list)
    np = 0
    outWS = mname+'_lnI'
    for ws in file_list:
        inWS = ws[:-3] + 'lnI'
        if (np == 0):
            CloneWorkspace(InputWorkspace=inWS,OutputWorkspace=outWS)
        else:
            ConjoinWorkspaces(InputWorkspace1=outWS, InputWorkspace2=inWS, CheckOverlapping=False)
        np += 1
    if Plot:
        msdfitPlotSeq(fitWS, Plot, xlabel)
    if Save:
        fit_path = os.path.join(workdir, fitWS+'.nxs')					# path name for nxs file
        SaveNexusProcessed(InputWorkspace=fitWS, Filename=fit_path, Title=fitWS)
        if Verbose:
            logger.notice('Output Fit file : '+fit_path)  
    EndTime('msdFit')
    return fitWS

def plotFury(inWS_n, spec):
    inWS = mtd[inWS_n[0]]
    nbins = inWS.getNumberBins()
    graph = mp.plotSpectrum(inWS_n, spec)
    layer = graph.activeLayer()
    layer.setScale(mp.Layer.Left, 0, 1.0)

def plotInput(inputfiles,spectra=[]):
    OneSpectra = False
    if len(spectra) != 2:
        spectra = [spectra[0], spectra[0]]
        OneSpectra = True
    workspaces = []
    for file in inputfiles:
        root = LoadNexus(Filename=file)
        if not OneSpectra:
            GroupDetectors(root, root,
                DetectorList=range(spectra[0],spectra[1]+1) )
        workspaces.append(root)
    if len(workspaces) > 0:
        graph = mp.plotSpectrum(workspaces,0)
        layer = graph.activeLayer().setTitle(", ".join(workspaces))
        
###############################################################################
## abscor #####################################################################
###############################################################################

def CubicFit(inputWS, spec, Verbose=False):
    '''Uses the Mantid Fit Algorithm to fit a quadratic to the inputWS
    parameter. Returns a list containing the fitted parameter values.'''
    function = 'name=Quadratic, A0=1, A1=0, A2=0'
    fit = Fit(Function=function, InputWorkspace=inputWS, WorkspaceIndex=spec,
      CreateOutput=True, Output='Fit')
    table = mtd['Fit_Parameters']
    A0 = table.getDouble('Value', 0)
    A1 = table.getDouble('Value', 1)
    A2 = table.getDouble('Value', 2)
    Abs = [A0, A1, A2]
    if Verbose:
       logger.notice('Group '+str(spec)+' of '+inputWS+' ; fit coefficients are : '+str(Abs))
    return Abs

def applyCorrections(inputWS, canWS, corr, Verbose=False):
    '''Through the PolynomialCorrection algorithm, makes corrections to the
    input workspace based on the supplied correction values.'''
    # Corrections are applied in Lambda (Wavelength)
    efixed = getEfixed(inputWS)                # Get efixed
    ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='Wavelength',
        EMode='Indirect', EFixed=efixed)
    corrections = [corr+'_1']
    CorrectedWS = inputWS[0:-3] +'Corrected'
    if canWS != '':
        corrections = [corr+'_1', corr+'_2', corr+'_3', corr+'_4']
        CorrectedWS = inputWS[0:-3] +'Correct_'+ canWS[3:8]
        ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='Wavelength',
            EMode='Indirect', EFixed=efixed)
    nHist = mtd[inputWS].getNumberHistograms()
    # Check that number of histograms in each corrections workspace matches
    # that of the input (sample) workspace
    for ws in corrections:
        if ( mtd[ws].getNumberHistograms() != nHist ):
            raise ValueError('Mismatch: num of spectra in '+ws+' and inputWS')
    # Workspaces that hold intermediate results
    CorrectedSampleWS = '__csam'
    CorrectedCanWS = '__ccan'
    for i in range(0, nHist): # Loop through each spectra in the inputWS
        ExtractSingleSpectrum(InputWorkspace=inputWS, OutputWorkspace=CorrectedSampleWS,
            WorkspaceIndex=i)
        if ( len(corrections) == 1 ):
            Ass = CubicFit(corrections[0], i, Verbose)
            PolynomialCorrection(InputWorkspace=CorrectedSampleWS, OutputWorkspace=CorrectedSampleWS,
                Coefficients=Ass, Operation='Divide')
            if ( i == 0 ):
                CloneWorkspace(InputWorkspace=CorrectedSampleWS, OutputWorkspace=CorrectedWS)
            else:
                ConjoinWorkspaces(InputWorkspace1=CorrectedWS, InputWorkspace2=CorrectedSampleWS)
        else:
            ExtractSingleSpectrum(InputWorkspace=canWS, OutputWorkspace=CorrectedCanWS,
                WorkspaceIndex=i)
            Acc = CubicFit(corrections[3], i, Verbose)
            PolynomialCorrection(InputWorkspace=CorrectedCanWS, OutputWorkspace=CorrectedCanWS,
                Coefficients=Acc, Operation='Divide')
            Acsc = CubicFit(corrections[2], i, Verbose)
            PolynomialCorrection(InputWorkspace=CorrectedCanWS, OutputWorkspace=CorrectedCanWS,
                Coefficients=Acsc, Operation='Multiply')
            Minus(LHSWorkspace=CorrectedSampleWS, RHSWorkspace=CorrectedCanWS, OutputWorkspace=CorrectedSampleWS)
            Assc = CubicFit(corrections[1], i, Verbose)
            PolynomialCorrection(InputWorkspace=CorrectedSampleWS, OutputWorkspace=CorrectedSampleWS,
                Coefficients=Assc, Operation='Divide')
            if ( i == 0 ):
                CloneWorkspace(InputWorkspace=CorrectedSampleWS, OutputWorkspace=CorrectedWS)
            else:
                ConjoinWorkspaces(InputWorkspace1=CorrectedWS, InputWorkspace2=CorrectedSampleWS)
    ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='DeltaE',
        EMode='Indirect', EFixed=efixed)
    ConvertUnits(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS, Target='DeltaE',
        EMode='Indirect', EFixed=efixed)
    if canWS != '':
        DeleteWorkspace(CorrectedCanWS)
        ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='DeltaE',
            EMode='Indirect', EFixed=efixed)
    DeleteWorkspace('Fit_NormalisedCovarianceMatrix')
    DeleteWorkspace('Fit_Parameters')
    DeleteWorkspace('Fit_Workspace')
    DeleteWorkspace('corrections')
    return CorrectedWS
                
def abscorFeeder(sample, container, geom, useCor):
    '''Load up the necessary files and then passes them into the main
    applyCorrections routine.'''
    StartTime('ApplyCorrections')
    Verbose = True
    PlotResult = 'Both'
    PlotContrib = 'Spectrum'
    workdir = config['defaultsave.directory']
    s_hist = mtd[sample].getNumberHistograms()       # no. of hist/groups in sam
    Xin = mtd[sample].readX(0)
    sxlen = len(Xin)
    if container != '':
        c_hist = mtd[container].getNumberHistograms()
        Xin = mtd[container].readX(0)
        cxlen = len(Xin)
        if s_hist != c_hist:	# check that no. groups are the same
            error = 'Can histograms (' +str(c_hist) + ') not = Sample (' +str(s_hist) +')'	
            exit(error)
        else:
            if sxlen != cxlen:	# check that array lengths are the same
                error = 'Can array length (' +str(cxlen) + ') not = Sample (' +str(sxlen) +')'	
                exit(error)
    if useCor:
        if Verbose:
            logger.notice('Correcting sample ' + sample + ' with ' + container)
        file = sample[:-3] + geom +'_Abs.nxs'
        abs_path = os.path.join(workdir, file)					# path name for nxs file
        if Verbose:
            logger.notice('Correction file :'+abs_path)
        LoadNexus(Filename=abs_path, OutputWorkspace='corrections')
        cor_result = applyCorrections(sample, container, 'corrections', Verbose)
        cor_path = os.path.join(workdir,cor_result+'.nxs')
        SaveNexusProcessed(InputWorkspace=cor_result,Filename=cor_path)
        if Verbose:
            logger.notice('Output file created : '+cor_path)
        plot_list = [cor_result,sample]
        if ( container != '' ):
            plot_list.append(container)
        if (PlotResult != 'None'):
            plotCorrResult(cor_result,PlotResult)
        if (PlotContrib != 'None'):
            plotCorrContrib(plot_list,0,PlotContrib)
    else:
        if ( container == '' ):
            sys.exit('Invalid options - nothing to do!')
        else:
            sub_result = sample[0:8] +'_Subtract_'+ container[3:8]
            Minus(LHSWorkspace=sample,RHSWorkspace=container,OutputWorkspace=sub_result)
            sub_path = os.path.join(workdir,sub_result+'.nxs')
            SaveNexusProcessed(InputWorkspace=sub_result,Filename=sub_path)
            if Verbose:
	            logger.notice('Subtracting '+container+' from '+sample)
	            logger.notice('Output file created : '+sub_path)
            if (PlotResult != 'None'):
                plotCorrResult(sub_result)
            if (PlotResult != 'None'):
                sub_plot=plotCorrContrib([sub_result,sample,container],0)
    EndTime('ApplyCorrections')

def plotCorrResult(inWS,PlotResult):
    if (PlotResult !='None'):
        if (PlotResult == 'Spectrum' or PlotResult == 'Both'):
            nHist = mtd[inWS].getNumberHistograms()
            plot_list = []
            for i in range(0, nHist):
                plot_list.append(i)
            res_plot=mp.plotSpectrum(inWS,plot_list)
        if (PlotResult == 'Contour' or PlotResult == 'Both'):
            mp.importMatrixWorkspace(inWS).plotGraph2D()

def plotCorrContrib(plot_list,n,PlotContrib):
        con_plot=mp.plotSpectrum(plot_list,0)
