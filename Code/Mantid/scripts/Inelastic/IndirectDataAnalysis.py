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

def getConvFitOption(ftype, bgd, Verbose):
    if ftype[:5] == 'Delta':
        delta = True
        lor = ftype[5:6]
    else:
        delta = False
        lor = ftype[:1]
    options = [bgd, delta, int(lor)]
    if Verbose:
        logger.notice('Fit type : Delta = ' + str(options[1]) + ' ; Lorentzians = ' + str(options[2]))
        logger.notice('Background type : ' + options[0])
    return options

##############################################################################

def createConvFitFun(options, par, file):
    bgd_fun = 'name=LinearBackground,A0='
    if options[0] == 'FixF':
        bgd_fun = bgd_fun +str(par[0])+',A1=0,ties=(A0='+str(par[0])+',A1=0.0)'
    if options[0] == 'FitF':
        bgd_fun = bgd_fun +str(par[0])+',A1=0,ties=(A1=0.0)'
    if options[0] == 'FitL':
        bgd_fun = bgd_fun +str(par[0])+',A1='+str(par[1])
    if options[1]:
        ip = 3
    else:
        ip = 2
    pk_1 = '(composite=Convolution;name=Resolution, FileName="'+file+'"'
    if  options[2] >= 1:
        lor_fun = 'name=Lorentzian,Amplitude='+str(par[ip])+',PeakCentre='+str(par[ip+1])+',HWHM='+str(par[ip+2])
    if options[2] == 2:
        lor_2 = 'name=Lorentzian,Amplitude='+str(par[ip+3])+',PeakCentre='+str(par[ip+4])+',HWHM='+str(par[ip+5])
        lor_fun = lor_fun +';'+ lor_2 +';ties=(f0.PeakCentre=f1.PeakCentre)'
    if options[1]:
        delta_fun = 'name=DeltaFunction,Amplitude='+str(par[2])
        lor_fun = delta_fun +';' + lor_fun
    func = bgd_fun +';'+ pk_1 +';('+ lor_fun +'))'
    return func

##############################################################################

def getConvFitResult(inputWS, resFile, outNm, ftype, bgd, Verbose):
    options = getConvFitOption(ftype, bgd[:-2], Verbose)   
    params = mtd[outNm+'_Parameters']
    A0 = params.column(1)     #bgd A0 value
    A1 = params.column(3)     #bgd A1 value
    if options[1]:
        ip = 7
        D1 = params.column(5)      #delta value
    else:
        ip = 5
    if options[2] >= 1:
        H1 = params.column(ip)        #height1 value
        C1 = params.column(ip+2)      #centre1 value
        W1 = params.column(ip+4)      #width1 value
    if options[2] == 2:
        H2 = params.column(ip+6)        #height2 value
        C2 = params.column(ip+8)      #centre2 value
        W2 = params.column(ip+10)      #width2 value
    nHist = mtd[inputWS].getNumberHistograms()
    for i in range(nHist):
        paras = [A0[i], A1[i]]
        if options[1]:
            paras.append(D1[i])
        if options[2] >= 1:
            paras.append(H1[i])
            paras.append(C1[i])
            paras.append(W1[i])
        if options[2] == 2:
            paras.append(H2[i])
            paras.append(C2[i])
            paras.append(W2[i])
        func = createConvFitFun(options, paras, resFile)
        if Verbose:
            logger.notice('Fit func : '+func)      
        fitWS = outNm + '_Result_'
        fout = fitWS + str(i)
        Fit(Function=func,InputWorkspace=inputWS,WorkspaceIndex=i,Output=fout,MaxIterations=0)
        unitx = mtd[fout+'_Workspace'].getAxis(0).setUnit("Label")
        unitx.setLabel('Time' , 'ns')
        RenameWorkspace(InputWorkspace=fout+'_Workspace', OutputWorkspace=fout)
        DeleteWorkspace(fitWS+str(i)+'_NormalisedCovarianceMatrix')
        DeleteWorkspace(fitWS+str(i)+'_Parameters')
        if i == 0:
            group = fout
        else:
            group += ',' + fout
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fitWS[:-1])

##############################################################################

def confitParsToWS(Table, Data, specMin=0, specMax=-1):
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
        if re.search('HWHM$', yAxis) or re.search('Amplitude$', yAxis):
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

##############################################################################

def confitPlotSeq(inputWS, Plot):
    nhist = mtd[inputWS].getNumberHistograms()
    if ( Plot == 'All' ):
        mp.plotSpectrum(inputWS, range(0, nhist), True)
        return    
    plotSpecs = []
    if ( Plot == 'Intensity' ):
        res = 'Amplitude$'
    elif ( Plot == 'HWHM' ):
        res = 'HWHM$'
    for i in range(0,nhist):
        title = mtd[inputWS].getAxis(1).label(i)
        if re.search(res, title):
            plotSpecs.append(i)
    mp.plotSpectrum(inputWS, plotSpecs, True)

##############################################################################

def confitSeq(inputWS, func, startX, endX, Save, Plot, ftype, bgd, specMin, specMax, Verbose):
    StartTime('ConvFit')
    workdir = config['defaultsave.directory']
    elements = func.split('"')
    resFile = elements[1]  
    if Verbose:
        logger.notice('Input files : '+str(inputWS))  
    input = inputWS+',i' + str(specMin)
    if (specMax == -1):
        specMax = mtd[inputWS].getNumberHistograms() - 1
    for i in range(specMin + 1, specMax + 1):
        input += ';'+inputWS+',i'+str(i)
    (instr, run) = getInstrRun(inputWS)
    run_name = instr + run
    outNm = getWSprefix(inputWS) + 'conv_' + ftype + bgd + str(specMin) + "_to_" + str(specMax)
    if Verbose:
        logger.notice(func)  
    PlotPeakByLogValue(Input=input, OutputWorkspace=outNm, Function=func, 
        StartX=startX, EndX=endX, FitType='Sequential')
    wsname = confitParsToWS(outNm, inputWS, specMin, specMax)
    RenameWorkspace(InputWorkspace=outNm, OutputWorkspace=outNm + "_Parameters")
    getConvFitResult(inputWS, resFile, outNm, ftype, bgd, Verbose)
    if Save:
        o_path = os.path.join(workdir, wsname+'.nxs')                    # path name for nxs file
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
    run = mtd[tempWS].getRun()
    unit1 = 'Temperature'               # default values
    unit2 = 'K'
    if log_type in run:                 # test logs in WS
        tmp = run[log_type].value
        temp = tmp[len(tmp)-1]
        xval = temp
        mess = ' Run : '+run_name +' ; Temperature in log = '+str(temp)
    else:                               # logs not in WS
        logger.notice('Log parameter not found')
        log_file = log_name+'.txt'
        log_path = FileFinder.getFullPath(log_file)
        if (log_path == ''):            # log file does not exists
            mess = ' Run : '+run_name +' ; Temperature file not found'
            xval = int(run_name[-3:])
            unit1 = 'Run-number'
            unit2 = 'last 3 digits'
        else:                           # get from log file
            LoadLog(Workspace=tempWS, Filename=log_path)
            run_logs = mtd[tempWS].getRun()
            tmp = run_logs[log_name].value
            temp = tmp[len(tmp)-1]
            xval = temp
            mess = ' Run : '+run_name+' ; Temperature in file = '+str(temp)
    if Verbose:
        logger.notice(mess)
    unit = [unit1,unit2]
    return xval,unit

def elwin(inputFiles, eRange, log_type='sample', Normalise = False,
        Save=False, Verbose=False, Plot=False): 
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
        (xval, unit) = GetTemperature(root,tempWS,log_type,Verbose)
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
        Nspec=nQ, UnitX='Energy', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=q1)
    unitx = mtd[elfWS].getAxis(0).setUnit("Label")
    unitx.setLabel(unit[0], unit[1])
    DeleteWorkspace('__elf')
    e1WS = ename+'_eq1'
    CreateWorkspace(OutputWorkspace=e1WS, DataX=datX1, DataY=datY1, DataE=datE1,
        Nspec=nr, UnitX='MomentumTransfer', VerticalAxisUnit='Energy', VerticalAxisValues=Taxis)
    unity = mtd[e1WS].getAxis(1).setUnit("Label")
    unity.setLabel(unit[0], unit[1])
    label = unit[0]+' / '+unit[1]
    AddSampleLog(Workspace=e1WS, LogName="Vaxis", LogType="String", LogText=label)
    e2WS = ename+'_eq2'
    CreateWorkspace(OutputWorkspace=e2WS, DataX=datX2, DataY=datY2, DataE=datE2,
        Nspec=nr, UnitX='QSquared', VerticalAxisUnit='Energy', VerticalAxisValues=Taxis)
    unity = mtd[e2WS].getAxis(1).setUnit("Label")
    unity.setLabel(unit[0], unit[1])
    AddSampleLog(Workspace=e2WS, LogName="Vaxis", LogType="String", LogText=label)
    if unit[0] == 'Temperature':
        nT = len(Tvalue)
        if Tvalue[0] < Tvalue[nT-1]:
            lo = 0
            hi = nT-1
        else:
            lo = nT-1
            hi = 0
        text = 'Temperature range : '+str(Tvalue[lo])+' to '+str(Tvalue[hi])
        if Normalise:
            yval = mtd[e1WS].readY(lo)
            normFactor = 1.0/yval[0]
            Scale(InputWorkspace=e1WS, OutputWorkspace=e1WS, Factor=normFactor, Operation='Multiply')
            if Verbose:
                logger.notice(text)
                logger.notice('Normalised eq1 by scale factor : '+str(normFactor))

    if Save:
        e1_path = os.path.join(workdir, e1WS+'.nxs')					# path name for nxs file
        e2_path = os.path.join(workdir, e2WS+'.nxs')					# path name for nxs file
        elf_path = os.path.join(workdir, elfWS+'.nxs')					# path name for nxs file
        if Verbose:
            logger.notice('Creating file : '+e1_path)
            logger.notice('Creating file : '+e2_path)
            logger.notice('Creating file : '+elf_path)
        SaveNexusProcessed(InputWorkspace=e1WS, Filename=e1_path)
        SaveNexusProcessed(InputWorkspace=e2WS, Filename=e2_path)
        SaveNexusProcessed(InputWorkspace=elfWS, Filename=elf_path)
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

def getFuryFitOption(option):
    nopt = len(option)
    if nopt == 2:
        npeak = option[0]
        type = option[1]
    elif nopt == 4:
        npeak = '2'
        type = 'SE'
    else:
        error = 'Bad option : ' +option
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    return npeak, type

def furyfitParsToWS(Table, Data, option):
    npeak, type = getFuryFitOption(option)   
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

def createFurySeqResFun(ties, par, option):
    npeak, type = getFuryFitOption(option)   
    fun = 'name=LinearBackground,A0='+str(par[0])+',A1=0,ties=(A1=0);'
    if npeak == '1' and type == 'E':
        fun += 'name=UserFunction,Formula=Intensity*exp(-(x/Tau)),Intensity='+str(par[1])+',Tau='+str(par[2])
    if npeak == '1' and type == 'S':
        fun += 'name=UserFunction,Formula=Intensity*exp(-(x/Tau)^Beta),Intensity='+str(par[1])+',Tau='+str(par[2])+',Beta='+str(par[3])
    if ties:
        fun += ';ties=(f1.Intensity=1-f0.A0)'
    return fun

def getFurySeqResult(inputWS, outNm, option, Verbose):
    logger.notice('Option : ' +option)
    npeak, type = getFuryFitOption(option)   
    params = mtd[outNm+'_Parameters']
    A0 = params.column(1)     #bgd value
    I1 = params.column(5)      #intensity1 value
    T1 = params.column(7)      #tau1 value
    if npeak == '1' and type == 'S':
        B1 = params.column(9)  #beta1 value
    if npeak == '2':
        I2 = params.column(9)  #intensity2 value
        T2 = params.column(11)  #tau2 value
    nHist = mtd[inputWS].getNumberHistograms()
    for i in range(nHist):
        paras = [A0[i], I1[i], T1[i]]
        if npeak == '1' and type == 'S':
            paras.append(B1[i])
        if npeak == '2':
            paras.append(I2[i])
            paras.append(T2[i])
        func = createFurySeqResFun(True, paras, option)
        if Verbose:
            logger.notice('Fit func : '+func)  	
        fitWS = outNm + '_Result_'
        fout = fitWS + str(i)
        Fit(Function=func,InputWorkspace=inputWS,WorkspaceIndex=i,Output=fout,MaxIterations=0)
        unitx = mtd[fout+'_Workspace'].getAxis(0).setUnit("Label")
        unitx.setLabel('Time' , 'ns')
        RenameWorkspace(InputWorkspace=fout+'_Workspace', OutputWorkspace=fout)
        DeleteWorkspace(fitWS+str(i)+'_NormalisedCovarianceMatrix')
        DeleteWorkspace(fitWS+str(i)+'_Parameters')
        if i == 0:
            group = fout
        else:
            group += ',' + fout
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fitWS[:-1])

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
    fitWS = furyfitParsToWS(outNm, inputWS, option)
    RenameWorkspace(InputWorkspace=outNm, OutputWorkspace=outNm+"_Parameters")
    CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=inputWS, XMin=startx, XMax=endx)
    getFurySeqResult(inputWS, outNm, option, Verbose)
    if Save:
        opath = os.path.join(workdir, fitWS+'.nxs')					# path name for nxs file
        SaveNexusProcessed(InputWorkspace=fitWS, Filename=opath)
        if Verbose:
            logger.notice('Output file : '+opath)  
    if ( Plot != 'None' ):
        furyfitPlotSeq(fitWS, Plot)
    EndTime('FuryFit')
    return mtd[fitWS]

def furyfitMultParsToWS(Table, Data):
#   Q = createQaxis(Data)
    theta,Q = GetThetaQ(Data)
    ws = mtd[Table+'_Parameters']
    rCount = ws.rowCount()
    cCount = ws.columnCount()
    nSpec = ( rCount - 1 ) / 5
    val = ws.column(1)     #value
    err = ws.column(2)     #error
    dataX = []
    A0val = []
    A0err = []
    Ival = []
    Ierr = []
    Tval = []
    Terr = []
    Bval = []
    Berr = []
    for spec in range(0,nSpec):
        n1 = spec*5
        A0 = n1
        A1 = n1+1
        int = n1+2                   #intensity value
        tau = n1+3                   #tau value
        beta = n1+4                   #beta value
        dataX.append(Q[spec])
        A0val.append(val[A0])
        A0err.append(err[A0])
        Ival.append(val[int])
        Ierr.append(err[int])
        Tval.append(val[tau])
        Terr.append(err[tau])
        Bval.append(val[beta])
        Berr.append(err[beta])
    nQ = len(dataX)
    Qa = np.array(dataX)
    dataY = np.array(A0val)
    dataE = np.array(A0err)
    dataY = np.append(dataY,np.array(Ival))
    dataE = np.append(dataE,np.array(Ierr))
    dataY = np.append(dataY,np.array(Tval))
    dataE = np.append(dataE,np.array(Terr))
    dataY = np.append(dataY,np.array(Bval))
    dataE = np.append(dataE,np.array(Berr))
    names = 'A0,Intensity,Tau,Beta'
    suffix = 'Workspace'
    wsname = Table + '_' + suffix
    CreateWorkspace(OutputWorkspace=wsname, DataX=Qa, DataY=dataY, DataE=dataE, 
        Nspec=4, UnitX='MomentumTransfer', VerticalAxisUnit='Text',
        VerticalAxisValues=names)
    return wsname

def furyfitPlotMult(inputWS, Plot):
    nHist = mtd[inputWS].getNumberHistograms()
    if ( Plot == 'All' ):
        mp.plotSpectrum(inputWS, range(0, nHist))
        return
    plotSpecs = []
    if ( Plot == 'Intensity' ):
        mp.plotSpectrum(inputWS, 1, True)
    if ( Plot == 'Tau' ):
        mp.plotSpectrum(inputWS, 2, True)
    elif ( Plot == 'Beta' ):
        mp.plotSpectrum(inputWS, 3, True)   


def createFuryMultFun(ties = True, function = ''):
    fun =  '(composite=CompositeFunction,$domains=i;'
    fun += function
    if ties:
        fun += ';ties=(f1.Intensity=1-f0.A0)'
    fun += ');'
    return fun

def createFuryMultResFun(ties = True, A0 = 0.02, Intensity = 0.98 ,Tau = 0.025, Beta = 0.8):
    fun =  '(composite=CompositeFunction,$domains=i;'
    fun += 'name=LinearBackground,A0='+str(A0)+',A1=0,ties=(A1=0);'
    fun += 'name=UserFunction,Formula=Intensity*exp(-(x/Tau)^Beta),Intensity='+str(Intensity)+',Tau='+str(Tau)+',Beta='+str(Beta)
    if ties:
        fun += ';ties=(f1.Intensity=1-f0.A0)'
    fun += ');'
    return fun

def getFuryMultResult(inputWS, outNm, function, Verbose):
    params = mtd[outNm+'_Parameters']
    nHist = mtd[inputWS].getNumberHistograms()
    for i in range(nHist):
        j = 5 * i
#        assert( params.row(j)['Name'][3:] == 'f0.A0' )
        A0 = params.row(j)['Value']
        A1 = params.row(j + 1)['Value']
        Intensity = params.row(j + 2)['Value']
        Tau = params.row(j + 3)['Value']
        Beta = params.row(j + 4)['Value']
        func = createFuryMultResFun(True,  A0, Intensity ,Tau, Beta)
        if Verbose:
            logger.notice('Fit func : '+func)  	
        fitWS = outNm + '_Result_'
        fout = fitWS + str(i)
        Fit(Function=func,InputWorkspace=inputWS,WorkspaceIndex=i,Output=fout,MaxIterations=0)
        unitx = mtd[fout+'_Workspace'].getAxis(0).setUnit("Label")
        unitx.setLabel('Time' , 'ns')
        RenameWorkspace(InputWorkspace=fout+'_Workspace', OutputWorkspace=fout)
        DeleteWorkspace(fitWS+str(i)+'_NormalisedCovarianceMatrix')
        DeleteWorkspace(fitWS+str(i)+'_Parameters')
        if i == 0:
            group = fout
        else:
            group += ',' + fout
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fitWS[:-1])

def furyfitMult(inputWS, function, ftype, startx, endx, Save, Plot, Verbose=False):
    StartTime('FuryFit Mult')
    workdir = config['defaultsave.directory']
    option = ftype[:-2]
    if Verbose:
        logger.notice('Option: '+option)  
        logger.notice('Function: '+function)  
    nHist = mtd[inputWS].getNumberHistograms()
    outNm = inputWS[:-3] + 'fury_mult'
    f1 = createFuryMultFun(True, function)
    func= 'composite=MultiDomainFunction,NumDeriv=1;'
    ties='ties=('
    kwargs = {}
    for i in range(0,nHist):
        func+=f1
        if i > 0:
            ties += 'f' + str(i) + '.f1.Beta=f0.f1.Beta'
            if i < nHist-1:
                ties += ','
            kwargs['InputWorkspace_' + str(i)] = inputWS
        kwargs['WorkspaceIndex_' + str(i)] = i
    ties+=')'
    func += ties
    CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=inputWS, XMin=startx, XMax=endx)
    Fit(Function=func,InputWorkspace=inputWS,WorkspaceIndex=0,Output=outNm,**kwargs)
    outWS = furyfitMultParsToWS(outNm, inputWS)
    getFuryMultResult(inputWS, outNm, function, Verbose)
    if Save:
        opath = os.path.join(workdir, outWS+'.nxs')					# path name for nxs file
        SaveNexusProcessed(InputWorkspace=outWS, Filename=opath)
        rpath = os.path.join(workdir, outNm+'_result.nxs')					# path name for nxs file
        SaveNexusProcessed(InputWorkspace=outNm+'_result', Filename=rpath)
        if Verbose:
            logger.notice('Output file : '+opath)  
            logger.notice('Output file : '+rpath)  
    if ( Plot != 'None' ):
        furyfitPlotMult(outWS, Plot)
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

def msdfitPlotFits(calcWS, n):
    mfit_plot = mp.plotSpectrum(calcWS+'_0',[0,1],True)
    mfit_layer = mfit_plot.activeLayer()
    mfit_layer.setAxisTitle(mp.Layer.Left,'log(Elastic Intensity)')

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
    calcWS = mname+'_msd_Result'
    a0 = mtd[msdWS+'_a0'].readY(0)
    a1 = mtd[msdWS+'_a1'].readY(0)
    for nr in range(0, nHist):
        inWS = file_list[nr]
        CropWorkspace(InputWorkspace=inWS,OutputWorkspace='__data',XMin=0.95*startX,XMax=1.05*endX)
        dataX = mtd['__data'].readX(0)
        nxd = len(dataX)
        dataX = np.append(dataX,2*dataX[nxd-1]-dataX[nxd-2])
        dataY = np.array(mtd['__data'].readY(0))
        dataE = np.array(mtd['__data'].readE(0))
        xd = []
        yd = []
        ed = []
        for n in range(0,nxd):
            line = a0[nr] - a1[nr]*dataX[n]
            xd.append(dataX[n])
            yd.append(line)
            ed.append(0.0)
        xd.append(dataX[nxd])
        dataX = np.append(dataX,np.array(xd))
        dataY = np.append(dataY,np.array(yd))
        dataE = np.append(dataE,np.array(ed))
        fout = calcWS +'_'+ str(nr)
        CreateWorkspace(OutputWorkspace=fout, DataX=dataX, DataY=dataY, DataE=dataE,
            Nspec=2, UnitX='DeltaE', VerticalAxisUnit='Text', VerticalAxisValues='Data,Calc')
        if nr == 0:
            gro = fout
        else:
            gro += ',' + fout
        DeleteWorkspace(inWS)
        DeleteWorkspace('__data')
    GroupWorkspaces(InputWorkspaces=gro,OutputWorkspace=calcWS)
    if Plot:
        msdfitPlotSeq(msdWS, xlabel)
        msdfitPlotFits(calcWS, 0)
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
    replace_workspace_axis(CorrectedWS+'_rqw', Q, 'MomentumTransfer')
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
        rws = mtd[cor_result+'_red']
        outNm= cor_result + '_Result_'
        if Save:
            cred_path = os.path.join(workdir,cor_result+'_red.nxs')
            SaveNexusProcessed(InputWorkspace=cor_result+'_red',Filename=cred_path)
            if Verbose:
                logger.notice('Output file created : '+cred_path)
        calc_plot = [cor_result+'_red',sample]
        res_plot = cor_result+'_rqw'
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
            replace_workspace_axis(sub_result+'_rqw', Q, 'MomentumTransfer')
            RenameWorkspace(InputWorkspace=sub_result, OutputWorkspace=sub_result+'_red')
            rws = mtd[sub_result+'_red']
            outNm= sub_result + '_Result_'
            if Save:
                sred_path = os.path.join(workdir,sub_result+'_red.nxs')
                SaveNexusProcessed(InputWorkspace=sub_result+'_red',Filename=sred_path)
                if Verbose:
                    logger.notice('Output file created : '+sred_path)
            res_plot = sub_result+'_rqw'
    if (PlotResult != 'None'):
        plotCorrResult(res_plot,PlotResult)
    if ( container != '' ):
        sws = mtd[sample]
        cws = mtd[container]
        names = 'Sample,Can,Calc'
        for i in range(0, s_hist): # Loop through each spectra in the inputWS
            dataX = np.array(sws.readX(i))
            dataY = np.array(sws.readY(i))
            dataE = np.array(sws.readE(i))
            dataX = np.append(dataX,np.array(cws.readX(i)))
            dataY = np.append(dataY,np.array(cws.readY(i)))
            dataE = np.append(dataE,np.array(cws.readE(i)))
            dataX = np.append(dataX,np.array(rws.readX(i)))
            dataY = np.append(dataY,np.array(rws.readY(i)))
            dataE = np.append(dataE,np.array(rws.readE(i)))
            fout = outNm + str(i)
            CreateWorkspace(OutputWorkspace=fout, DataX=dataX, DataY=dataY, DataE=dataE,
                Nspec=3, UnitX='DeltaE', VerticalAxisUnit='Text', VerticalAxisValues=names)
            if i == 0:
                group = fout
            else:
                group += ',' + fout
        GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=outNm[:-1])
        if PlotContrib:
            plotCorrContrib(outNm+'0',[0,1,2])
        if Save:
            res_path = os.path.join(workdir,outNm[:-1]+'.nxs')
            SaveNexusProcessed(InputWorkspace=outNm[:-1],Filename=res_path)
            if Verbose:
                logger.notice('Output file created : '+res_path)
    EndTime('ApplyCorrections')

from mantid.api import NumericAxis      
def replace_workspace_axis(wsName, new_values, new_unit):
    ax1 = NumericAxis.create(len(new_values))
    for i in range(len(new_values)):
        ax1.setValue(i, new_values[i])
    ax1.setUnit(new_unit)
    mtd[wsName].replaceAxis(1, ax1)      #axis=1 is vertical

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
