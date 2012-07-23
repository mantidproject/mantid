from mantid.simpleapi import *
from IndirectImport import import_mantidplot
mp = import_mantidplot()
from IndirectCommon import *
from mantid import config, logger
import math, re, os.path

##############################################################################
# Misc. Helper Functions
##############################################################################

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

##############################################################################
# ConvFit
##############################################################################

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
    nSpec = ( ws.columnCount() - 1 ) / 2
    for spec in range(0,nSpec):
        yCol = (spec*2)+1
        yAxis = cName[(spec*2)+1]
        if re.search('HWHM$', yAxis) or re.search('Height$', yAxis):
            xAxisVals += dataX
            if (len(names) > 0):
                names += ","
            names += yAxis
            eCol = (spec*2)+2
            eAxis = cName[(spec*2)+2]
            for row in range(0, ws.rowCount()):
                dataY.append(ws.cell(row,yCol))
                dataE.append(ws.cell(row,eCol))
        else:
            nSpec -= 1
    outNm = Table + "_Workspace"
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

def confitSeq(inputWS, func, startX, endX, save, plot, ftype, bg, specMin, specMax, Verbose=True):
    StartTime('ConvFit')
    workdir = config['defaultsave.directory']
    input = inputWS+',i' + str(specMin)
    if (specMax == -1):
        specMax = mtd[inputWS].getNumberHistograms() - 1
    for i in range(specMin + 1, specMax + 1):
        input += ';'+inputWS+',i'+str(i)
    outNm = getWSprefix(inputWS) + 'conv_' + ftype + bg + "_s" + str(specMin) + "_to_" + str(specMax)
    if Verbose:
        logger.notice(func)  
    PlotPeakByLogValue(Input=input, OutputWorkspace=outNm, Function=func, 
	    StartX=startX, EndX=endX, FitType='Sequential')
    wsname = confitParsToWS(outNm, inputWS, bg, specMin, specMax)
    RenameWorkspace(InputWorkspace=outNm,
                    OutputWorkspace=outNm + "_Parameters")
    if save:
        SaveNexusProcessed(InputWorkspace=wsname, Filename=wsname+'.nxs')
    if plot != 'None':
        confitPlotSeq(wsname, plot)
    EndTime('ConvFit')

##############################################################################
# Elwin
##############################################################################

def elwin(inputFiles, eRange, Save=False, Verbose=True, Plot=False):
    StartTime('ElWin')
    Verbose = True
    workdir = config['defaultsave.directory']
    CheckXrange(eRange,'Energy')
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
        nsam,ntc = CheckHistZero(tempWS)
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
        elwinPlot(eq1,eq2)
    EndTime('Elwin')
    return eq1, eq2

def elwinPlot(eq1,eq2):
    nBins = mtd[eq1[0]].blocksize()
    if nBins >= 10:
        nBins = 10
    lastXeq1 = mtd[eq1[0]].readX(0)[nBins-1]
    graph1 = mp.plotSpectrum(eq1, 0)
    layer = graph1.activeLayer()
    layer.setScale(mp.Layer.Bottom, 0.0, lastXeq1)
    nBins = mtd[eq2[0]].blocksize()
    if nBins >= 10:
        nBins = 10
    lastXeq2 = mtd[eq2[0]].readX(0)[nBins-1]
    graph2 = mp.plotSpectrum(eq2, 0)
    layer = graph2.activeLayer()
    layer.setScale(mp.Layer.Bottom, 0.0, lastXeq2)

##############################################################################
# Fury
##############################################################################

def furyPlot(inWS, spec):
    graph = mp.plotSpectrum(inWS, spec)
    layer = graph.activeLayer()
    layer.setScale(mp.Layer.Left, 0, 1.0)

def fury(sam_files, res_file, rebinParam, RES=True, Save=False, Verbose=False,
        Plot=False):
    StartTime('Fury')
    Verbose = True
    workdir = config['defaultsave.directory']
    LoadNexus(Filename=sam_files[0], OutputWorkspace='__sam_tmp') # SAMPLE
    nsam,npt = CheckHistZero('__sam_tmp')
    Xin = mtd['__sam_tmp'].readX(0)
    d1 = Xin[1]-Xin[0]
    if d1 < 1e-8:
        error = 'Data energy bin is zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    d2 = Xin[npt-1]-Xin[npt-2]
    dmin = min(d1,d2)
    pars = rebinParam.split(',')
    if (float(pars[1]) <= dmin):
        error = 'EWidth = ' + pars[1] + ' < smallest Eincr = ' + str(dmin)
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    outWSlist = []
    # Process RES Data Only Once
    if Verbose:
        logger.notice('Reading RES file : '+res_file)
    LoadNexus(Filename=res_file, OutputWorkspace='res_data') # RES
    CheckAnalysers('__sam_tmp','res_data',Verbose)
    nres,nptr = CheckHistZero('res_data')
    if nres > 1:
        CheckHistSame('__sam_tmp','Sample','res_data','Resolution')
    DeleteWorkspace('__sam_tmp')
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
        bin = int(math.ceil(mtd[savefile].blocksize()/2.0))
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
        furyPlot(outWSlist, specrange)
    EndTime('Fury')
    return outWSlist

##############################################################################
# FuryFit
##############################################################################

def furyfitParsToWS(Table, Data):
    dataX = createQaxis(Data)
    dataY = []
    dataE = []
    names = ""
    xAxisVals = []
    ws = mtd[Table]
    cCount = ws.columnCount()
    rCount = ws.rowCount()
    cName =  ws.getColumnNames()
    nSpec = ( cCount - 1 ) / 2
    yA0 = ws.column(1)
    eA0 = ws.column(2)
    logger.notice(str(yA0))
    logger.notice(str(eA0))
    xAxis = cName[0]
    stretched = 0
    for spec in range(0,nSpec):
        yCol = (spec*2)+1
        yAxis = cName[(spec*2)+1]
        if ( re.search('Intensity$', yAxis) or re.search('Tau$', yAxis) or
            re.search('Beta$', yAxis) ):
            xAxisVals += dataX
            if (len(names) > 0):
                names += ","
            names += yAxis
            eCol = (spec*2)+2
            eAxis = cName[(spec*2)+2]
            for row in range(0, rCount):
                dataY.append(ws.cell(row,yCol))
                dataE.append(ws.cell(row,eCol))
            if ( re.search('Beta$', yAxis) ): # need to know how many of curves
                stretched += 1                # are stretched exponentials
        else:
            nSpec -= 1
    wsname = Table + "_Workspace"
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

def furyfitSeq(inputWS, func, ftype, startx, endx, Save, Plot, Verbose = True):
    StartTime('FuryFit')
    workdir = config['defaultsave.directory']
    input = inputWS+',i0'
    nHist = mtd[inputWS].getNumberHistograms()
    for i in range(1,nHist):
        input += ';'+inputWS+',i'+str(i)
    outNm = getWSprefix(inputWS) + 'fury_' + ftype + "0_to_" + str(nHist-1)
    if Verbose:
        logger.notice(func)  
    PlotPeakByLogValue(Input=input, OutputWorkspace=outNm, Function=func, 
        StartX=startx, EndX=endx, FitType='Sequential')
    wsname = furyfitParsToWS(outNm, inputWS)
    RenameWorkspace(InputWorkspace=outNm, OutputWorkspace=outNm+"_Parameters")
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
    rCount = ws.rowCount()
    nSpec = ( rCount - 1 ) / 5
    for spec in range(0,nSpec):
        n1 = spec*5
        rowi = n1 + 2                   #intensity
        ival = 5                   #intensity value
        ierr = 6                   #intensity error
        tval = 7                   #tau value
        terr = 8                   #tau error
        rowt = n1 + 3                   #tau
        rowb = 4                        #beta
        bval = 9                   #beta value
        bval = 10                   #beta error
        dataX.append(spec)
        dataY1.append(ws.cell(spec,ival))
        dataE1.append(ws.cell(spec,ierr))
        dataY2.append(ws.cell(spec,tval))
        dataE2.append(ws.cell(spec,terr))
        dataY3.append(ws.cell(spec,bval))
        dataE3.append(ws.cell(spec,berr))
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

##############################################################################
# MSDFit
##############################################################################

def msdfitParsToWS(Table, xData):
    dataX = xData
    ws = mtd[Table+'_Table']
    rCount = ws.rowCount()
    yA0 = ws.column(1)
    eA0 = ws.column(2)
    yA1 = ws.column(3)  
    dataY1 = map(lambda x : -x, yA1) 
    eA1 = ws.column(4)
    wsname = Table
    CreateWorkspace(OutputWorkspace=wsname+'_a0', DataX=dataX, DataY=yA0, DataE=eA0,
        Nspec=1, UnitX='')
    CreateWorkspace(OutputWorkspace=wsname+'_a1', DataX=dataX, DataY=dataY1, DataE=eA1,
        Nspec=1, UnitX='')
    group = wsname+'_a0,'+wsname+'_a1'
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=wsname)
    return wsname

def msdfitPlotSeq(inputWS, xlabel):
    msd_plot = mp.plotSpectrum(inputWS+'_a1',0,True)
    msd_layer = msd_plot.activeLayer()
    msd_layer.setAxisTitle(mp.Layer.Bottom,xlabel)
    msd_layer.setAxisTitle(mp.Layer.Left,'<u2>')

def msdfitPlotFits(lniWS, fitWS, n):
    mfit_plot = mp.plotSpectrum(lniWS,n,True)
    mp.mergePlots(mfit_plot,mp.plotSpectrum(fitWS+'_line',n,False))

def msdfit(inputs, startX, endX, Save=False, Verbose=True, Plot=True):
    StartTime('msdFit')
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
        nsam,ntc = CheckHistZero(root)
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
            last = root[0:8]
            run_list = lnWS
        else:
            last = root[0:8]
            run_list += ';'+lnWS
        x_list.append(xval)
        DeleteWorkspace(root)
        np += 1
    if Verbose:
       logger.notice('Fitting Runs '+first+' to '+last)
       logger.notice('Q-range from '+str(startX)+' to '+str(endX))
    function = 'name=LinearBackground, A0=0, A1=0'
    mname = first[0:8]+'_to_'+last[3:8]
    msdWS = mname+'_msd'
    PlotPeakByLogValue(Input=run_list, OutputWorkspace=msdWS+'_Table', Function=function,
        StartX=startX, EndX=endX, FitType = 'Sequential')
    msdfitParsToWS(msdWS, x_list)
    np = 0
    lniWS = mname+'_lnI'
    fitWS = mname+'_Fit'
    a0 = mtd[msdWS+'_a0'].readY(0)
    a1 = mtd[msdWS+'_a1'].readY(0)
    for ws in file_list:
        inWS = ws[:-3] + 'lnI'
        CropWorkspace(InputWorkspace=inWS,OutputWorkspace='__data',XMin=startX,XMax=endX)
        xin = mtd['__data'].readX(0)
        nxd = len(xin)-1
        xd = []
        yd = []
        ed = []
        for n in range(0,nxd):
            line = a0[np] - a1[np]*xin[n]
            xd.append(xin[n])
            yd.append(line)
            ed.append(0.0)
            CreateWorkspace(OutputWorkspace='__line', DataX=xd, DataY=yd, DataE=ed,
				Nspec=1)
        if (np == 0):
            RenameWorkspace(InputWorkspace=inWS,OutputWorkspace=lniWS)
            RenameWorkspace(InputWorkspace='__data',OutputWorkspace=fitWS+'_data')
            RenameWorkspace(InputWorkspace='__line',OutputWorkspace=fitWS+'_line')
        else:
            ConjoinWorkspaces(InputWorkspace1=lniWS, InputWorkspace2=inWS, CheckOverlapping=False)
            ConjoinWorkspaces(InputWorkspace1=fitWS+'_data', InputWorkspace2='__data', CheckOverlapping=False)
            ConjoinWorkspaces(InputWorkspace1=fitWS+'_line', InputWorkspace2='__line', CheckOverlapping=False)
        np += 1
        group = fitWS+'_data,'+ fitWS+'_line'
        GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fitWS)
    if Plot:
        msdfitPlotSeq(msdWS, xlabel)
        msdfitPlotFits(lniWS, fitWS, 0)
    if Save:
        msd_path = os.path.join(workdir, msdWS+'.nxs')					# path name for nxs file
        SaveNexusProcessed(InputWorkspace=msdWS, Filename=msd_path, Title=msdWS)
        if Verbose:
            logger.notice('Output msd file : '+msd_path)  
    EndTime('msdFit')
    return msdWS

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
        
##############################################################################
# Corrections
##############################################################################

def CubicFit(inputWS, spec, Verbose=False):
    '''Uses the Mantid Fit Algorithm to fit a quadratic to the inputWS
    parameter. Returns a list containing the fitted parameter values.'''
    function = 'name=Quadratic, A0=1, A1=0, A2=0'
    fit = Fit(Function=function, InputWorkspace=inputWS, WorkspaceIndex=spec,
      CreateOutput=True, Output='Fit')
    table = mtd['Fit_Parameters']
    A0 = table.cell(0,1)
    A1 = table.cell(1,1)
    A2 = table.cell(2,1)
    Abs = [A0, A1, A2]
    if Verbose:
       logger.notice('Group '+str(spec)+' of '+inputWS+' ; fit coefficients are : '+str(Abs))
    return Abs

def applyCorrections(inputWS, canWS, corr, Verbose=False):
    '''Through the PolynomialCorrection algorithm, makes corrections to the
    input workspace based on the supplied correction values.'''
    # Corrections are applied in Lambda (Wavelength)
    efixed = getEfixed(inputWS)                # Get efixed
    theta,Q = GetThetaQ(inputWS)
    ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='Wavelength',
        EMode='Indirect', EFixed=efixed)
    if canWS != '':
        corrections = [corr+'_1', corr+'_2', corr+'_3', corr+'_4']
        CorrectedWS = inputWS[0:-3] +'Correct_'+ canWS[3:8]
        ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='Wavelength',
            EMode='Indirect', EFixed=efixed)
    else:
        corrections = [corr+'_1']
        CorrectedWS = inputWS[0:-3] +'Corrected'
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
    CloneWorkspace(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS+'_sqw')
    replace_workspace_axis(CorrectedWS+'_sqw', Q)
    RenameWorkspace(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS+'_red')
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
    Save = True
    PlotResult = 'Both'
    PlotContrib = 'Spectrum'
    workdir = config['defaultsave.directory']
    CheckAnalysers(sample,container,Verbose)
    s_hist,sxlen = CheckHistZero(sample)
    if container != '':
        CheckHistSame(sample,'Sample',container,'Container')
    if useCor:
        if Verbose:
            text = 'Correcting sample ' + sample
            if container != '':
                text += ' with ' + container
            logger.notice(text)
        file = sample[:-3] + geom +'_Abs.nxs'
        abs_path = os.path.join(workdir, file)					# path name for nxs file
        if Verbose:
            logger.notice('Correction file :'+abs_path)
        LoadNexus(Filename=abs_path, OutputWorkspace='corrections')
        cor_result = applyCorrections(sample, container, 'corrections', Verbose)
        if Save:
            cred_path = os.path.join(workdir,cor_result+'_red.nxs')
            SaveNexusProcessed(InputWorkspace=cor_result+'_red',Filename=cred_path)
            csqw_path = os.path.join(workdir,cor_result+'_sqw.nxs')
            SaveNexusProcessed(InputWorkspace=cor_result+'_sqw',Filename=csqw_path)
            if Verbose:
                logger.notice('Output file created : '+cred_path)
                logger.notice('Output file created : '+csqw_path)
        plot_list = [cor_result+'_red',sample]
        if ( container != '' ):
            plot_list.append(container)
        if (PlotResult != 'None'):
            plotCorrResult(cor_result+'_sqw',PlotResult)
        if (PlotContrib != 'None'):
            plotCorrContrib(plot_list,0)
    else:
        if ( container == '' ):
            sys.exit('ERROR *** Invalid options - nothing to do!')
        else:
            sub_result = sample[:-3] +'Subtract_'+ container[3:8]
            if Verbose:
	            logger.notice('Subtracting '+container+' from '+sample)
            Minus(LHSWorkspace=sample,RHSWorkspace=container,OutputWorkspace=sub_result)
            CloneWorkspace(InputWorkspace=sub_result, OutputWorkspace=sub_result+'_sqw')
            theta,Q = GetThetaQ(sample)
            replace_workspace_axis(sub_result+'_sqw', Q)
            RenameWorkspace(InputWorkspace=sub_result, OutputWorkspace=sub_result+'_red')
            if Save:
                sred_path = os.path.join(workdir,sub_result+'_red.nxs')
                SaveNexusProcessed(InputWorkspace=sub_result+'_red',Filename=sred_path)
                ssqw_path = os.path.join(workdir,sub_result+'_sqw.nxs')
                SaveNexusProcessed(InputWorkspace=sub_result+'_sqw',Filename=ssqw_path)
                if Verbose:
	                logger.notice('Output file created : '+sred_path)
	                logger.notice('Output file created : '+ssqw_path)
            plot_list = [sub_result+'_red',sample]
            if (PlotResult != 'None'):
                plotCorrResult(sub_result+'_sqw',PlotResult)
            if (PlotResult != 'None'):
                plotCorrContrib(plot_list,0)
    EndTime('ApplyCorrections')

def plotCorrResult(inWS,PlotResult):
    nHist = mtd[inWS].getNumberHistograms()
    if (PlotResult == 'Spectrum' or PlotResult == 'Both'):
        if nHist >= 10:
            nHist = 10
        plot_list = []
        for i in range(0, nHist):
            plot_list.append(i)
        res_plot=mp.plotSpectrum(inWS,plot_list)
    if (PlotResult == 'Contour' or PlotResult == 'Both'):
        if nHist >= 5:
            mp.importMatrixWorkspace(inWS).plotGraph2D()

def plotCorrContrib(plot_list,n):
        con_plot=mp.plotSpectrum(plot_list,n)

def replace_workspace_axis(wsName, new_values):
    from mantidsimple import createNumericAxis, mtd        #temporary use of old API
    ax1 = createNumericAxis(len(new_values))
    for i in range(len(new_values)):
        ax1.setValue(i, new_values[i])
    ax1.setUnit('MomentumTransfer')
    ws = mtd[wsName]
    ws.replaceAxis(1, ax1)
