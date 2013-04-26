from mantid.simpleapi import *
from IndirectImport import import_mantidplot
mp = import_mantidplot()
from IndirectCommon import *
from mantid import config, logger
import math, re, os.path, numpy as np

##############################################################################
# Misc. Helper Functions
##############################################################################

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

def confitPlotSeq(inputWS, Plot):
    nhist = mtd[inputWS].getNumberHistograms()
    if ( Plot == 'All' ):
        mp.plotSpectrum(inputWS, range(0, nhist), True)
        return    
    plotSpecs = []
    if ( Plot == 'Intensity' ):
        res = 'Height$'
    elif ( Plot == 'HWHM' ):
        res = 'HWHM$'
    for i in range(0,nhist):
        title = mtd[inputWS].getAxis(1).label(i)
        if re.search(res, title):
            plotSpecs.append(i)
    mp.plotSpectrum(inputWS, plotSpecs, True)

def confitSeq(inputWS, func, startX, endX, Save, Plot, ftype, bg, specMin, specMax, Verbose):
    StartTime('ConvFit')
    workdir = config['defaultsave.directory']
    if Verbose:
        logger.notice('Input files : '+str(inputWS))  
    input = inputWS+',i' + str(specMin)
    if (specMax == -1):
        specMax = mtd[inputWS].getNumberHistograms() - 1
    for i in range(specMin + 1, specMax + 1):
        input += ';'+inputWS+',i'+str(i)
    (instr, run) = getInstrRun(inputWS)
    run_name = instr + run
    outNm = getWSprefix(inputWS) + 'conv_' + ftype + bg + str(specMin) + "_to_" + str(specMax)
    if Verbose:
        logger.notice(func)  
    PlotPeakByLogValue(Input=input, OutputWorkspace=outNm, Function=func, 
	    StartX=startX, EndX=endX, FitType='Sequential')
    wsname = confitParsToWS(outNm, inputWS, bg, specMin, specMax)
    RenameWorkspace(InputWorkspace=outNm, OutputWorkspace=outNm + "_Parameters")
    if Save:
        o_path = os.path.join(workdir, wsname+'.nxs')					# path name for nxs file
        if Verbose:
            logger.notice('Creating file : '+o_path)
        SaveNexusProcessed(InputWorkspace=wsname, Filename=o_path)
    if Plot != 'None':
        confitPlotSeq(wsname, Plot)
    EndTime('ConvFit')

##############################################################################
# Elwin
##############################################################################

def GetTemperature(root,tempWS,log_type,Verbose):
    (instr, run) = getInstrRun(root)
    run_name = instr+run
    log_name = run_name+'_'+log_type
    logger.notice(log_name)
    log_file = log_name+'.txt'
    log_path = FileFinder.getFullPath(log_file)
    logger.notice(log_path)
    if (log_path == ''):
        mess = ' Run : '+run_name +' ; Temperature file not found'
        xval = int(run_name[-3:])
        xlabel = 'Run-numbers (last 3 digits)'
    else:			
        LoadLog(Workspace=tempWS, Filename=log_path)
        run_logs = mtd[tempWS].getRun()
        tmp = run_logs[log_name].value
        temp = tmp[len(tmp)-1]
        mess = ' Run : '+run_name+' ; Temperature = '+str(temp)
        xval = temp
        xlabel = 'Temperature (K)'
    if Verbose:
        logger.notice(mess)
    return xval,xlabel

def elwin(inputFiles, eRange, Save=False, Verbose=False, Plot=False): 
    StartTime('ElWin')
    workdir = config['defaultsave.directory']
    CheckXrange(eRange,'Energy')
    tempWS = '__temp'
    if Verbose:
        range1 = str(eRange[0])+' to '+str(eRange[1])
        if ( len(eRange) == 4 ): 
            range2 = str(eRange[2])+' to '+str(eRange[3])
            logger.notice('Using 2 energy ranges from '+range1+' & '+range2)
        elif ( len(eRange) == 2 ):
            logger.notice('Using 1 energy range from '+range1)
    nr = 0
    inputRuns = sorted(inputFiles)
    for file in inputRuns:
        (direct, file_name) = os.path.split(file)
        (root, ext) = os.path.splitext(file_name)
        LoadNexus(Filename=file, OutputWorkspace=tempWS)
        nsam,ntc = CheckHistZero(tempWS)
        log_type = 'sample'
        (xval, xlabel) = GetTemperature(root,tempWS,log_type,Verbose)
        if Verbose:
            logger.notice('Reading file : '+file)
        if ( len(eRange) == 4 ):
            ElasticWindow(InputWorkspace=tempWS, Range1Start=eRange[0], Range1End=eRange[1], 
                Range2Start=eRange[2], Range2End=eRange[3],
                OutputInQ='__eq1', OutputInQSquared='__eq2')
        elif ( len(eRange) == 2 ):
            ElasticWindow(InputWorkspace=tempWS, Range1Start=eRange[0], Range1End=eRange[1],
                OutputInQ='__eq1', OutputInQSquared='__eq2')
        (instr, last) = getInstrRun(root)
        q1 = np.array(mtd['__eq1'].readX(0))
        i1 = np.array(mtd['__eq1'].readY(0))
        e1 = np.array(mtd['__eq1'].readE(0))
        q2 = np.array(mtd['__eq2'].readX(0))
        inY = mtd['__eq2'].readY(0)
        inE = mtd['__eq2'].readE(0)
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
        i2 = np.array(logy)
        e2 = np.array(loge)
        if (nr == 0):
            CloneWorkspace(InputWorkspace='__eq1', OutputWorkspace='__elf')
            first = getWSprefix(tempWS,root)
            datX1 = q1
            datY1 = i1
            datE1 = e1
            datX2 = q2
            datY2 = i2
            datE2 = e2
            Tvalue = [xval]
            Terror = [0.0]
            Taxis = str(xval)
        else:
            CloneWorkspace(InputWorkspace='__eq1', OutputWorkspace='__elftmp')
            ConjoinWorkspaces(InputWorkspace1='__elf', InputWorkspace2='__elftmp', CheckOverlapping=False)
            datX1 = np.append(datX1,q1)
            datY1 = np.append(datY1,i1)
            datE1 = np.append(datE1,e1)
            datX2 = np.append(datX2,q2)
            datY2 = np.append(datY2,i2)
            datE2 = np.append(datE2,e2)
            Tvalue.append(xval)
            Terror.append(0.0)
            Taxis += ','+str(xval)
        nr += 1
    Txa = np.array(Tvalue)
    Tea = np.array(Terror)
    nQ = len(q1)
    for nq in range(0,nQ):
        iq = []
        eq = []
        for nt in range(0,len(Tvalue)):
            ii = mtd['__elf'].readY(nt)
            iq.append(ii[nq])
            ie = mtd['__elf'].readE(nt)
            eq.append(ie[nq])
        iqa = np.array(iq)
        eqa = np.array(eq)
        if (nq == 0):
            datTx = Txa
            datTy = iqa
            datTe = eqa
        else:
            datTx = np.append(datTx,Txa)
            datTy = np.append(datTy,iqa)
            datTe = np.append(datTe,eqa)
    DeleteWorkspace(tempWS)
    DeleteWorkspace('__eq1')
    DeleteWorkspace('__eq2')
    if (nr == 1):
        ename = first[:-1]
    else:
        ename = first+'to_'+last
    elfWS = ename+'_elf'    # interchange Q & T
    CreateWorkspace(OutputWorkspace=elfWS, DataX=datTx, DataY=datTy, DataE=datTe,
        Nspec=nQ, UnitX='Energy')
    DeleteWorkspace('__elf')
    e1WS = ename+'_eq1'
    CreateWorkspace(OutputWorkspace=e1WS, DataX=datX1, DataY=datY1, DataE=datE1,
        Nspec=nr, UnitX='MomentumTransfer', VerticalAxisUnit='Energy', VerticalAxisValues=Taxis)
    AddSampleLog(Workspace=e1WS, LogName="Vaxis", LogType="String", LogText=xlabel)
    e2WS = ename+'_eq2'
    CreateWorkspace(OutputWorkspace=e2WS, DataX=datX2, DataY=datY2, DataE=datE2,
        Nspec=nr, UnitX='QSquared', VerticalAxisUnit='Energy', VerticalAxisValues=Taxis)
    AddSampleLog(Workspace=e2WS, LogName="Vaxis", LogType="String", LogText=xlabel)
    if Save:
        e1_path = os.path.join(workdir, e1WS+'.nxs')					# path name for nxs file
        e2_path = os.path.join(workdir, e2WS+'.nxs')					# path name for nxs file
        if Verbose:
            logger.notice('Creating file : '+e1_path)
            logger.notice('Creating file : '+e2_path)
        SaveNexusProcessed(InputWorkspace=e1WS, Filename=e1_path)
        SaveNexusProcessed(InputWorkspace=e2WS, Filename=e2_path)
    if Plot:
        elwinPlot(e1WS,e2WS,elfWS)
    EndTime('Elwin')
    return e1WS,e2WS

def elwinPlot(eq1,eq2,elf):
    nhist = mtd[eq1].getNumberHistograms()                      # no. of hist/groups in sam
    nBins = mtd[eq1].blocksize()
    lastXeq1 = mtd[eq1].readX(0)[nBins-1]
    graph1 = mp.plotSpectrum(eq1, range(0,nhist))
    layer1 = graph1.activeLayer()
    layer1.setScale(mp.Layer.Bottom, 0.0, lastXeq1)
    layer1.setAxisTitle(mp.Layer.Left,'Elastic Intensity')
    nBins = mtd[eq2].blocksize()
    lastXeq2 = mtd[eq2].readX(0)[nBins-1]
    graph2 = mp.plotSpectrum(eq2, range(0,nhist))
    layer2 = graph2.activeLayer()
    layer2.setScale(mp.Layer.Bottom, 0.0, lastXeq2)
    layer2.setAxisTitle(mp.Layer.Left,'log(Elastic Intensity)')
    ntemp = mtd[elf].getNumberHistograms()                      # no. of hist/groups in sam
    graph3 = mp.plotSpectrum(elf, range(0,ntemp))
    layer3 = graph3.activeLayer()
    layer3.setAxisTitle(mp.Layer.Bottom, 'Temperature(K)')

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
    Integration(InputWorkspace='res_data', OutputWorkspace='res_int')
    ConvertToPointData(InputWorkspace='res_data', OutputWorkspace='res_data')
    ExtractFFTSpectrum(InputWorkspace='res_data', OutputWorkspace='res_fft', FFTPart=2)
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
        Integration(InputWorkspace='sam_data', OutputWorkspace='sam_int')
        ConvertToPointData(InputWorkspace='sam_data', OutputWorkspace='sam_data')
        ExtractFFTSpectrum(InputWorkspace='sam_data', OutputWorkspace='sam_fft', FFTPart=2)
        Divide(LHSWorkspace='sam_fft', RHSWorkspace='sam_int', OutputWorkspace='sam')
        # Create save file name
        savefile = getWSprefix('sam_data', root) + 'iqt'
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

def furyfitParsToWS(Table, Data, option):
    nopt = len(option)
    if nopt == 2:
        npeak = option[0]
        type = option[1]
    elif nopt == 4:
        npeak = '2'
        type = 'SE'
    else:
        logger.notice('Bad option : ' +option)	    
    Q = createQaxis(Data)
    nQ = len(Q)
    ws = mtd[Table]
    rCount = ws.rowCount()
    cCount = ws.columnCount()
    cName =  ws.getColumnNames()
    Qa = np.array(Q)
    A0v = ws.column(1)     #bgd value
    A0e = ws.column(2)     #bgd error
    Iy1 = ws.column(5)      #intensity1 value
    Ie1 = ws.column(2)      #intensity1 error = bgd
    dataX = Qa
    dataY = np.array(A0v)
    dataE = np.array(A0e)
    names = cName[1]
    dataX = np.append(dataX,Qa)
    dataY = np.append(dataY,np.array(Iy1))
    dataE = np.append(dataE,np.array(Ie1))
    names += ","+cName[5]
    Ty1 = ws.column(7)      #tau1 value
    Te1 = ws.column(8)      #tau1 error
    dataX = np.append(dataX,Qa)
    dataY = np.append(dataY,np.array(Ty1))
    dataE = np.append(dataE,np.array(Te1))
    names += ","+cName[7]
    nSpec = 3
    if npeak == '1' and type == 'S':
        By1 = ws.column(9)  #beta1 value
        Be1 = ws.column(10) #beta2 error
        dataX = np.append(dataX,Qa)
        dataY = np.append(dataY,np.array(By1))
        dataE = np.append(dataE,np.array(Be1))
        names += ","+cName[9]
        nSpec += 1
    if npeak == '2':
        Iy2 = ws.column(9)  #intensity2 value
        Ie2 = ws.column(10) #intensity2 error
        dataX = np.append(dataX,Qa)
        dataY = np.append(dataY,np.array(Iy2))
        dataE = np.append(dataE,np.array(Ie2))
        names += ","+cName[9]
        nSpec += 1
        Ty2 = ws.column(11)  #tau2 value
        Te2 = ws.column(12) #tau2 error
        dataX = np.append(dataX,Qa)
        dataY = np.append(dataY,np.array(Ty2))
        dataE = np.append(dataE,np.array(Te2))
        names += ","+cName[11]
        nSpec += 1
    wsname = Table + "_Workspace"
    CreateWorkspace(OutputWorkspace=wsname, DataX=dataX, DataY=dataY, DataE=dataE, 
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

def furyfitSeq(inputWS, func, ftype, startx, endx, Save, Plot, Verbose=False): 
    StartTime('FuryFit')
    workdir = config['defaultsave.directory']
    input = inputWS+',i0'
    nHist = mtd[inputWS].getNumberHistograms()
    for i in range(1,nHist):
        input += ';'+inputWS+',i'+str(i)
    outNm = getWSprefix(inputWS) + 'fury_' + ftype + "0_to_" + str(nHist-1)
    option = ftype[:-2]
    if Verbose:
        logger.notice('Option: '+option)  
        logger.notice(func)  
    PlotPeakByLogValue(Input=input, OutputWorkspace=outNm, Function=func, 
        StartX=startx, EndX=endx, FitType='Sequential')
    wsname = furyfitParsToWS(outNm, inputWS, option)
    RenameWorkspace(InputWorkspace=outNm, OutputWorkspace=outNm+"_Parameters")
    if Save:
        opath = os.path.join(workdir, wsname+'.nxs')					# path name for nxs file
        SaveNexusProcessed(InputWorkspace=wsname, Filename=opath)
        if Verbose:
            logger.notice('Output file : '+opath)  
    if ( Plot != 'None' ):
        furyfitPlotSeq(wsname, Plot)
    EndTime('FuryFit')
    return mtd[wsname]

def furyfitMultParsToWS(Table, Data):
    dataX = []
    dataA0v = []
    dataA0e = []
    dataY1 = []
    dataE1 = []
    dataY2 = []
    dataE2 = []
    dataY3 = []
    dataE3 = []
    ws = mtd[Table+'_Parameters']
    rCount = ws.rowCount()
    cCount = ws.columnCount()
    logger.notice(' Cols : '+str(cCount))
    nSpec = ( rCount - 1 ) / 5
    for spec in range(0,nSpec):
        A0val = 1
        A0err = 2
        ival = 5                   #intensity value
        ierr = 6                   #intensity error
        tval = 7                   #tau value
        terr = 8                   #tau error
        bval = 9                   #beta value
        bval = 10                   #beta error
        dataX.append(spec)
        dataA0v.append(ws.cell(spec,A0val))
        dataA0e.append(ws.cell(spec,A0err))
        dataY1.append(ws.cell(spec,ival))
        dataE1.append(ws.cell(spec,A0err))
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
    mfit_layer = mfit_plot.activeLayer()
    mfit_layer.setAxisTitle(mp.Layer.Left,'log(Elastic Intensity)')
    mp.mergePlots(mfit_plot,mp.plotSpectrum(fitWS+'_line',n,False))

def msdfit(inputs, startX, endX, Save=False, Verbose=False, Plot=True): 
    StartTime('msdFit')
    workdir = config['defaultsave.directory']
    log_type = 'sample'
    file = inputs[0]
    (direct, filename) = os.path.split(file)
    (root, ext) = os.path.splitext(filename)
    (instr, first) = getInstrRun(filename)
    if Verbose:
        logger.notice('Reading Run : '+file)
    LoadNexusProcessed(FileName=file, OutputWorkspace=root)
    nHist = mtd[root].getNumberHistograms()
    file_list = []
    run_list = []
    ws = mtd[root]
    ws_run = ws.getRun()
    vertAxisValues = ws.getAxis(1).extractValues()
    x_list = vertAxisValues
    if 'Vaxis' in ws_run:
        xlabel = ws_run.getLogData('Vaxis').value
    for nr in range(0, nHist):
        nsam,ntc = CheckHistZero(root)
        lnWS = '__lnI_'+str(nr)
        file_list.append(lnWS)
        ExtractSingleSpectrum(InputWorkspace=root, OutputWorkspace=lnWS,
            WorkspaceIndex=nr)
        if (nr == 0):
            run_list = lnWS
        else:
            run_list += ';'+lnWS
    mname = root[:-4]
    msdWS = mname+'_msd'
    if Verbose:
       logger.notice('Fitting Runs '+mname)
       logger.notice('Q-range from '+str(startX)+' to '+str(endX))
    function = 'name=LinearBackground, A0=0, A1=0'
    PlotPeakByLogValue(Input=run_list, OutputWorkspace=msdWS+'_Table', Function=function,
        StartX=startX, EndX=endX, FitType = 'Sequential')
    msdfitParsToWS(msdWS, x_list)
    nr = 0
    fitWS = mname+'_Fit'
    a0 = mtd[msdWS+'_a0'].readY(0)
    a1 = mtd[msdWS+'_a1'].readY(0)
    for nr in range(0, nHist):
        inWS = file_list[nr]
        CropWorkspace(InputWorkspace=inWS,OutputWorkspace='__data',XMin=0.95*startX,XMax=1.05*endX)
        xin = mtd['__data'].readX(0)
        nxd = len(xin)-1
        xd = []
        yd = []
        ed = []
        for n in range(0,nxd):
            line = a0[nr] - a1[nr]*xin[n]
            xd.append(xin[n])
            yd.append(line)
            ed.append(0.0)
        xd.append(xin[nxd])
        CreateWorkspace(OutputWorkspace='__line', DataX=xd, DataY=yd, DataE=ed,
		    Nspec=1)
        if (nr == 0):
            RenameWorkspace(InputWorkspace='__data',OutputWorkspace=fitWS+'_data')
            RenameWorkspace(InputWorkspace='__line',OutputWorkspace=fitWS+'_line')
        else:
            ConjoinWorkspaces(InputWorkspace1=fitWS+'_data', InputWorkspace2='__data', CheckOverlapping=False)
            ConjoinWorkspaces(InputWorkspace1=fitWS+'_line', InputWorkspace2='__line', CheckOverlapping=False)
        nr += 1
        group = fitWS+'_data,'+ fitWS+'_line'
        GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fitWS)
        DeleteWorkspace(inWS)
    if Plot:
        msdfitPlotSeq(msdWS, xlabel)
        msdfitPlotFits(root, fitWS, 0)
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
    sam_name = getWSprefix(inputWS)
    ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='Wavelength',
        EMode='Indirect', EFixed=efixed)
    if canWS != '':
        (instr, can_run) = getInstrRun(canWS)
        corrections = [corr+'_1', corr+'_2', corr+'_3', corr+'_4']
        CorrectedWS = sam_name +'Correct_'+ can_run
        ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='Wavelength',
            EMode='Indirect', EFixed=efixed)
    else:
        corrections = [corr+'_1']
        CorrectedWS = sam_name +'Corrected'
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
                ConjoinWorkspaces(InputWorkspace1=CorrectedWS, InputWorkspace2=CorrectedSampleWS,
                                  CheckOverlapping=False)
    ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='DeltaE',
        EMode='Indirect', EFixed=efixed)
    ConvertUnits(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS, Target='DeltaE',
        EMode='Indirect', EFixed=efixed)
    CloneWorkspace(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS+'_rqw')
    replace_workspace_axis(CorrectedWS+'_rqw', Q)
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
                
def abscorFeeder(sample, container, geom, useCor, Verbose=False, ScaleOrNotToScale=False, factor=1, Save=False,
        PlotResult='None', PlotContrib=False):
    '''Load up the necessary files and then passes them into the main
    applyCorrections routine.'''
    StartTime('ApplyCorrections')
    workdir = config['defaultsave.directory']
    CheckAnalysers(sample,container,Verbose)
    s_hist,sxlen = CheckHistZero(sample)
    sam_name = getWSprefix(sample)
    if container != '':
        CheckHistSame(sample,'Sample',container,'Container')
        (instr, can_run) = getInstrRun(container)
        if ScaleOrNotToScale:
            Scale(InputWorkspace=container, OutputWorkspace=container, Factor=factor, Operation='Multiply')
            if Verbose:
                logger.notice('Container scaled by '+str(factor))
    if useCor:
        if Verbose:
            text = 'Correcting sample ' + sample
            if container != '':
                text += ' with ' + container
            logger.notice(text)
        file = sam_name + geom +'_Abs.nxs'
        abs_path = os.path.join(workdir, file)					# path name for nxs file
        if Verbose:
            logger.notice('Correction file :'+abs_path)
        LoadNexus(Filename=abs_path, OutputWorkspace='corrections')
        cor_result = applyCorrections(sample, container, 'corrections', Verbose)
        if Save:
            cred_path = os.path.join(workdir,cor_result+'_red.nxs')
            SaveNexusProcessed(InputWorkspace=cor_result+'_red',Filename=cred_path)
            if Verbose:
                logger.notice('Output file created : '+cred_path)
        plot_list = [cor_result+'_red',sample]
        if ( container != '' ):
            plot_list.append(container)
        if (PlotResult != 'None'):
            plotCorrResult(cor_result+'_rqw',PlotResult)
        if PlotContrib:
            plotCorrContrib(plot_list,0)
    else:
        if ( container == '' ):
            sys.exit('ERROR *** Invalid options - nothing to do!')
        else:
            sub_result = sam_name +'Subtract_'+ can_run
            if Verbose:
                logger.notice('Subtracting '+container+' from '+sample)
            Minus(LHSWorkspace=sample,RHSWorkspace=container,OutputWorkspace=sub_result)
            CloneWorkspace(InputWorkspace=sub_result, OutputWorkspace=sub_result+'_rqw')
            theta,Q = GetThetaQ(sample)
            replace_workspace_axis(sub_result+'_rqw', Q)
            RenameWorkspace(InputWorkspace=sub_result, OutputWorkspace=sub_result+'_red')
            if Save:
                sred_path = os.path.join(workdir,sub_result+'_red.nxs')
                SaveNexusProcessed(InputWorkspace=sub_result+'_red',Filename=sred_path)
                if Verbose:
                    logger.notice('Output file created : '+sred_path)
            plot_list = [sub_result+'_red',sample]
            if (PlotResult != 'None'):
                plotCorrResult(sub_result+'_rqw',PlotResult)
            if PlotContrib:
                plotCorrContrib(plot_list,0)
    EndTime('ApplyCorrections')

def plotCorrResult(inWS,PlotResult):
    nHist = mtd[inWS].getNumberHistograms()
    if (PlotResult == 'Spectrum' or PlotResult == 'Both'):
        if nHist >= 10:                       #only plot up to 10 hists
            nHist = 10
        plot_list = []
        for i in range(0, nHist):
            plot_list.append(i)
        res_plot=mp.plotSpectrum(inWS,plot_list)
    if (PlotResult == 'Contour' or PlotResult == 'Both'):
        if nHist >= 5:                        #needs at least 5 hists for a contour
            mp.importMatrixWorkspace(inWS).plotGraph2D()

def plotCorrContrib(plot_list,n):
        con_plot=mp.plotSpectrum(plot_list,n)

def replace_workspace_axis(wsName, new_values):
    from mantidsimple import createNumericAxis, mtd        #temporary use of old API
    ax1 = createNumericAxis(len(new_values))
    for i in range(len(new_values)):
        ax1.setValue(i, new_values[i])
    ax1.setUnit('MomentumTransfer')
    mtd[wsName].replaceAxis(1, ax1)      #axis=1 is vertical
