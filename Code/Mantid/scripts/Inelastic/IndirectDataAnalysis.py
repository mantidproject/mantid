from IndirectImport import import_mantidplot
mp = import_mantidplot()
from IndirectCommon import *

import math, re, os.path, numpy as np
from mantid.simpleapi import *
from mantid.api import TextAxis
from mantid import *

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

def calculateEISF(params_table):
    #get height data from parameter table
    height = search_for_fit_params('Height', params_table)[0]
    height_error = search_for_fit_params('Height_Err', params_table)[0]
    height_y = np.asarray(mtd[params_table].column(height))
    height_e = np.asarray(mtd[params_table].column(height_error))

    #get amplitude column names
    amp_names = search_for_fit_params('Amplitude', params_table)
    amp_error_names = search_for_fit_params('Amplitude_Err', params_table)

    #for each lorentzian, calculate EISF
    for amp_name, amp_error_name in zip(amp_names, amp_error_names):
        #get amplitude from column in table workspace
        amp_y = np.asarray(mtd[params_table].column(amp_name))
        amp_e = np.asarray(mtd[params_table].column(amp_error_name))

        #calculate EISF and EISF error
        total = height_y+amp_y
        EISF_y = height_y/total

        total_error = height_e**2 + np.asarray(amp_e)**2
        EISF_e = EISF_y * np.sqrt((height_e**2/height_y**2) + (total_error/total**2))

        #append the calculated values to the table workspace
        col_name = amp_name[:-len('Amplitude')] + 'EISF'
        error_col_name = amp_error_name[:-len('Amplitude_Err')] + 'EISF_Err'

        mtd[params_table].addColumn('double', col_name)
        mtd[params_table].addColumn('double', error_col_name)

        for i, (value, error) in enumerate(zip(EISF_y, EISF_e)):
            mtd[params_table].setCell(col_name, i, value)
            mtd[params_table].setCell(error_col_name, i, error)

##############################################################################

def confitSeq(inputWS, func, startX, endX, ftype, bgd, temperature=None, specMin=0, specMax=None, Verbose=False, Plot='None', Save=False):
    StartTime('ConvFit')

    bgd = bgd[:-2]

    num_spectra = mtd[inputWS].getNumberHistograms()
    if specMin < 0 or specMax >= num_spectra:
        raise ValueError("Invalid spectrum range: %d - %d" % (specMin, specMax))

    using_delta_func = ftype[:5] == 'Delta'
    lorentzians = ftype[5:6] if using_delta_func else ftype[:1]

    if Verbose:
        logger.notice('Input files : '+str(inputWS))
        logger.notice('Fit type : Delta = ' + str(using_delta_func) + ' ; Lorentzians = ' + str(lorentzians))
        logger.notice('Background type : ' + bgd)

    output_workspace = getWSprefix(inputWS) + 'conv_' + ftype + bgd + '_' + str(specMin) + "_to_" + str(specMax)

    #convert input workspace to get Q axis
    temp_fit_workspace = "__convfit_fit_ws"
    convertToElasticQ(inputWS, temp_fit_workspace)

    #fit all spectra in workspace
    input_params = [temp_fit_workspace+',i%d' % i
                    for i in xrange(specMin, specMax+1)]

    PlotPeakByLogValue(Input=';'.join(input_params),
                       OutputWorkspace=output_workspace, Function=func,
                       StartX=startX, EndX=endX, FitType='Sequential',
                       CreateOutput=True, OutputCompositeMembers=True,
                       ConvolveMembers=True)

    DeleteWorkspace(output_workspace + '_NormalisedCovarianceMatrices')
    DeleteWorkspace(output_workspace + '_Parameters')
    DeleteWorkspace(temp_fit_workspace)

    wsname = output_workspace + "_Result"
    parameter_names = ['Height', 'Amplitude', 'FWHM', 'EISF']
    if using_delta_func:
        calculateEISF(output_workspace)
    convertParametersToWorkspace(output_workspace, "axis-1", parameter_names, wsname)

    #set x units to be momentum transfer
    axis = mtd[wsname].getAxis(0)
    axis.setUnit("MomentumTransfer")

    CopyLogs(InputWorkspace=inputWS, OutputWorkspace=wsname)
    AddSampleLog(Workspace=wsname, LogName="fit_program", LogType="String", LogText='ConvFit')
    AddSampleLog(Workspace=wsname, LogName='background', LogType='String', LogText=str(bgd))
    AddSampleLog(Workspace=wsname, LogName='delta_function', LogType='String', LogText=str(using_delta_func))
    AddSampleLog(Workspace=wsname, LogName='lorentzians', LogType='String', LogText=str(lorentzians))

    CopyLogs(InputWorkspace=wsname, OutputWorkspace=output_workspace + "_Workspaces")

    temp_correction = temperature is not None
    AddSampleLog(Workspace=wsname, LogName='temperature_correction', LogType='String', LogText=str(temp_correction))
    if temp_correction:
        AddSampleLog(Workspace=wsname, LogName='temperature_value', LogType='String', LogText=str(temperature))

    RenameWorkspace(InputWorkspace=output_workspace, OutputWorkspace=output_workspace + "_Parameters")
    fit_workspaces = mtd[output_workspace + '_Workspaces'].getNames()
    for i, ws in enumerate(fit_workspaces):
        RenameWorkspace(ws, OutputWorkspace=output_workspace + '_' + str(i+specMin) + '_Workspace')

    if Save:
        # path name for nxs file
        workdir = getDefaultWorkingDirectory()
        o_path = os.path.join(workdir, wsname+'.nxs')
        if Verbose:
            logger.notice('Creating file : '+ o_path)
        SaveNexusProcessed(InputWorkspace=wsname, Filename=o_path)

    if Plot == 'All':
        plotParameters(wsname, *parameter_names)
    elif Plot != 'None':
        plotParameters(wsname, Plot)

    EndTime('ConvFit')

##############################################################################
# Elwin
##############################################################################

def GetTemperature(root, tempWS, log_type, Verbose):
    (instr, run_number) = getInstrRun(tempWS)

    facility = config.getFacility()
    pad_num = facility.instrument(instr).zeroPadding(int(run_number))
    zero_padding = '0' * (pad_num - len(run_number))

    run_name = instr + zero_padding + run_number
    log_name = run_name.upper() + '.log'

    run = mtd[tempWS].getRun()
    unit = ['Temperature', 'K']
    if log_type in run:
        # test logs in WS
        tmp = run[log_type].value
        temp = tmp[len(tmp)-1]

        if Verbose:
            mess = ' Run : '+run_name +' ; Temperature in log = '+str(temp)
            logger.notice(mess)
    else:
        # logs not in WS
        logger.warning('Log parameter not found in workspace. Searching for log file.')
        log_path = FileFinder.getFullPath(log_name)

        if log_path != '':
            # get temperature from log file
            LoadLog(Workspace=tempWS, Filename=log_path)
            run_logs = mtd[tempWS].getRun()
            tmp = run_logs[log_type].value
            temp = tmp[len(tmp)-1]
            mess = ' Run : '+run_name+' ; Temperature in file = '+str(temp)
            logger.warning(mess)
        else:
            # can't find log file
            temp = int(run_name[-3:])
            unit = ['Run-number', 'last 3 digits']
            mess = ' Run : '+run_name +' ; Temperature file not found'
            logger.warning(mess)

    return temp,unit

def elwin(inputFiles, eRange, log_type='sample', Normalise = False,
        Save=False, Verbose=False, Plot=False):
    StartTime('ElWin')
    workdir = config['defaultsave.directory']
    CheckXrange(eRange,'Energy')
    tempWS = '__temp'
    Range2 = ( len(eRange) == 4 )
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
        (xval, unit) = GetTemperature(root,tempWS,log_type, Verbose)
        if Verbose:
            logger.notice('Reading file : '+file)
        if ( len(eRange) == 4 ):
            ElasticWindow(InputWorkspace=tempWS, Range1Start=eRange[0], Range1End=eRange[1],
                Range2Start=eRange[2], Range2End=eRange[3],
                OutputInQ='__eq1', OutputInQSquared='__eq2')
        elif ( len(eRange) == 2 ):
            ElasticWindow(InputWorkspace=tempWS, Range1Start=eRange[0], Range1End=eRange[1],
                OutputInQ='__eq1', OutputInQSquared='__eq2')
        (instr, last) = getInstrRun(tempWS)
        q1 = np.array(mtd['__eq1'].readX(0))
        i1 = np.array(mtd['__eq1'].readY(0))
        e1 = np.array(mtd['__eq1'].readE(0))
        Logarithm(InputWorkspace='__eq2', OutputWorkspace='__eq2')
        q2 = np.array(mtd['__eq2'].readX(0))
        i2 = np.array(mtd['__eq2'].readY(0))
        e2 = np.array(mtd['__eq2'].readE(0))
        if (nr == 0):
            CloneWorkspace(InputWorkspace='__eq1', OutputWorkspace='__elf')
            first = getWSprefix(tempWS)
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

    DeleteWorkspace('__eq1')
    DeleteWorkspace('__eq2')
    DeleteWorkspace('__elf')

    if (nr == 1):
        ename = first[:-1]
    else:
        ename = first+'to_'+last

    #check if temp was increasing or decreasing
    if(datTx[0] > datTx[-1]):
        # if so reverse data to follow natural ordering
        datTx = datTx[::-1]
        datTy = datTy[::-1]
        datTe = datTe[::-1]

    elfWS = ename+'_elf'
    e1WS = ename+'_eq1'
    e2WS = ename+'_eq2'
    #elt only created if we normalise
    eltWS = None

    wsnames = [elfWS, e1WS, e2WS]

    #x,y,e data for the elf, e1 and e2 workspaces
    data = [[datTx, datTy, datTe],
            [datX1, datY1, datE1],
            [datX2, datY2, datE2]]

    #x and vertical units for the elf, e1 and e2 workspaces
    xunits = ['Energy', 'MomentumTransfer', 'QSquared']
    vunits = ['MomentumTransfer', 'Energy', 'Energy']

    #vertical axis values for the elf, e1 and e2 workspaces
    vvalues = [q1, Taxis, Taxis]

    #number of spectra in each workspace
    nspecs =  [nQ, nr, nr]

    #x-axis units label
    label = unit[0]+' / '+unit[1]

    wsInfo = zip(wsnames,data, xunits, vunits, vvalues, nspecs)

    #Create output workspaces and add sample logs
    for wsname, wsdata, xunit, vunit, vvalue, nspec in wsInfo:
        x, y, e = wsdata

        CreateWorkspace(OutputWorkspace=wsname, DataX=x, DataY=y, DataE=e,
            Nspec=nspec, UnitX=xunit, VerticalAxisUnit=vunit, VerticalAxisValues=vvalue)

        #add sample logs to new workspace
        CopyLogs(InputWorkspace=tempWS, OutputWorkspace=wsname)
        addElwinLogs(wsname, label, eRange, Range2)

    # remove the temp workspace now we've copied the logs
    DeleteWorkspace(tempWS)

    if unit[0] == 'Temperature':

        AddSampleLog(Workspace=e1WS, LogName="temp_normalise",
            LogType="String", LogText=str(Normalise))

        #create workspace normalized to the lowest temperature
        if Normalise:
            eltWS = ename+'_elt'

            #create elt workspace
            mtd[elfWS].clone(OutputWorkspace=eltWS)
            elwinNormalizeToLowestTemp(eltWS)

            #set labels and meta data
            unitx = mtd[eltWS].getAxis(0).setUnit("Label")
            unitx.setLabel(unit[0], unit[1])
            addElwinLogs(eltWS, label, eRange, Range2)

            #append workspace name to output files list
            wsnames.append(eltWS)

    #set labels on workspace axes
    unity = mtd[e1WS].getAxis(1).setUnit("Label")
    unity.setLabel(unit[0], unit[1])

    unity = mtd[e2WS].getAxis(1).setUnit("Label")
    unity.setLabel(unit[0], unit[1])

    unitx = mtd[elfWS].getAxis(0).setUnit("Label")
    unitx.setLabel(unit[0], unit[1])

    if Save:
        elwinSaveWorkspaces(wsnames, workdir, Verbose)

    if Plot:
        elwinPlot(label,e1WS,e2WS,elfWS,eltWS)

    EndTime('Elwin')
    return e1WS,e2WS

#normalize workspace to the lowest temperature
def elwinNormalizeToLowestTemp(eltWS):
    nhist = mtd[eltWS].getNumberHistograms()

    #normalize each spectrum in the workspace
    for n in range(0,nhist):
        y = mtd[eltWS].readY(n)
        scale = 1.0/y[0]
        yscaled = scale * y
        mtd[eltWS].setY(n, yscaled)

# Write each of the created workspaces to file
def elwinSaveWorkspaces(flist, dir, Verbose):
    for fname in flist:
        fpath = os.path.join(dir, fname+'.nxs')

        if Verbose:
            logger.notice('Creating file : '+ fpath)

        SaveNexusProcessed(InputWorkspace=fname, Filename=fpath)

# Add sample log to each of the workspaces created by Elwin
def addElwinLogs(ws, label, eRange, Range2):

    AddSampleLog(Workspace=ws, LogName="vert_axis", LogType="String", LogText=label)
    AddSampleLog(Workspace=ws, LogName="range1_start", LogType="Number", LogText=str(eRange[0]))
    AddSampleLog(Workspace=ws, LogName="range1_end", LogType="Number", LogText=str(eRange[1]))
    AddSampleLog(Workspace=ws, LogName="two_ranges", LogType="String", LogText=str(Range2))

    if Range2:
        AddSampleLog(Workspace=ws, LogName="range2_start", LogType="Number", LogText=str(eRange[2]))
        AddSampleLog(Workspace=ws, LogName="range2_end", LogType="Number", LogText=str(eRange[3]))

#Plot each of the workspace output by elwin
def elwinPlot(label,eq1,eq2,elf,elt):
    plotElwinWorkspace(eq1, yAxisTitle='Elastic Intensity', setScale=True)
    plotElwinWorkspace(eq2, yAxisTitle='log(Elastic Intensity)', setScale=True)
    plotElwinWorkspace(elf, xAxisTitle=label)

    if elt is not None:
        plotElwinWorkspace(elt, xAxisTitle=label)

#Plot a workspace generated by Elwin
def plotElwinWorkspace(ws, xAxisTitle=None, yAxisTitle=None, setScale=False):
    ws = mtd[ws]
    nBins = ws.blocksize()
    lastX = ws.readX(0)[nBins-1]

    nhist = ws.getNumberHistograms()

    try:
        graph = mp.plotSpectrum(ws, range(0,nhist))
    except RuntimeError, e:
        #User clicked cancel on plot so don't do anything
        return None

    layer = graph.activeLayer()

    #set the x scale of the layer
    if setScale:
        layer.setScale(mp.Layer.Bottom, 0.0, lastX)

    #set the title on the x-axis
    if xAxisTitle:
        layer.setAxisTitle(mp.Layer.Bottom, xAxisTitle)

    #set the title on the y-axis
    if yAxisTitle:
        layer.setAxisTitle(mp.Layer.Left, yAxisTitle)

##############################################################################
# Fury
##############################################################################

def furyPlot(inWS, spec):
    graph = mp.plotSpectrum(inWS, spec)
    layer = graph.activeLayer()
    layer.setScale(mp.Layer.Left, 0, 1.0)

def fury(samWorkspaces, res_workspace, rebinParam, RES=True, Save=False, Verbose=False,
        Plot=False):

    StartTime('Fury')
    workdir = config['defaultsave.directory']
    samTemp = samWorkspaces[0]
    nsam,npt = CheckHistZero(samTemp)
    Xin = mtd[samTemp].readX(0)
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
    CheckAnalysers(samTemp, res_workspace, Verbose)
    nres,nptr = CheckHistZero(res_workspace)
    if nres > 1:
        CheckHistSame(samTemp,'Sample', res_workspace, 'Resolution')

    tmp_res_workspace = '__tmp_' + res_workspace
    Rebin(InputWorkspace=res_workspace, OutputWorkspace=tmp_res_workspace, Params=rebinParam)
    Integration(InputWorkspace=tmp_res_workspace, OutputWorkspace='res_int')
    ConvertToPointData(InputWorkspace=tmp_res_workspace, OutputWorkspace=tmp_res_workspace)
    ExtractFFTSpectrum(InputWorkspace=tmp_res_workspace, OutputWorkspace='res_fft', FFTPart=2)
    Divide(LHSWorkspace='res_fft', RHSWorkspace='res_int', OutputWorkspace='res')
    for samWs in samWorkspaces:
        (direct, filename) = os.path.split(samWs)
        (root, ext) = os.path.splitext(filename)
        Rebin(InputWorkspace=samWs, OutputWorkspace='sam_data', Params=rebinParam)
        Integration(InputWorkspace='sam_data', OutputWorkspace='sam_int')
        ConvertToPointData(InputWorkspace='sam_data', OutputWorkspace='sam_data')
        ExtractFFTSpectrum(InputWorkspace='sam_data', OutputWorkspace='sam_fft', FFTPart=2)
        Divide(LHSWorkspace='sam_fft', RHSWorkspace='sam_int', OutputWorkspace='sam')
        # Create save file name
        savefile = getWSprefix(samWs) + 'iqt'
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
            opath = os.path.join(workdir, savefile+'.nxs')    				# path name for nxs file
            SaveNexusProcessed(InputWorkspace=savefile, Filename=opath)
            if Verbose:
                logger.notice('Output file : '+opath)
    # Clean Up RES files
    DeleteWorkspace(tmp_res_workspace)
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


def furyfitSeq(inputWS, func, ftype, startx, endx, intensities_constrained=False, Save=False, Plot='None', Verbose=False):

  StartTime('FuryFit')
  nHist = mtd[inputWS].getNumberHistograms()

  #name stem for generated workspace
  output_workspace = getWSprefix(inputWS) + 'fury_' + ftype + "0_to_" + str(nHist-1)

  fitType = ftype[:-2]
  if Verbose:
    logger.notice('Option: '+fitType)
    logger.notice(func)

  tmp_fit_workspace = "__furyfit_fit_ws"
  CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=tmp_fit_workspace, XMin=startx, XMax=endx)
  ConvertToHistogram(tmp_fit_workspace, OutputWorkspace=tmp_fit_workspace)
  convertToElasticQ(tmp_fit_workspace)

  #build input string for PlotPeakByLogValue
  input_str = [tmp_fit_workspace + ',i%d' % i for i in range(0,nHist)]
  input_str = ';'.join(input_str)

  PlotPeakByLogValue(Input=input_str, OutputWorkspace=output_workspace, Function=func,
                     StartX=startx, EndX=endx, FitType='Sequential', CreateOutput=True)

  #remove unsused workspaces
  DeleteWorkspace(output_workspace + '_NormalisedCovarianceMatrices')
  DeleteWorkspace(output_workspace + '_Parameters')

  fit_group = output_workspace + '_Workspaces'
  params_table = output_workspace + '_Parameters'
  RenameWorkspace(output_workspace, OutputWorkspace=params_table)

  #create *_Result workspace
  result_workspace = output_workspace + "_Result"
  parameter_names = ['A0', 'Intensity', 'Tau', 'Beta']
  convertParametersToWorkspace(params_table, "axis-1", parameter_names, result_workspace)

  #set x units to be momentum transfer
  axis = mtd[result_workspace].getAxis(0)
  axis.setUnit("MomentumTransfer")

  #process generated workspaces
  wsnames = mtd[fit_group].getNames()
  params = [startx, endx, fitType]
  for i, ws in enumerate(wsnames):
    output_ws = output_workspace + '_%d_Workspace' % i
    RenameWorkspace(ws, OutputWorkspace=output_ws)

  sample_logs  = {'start_x': startx, 'end_x': endx, 'fit_type': ftype,
                  'intensities_constrained': intensities_constrained, 'beta_constrained': False}

  CopyLogs(InputWorkspace=inputWS, OutputWorkspace=fit_group)
  CopyLogs(InputWorkspace=inputWS, OutputWorkspace=result_workspace)

  addSampleLogs(fit_group, sample_logs)
  addSampleLogs(result_workspace, sample_logs)

  if Save:
    save_workspaces = [result_workspace, fit_group]
    furyFitSaveWorkspaces(save_workspaces, Verbose)

  if Plot != 'None' :
    furyfitPlotSeq(result_workspace, Plot)

  EndTime('FuryFit')
  return result_workspace


def furyfitMult(inputWS, function, ftype, startx, endx, intensities_constrained=False, Save=False, Plot='None', Verbose=False):
  StartTime('FuryFit Multi')

  nHist = mtd[inputWS].getNumberHistograms()
  output_workspace = getWSprefix(inputWS) + 'fury_1Smult_s0_to_' + str(nHist-1)

  option = ftype[:-2]
  if Verbose:
    logger.notice('Option: '+option)
    logger.notice('Function: '+function)

  #prepare input workspace for fitting
  tmp_fit_workspace = "__furyfit_fit_ws"
  CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=tmp_fit_workspace, XMin=startx, XMax=endx)
  ConvertToHistogram(tmp_fit_workspace, OutputWorkspace=tmp_fit_workspace)
  convertToElasticQ(tmp_fit_workspace)

  #fit multi-domian functino to workspace
  multi_domain_func, kwargs = createFuryMultiDomainFunction(function, tmp_fit_workspace)
  Fit(Function=multi_domain_func, InputWorkspace=tmp_fit_workspace, WorkspaceIndex=0,
      Output=output_workspace, CreateOutput=True, **kwargs)

  params_table = output_workspace + '_Parameters'
  transposeFitParametersTable(params_table)

  #set first column of parameter table to be axis values
  ax = mtd[tmp_fit_workspace].getAxis(1)
  axis_values = ax.extractValues()
  for i, value in enumerate(axis_values):
    mtd[params_table].setCell('axis-1', i, value)

  #convert parameters to matrix workspace
  result_workspace = output_workspace + "_Result"
  parameter_names = ['A0', 'Intensity', 'Tau', 'Beta']
  convertParametersToWorkspace(params_table, "axis-1", parameter_names, result_workspace)

  #set x units to be momentum transfer
  axis = mtd[result_workspace].getAxis(0)
  axis.setUnit("MomentumTransfer")

  result_workspace = output_workspace + '_Result'
  fit_group = output_workspace + '_Workspaces'

  sample_logs  = {'start_x': startx, 'end_x': endx, 'fit_type': ftype,
                  'intensities_constrained': intensities_constrained, 'beta_constrained': True}

  CopyLogs(InputWorkspace=inputWS, OutputWorkspace=result_workspace)
  CopyLogs(InputWorkspace=inputWS, OutputWorkspace=fit_group)

  addSampleLogs(result_workspace, sample_logs)
  addSampleLogs(fit_group, sample_logs)

  DeleteWorkspace(tmp_fit_workspace)

  if Save:
    save_workspaces = [result_workspace]
    furyFitSaveWorkspaces(save_workspaces, Verbose)

  if Plot != 'None':
    furyfitPlotSeq(result_workspace, Plot)

  EndTime('FuryFit Multi')
  return result_workspace


def createFuryMultiDomainFunction(function, input_ws):
  multi= 'composite=MultiDomainFunction,NumDeriv=1;'
  comp =  '(composite=CompositeFunction,$domains=i;' + function + ');'

  ties = []
  kwargs = {}
  num_spectra = mtd[input_ws].getNumberHistograms()
  for i in range(0, num_spectra):
    multi += comp
    kwargs['WorkspaceIndex_' + str(i)] = i

    if i > 0:
      kwargs['InputWorkspace_' + str(i)] = input_ws

      #tie beta for every spectrum
      tie = 'f%d.f1.Beta=f0.f1.Beta' % i
      ties.append(tie)

  ties = ','.join(ties)
  multi += 'ties=(' + ties + ')'

  return multi, kwargs


def furyFitSaveWorkspaces(save_workspaces, Verbose):
  workdir = getDefaultWorkingDirectory()
  for ws in save_workspaces:
    #save workspace to default directory
    fpath = os.path.join(workdir, ws+'.nxs')
    SaveNexusProcessed(InputWorkspace=ws, Filename=fpath)

    if Verbose:
      logger.notice(ws + ' output to file : '+fpath)


def furyfitPlotSeq(ws, plot):
    if plot == 'All':
        param_names = ['Intensity', 'Tau', 'Beta']
    else:
        param_names = [plot]

    plotParameters(ws, *param_names)


##############################################################################
# MSDFit
##############################################################################

def msdfitPlotSeq(inputWS, xlabel):
    ws = mtd[inputWS+'_A1']
    if len(ws.readX(0)) > 1:
        msd_plot = mp.plotSpectrum(inputWS+'_A1',0,True)
        msd_layer = msd_plot.activeLayer()
        msd_layer.setAxisTitle(mp.Layer.Bottom,xlabel)
        msd_layer.setAxisTitle(mp.Layer.Left,'<u2>')

def msdfit(ws, startX, endX, spec_min=0, spec_max=None, Save=False, Verbose=False, Plot=True):
    StartTime('msdFit')
    workdir = getDefaultWorkingDirectory()

    num_spectra = mtd[ws].getNumberHistograms()
    if spec_max is None:
        spec_max = num_spectra - 1

    if spec_min < 0 or spec_max >= num_spectra:
        raise ValueError("Invalid spectrum range: %d - %d" % (spec_min, spec_max))

    xlabel = ''
    ws_run = mtd[ws].getRun()

    if 'vert_axis' in ws_run:
        xlabel = ws_run.getLogData('vert_axis').value

    mname = ws[:-4]
    msdWS = mname+'_msd'

    #fit line to each of the spectra
    function = 'name=LinearBackground, A0=0, A1=0'
    input_params = [ ws+',i%d' % i for i in xrange(spec_min, spec_max+1)]
    input_params = ';'.join(input_params)
    PlotPeakByLogValue(Input=input_params, OutputWorkspace=msdWS, Function=function,
                       StartX=startX, EndX=endX, FitType='Sequential', CreateOutput=True)

    DeleteWorkspace(msdWS + '_NormalisedCovarianceMatrices')
    DeleteWorkspace(msdWS + '_Parameters')
    msd_parameters = msdWS+'_Parameters'
    RenameWorkspace(msdWS, OutputWorkspace=msd_parameters)

    params_table = mtd[msd_parameters]

    #msd value should be positive, but the fit output is negative
    msd = params_table.column('A1')
    for i, value in enumerate(msd):
        params_table.setCell('A1', i, value * -1)

    #create workspaces for each of the parameters
    group = []

    ws_name = msdWS + '_A0'
    group.append(ws_name)
    ConvertTableToMatrixWorkspace(msd_parameters, OutputWorkspace=ws_name,
                                  ColumnX='axis-1', ColumnY='A0', ColumnE='A0_Err')
    xunit = mtd[ws_name].getAxis(0).setUnit('Label')
    xunit.setLabel('Temperature', 'K')

    ws_name = msdWS + '_A1'
    group.append(ws_name)
    ConvertTableToMatrixWorkspace(msd_parameters, OutputWorkspace=ws_name,
                                  ColumnX='axis-1', ColumnY='A1', ColumnE='A1_Err')

    SortXAxis(ws_name, OutputWorkspace=ws_name)

    xunit = mtd[ws_name].getAxis(0).setUnit('Label')
    xunit.setLabel('Temperature', 'K')

    GroupWorkspaces(InputWorkspaces=','.join(group),OutputWorkspace=msdWS)

    #add sample logs to output workspace
    fit_workspaces = msdWS + '_Workspaces'
    CopyLogs(InputWorkspace=ws, OutputWorkspace=msdWS)
    AddSampleLog(Workspace=msdWS, LogName="start_x", LogType="Number", LogText=str(startX))
    AddSampleLog(Workspace=msdWS, LogName="end_x", LogType="Number", LogText=str(endX))
    CopyLogs(InputWorkspace=msdWS + '_A0', OutputWorkspace=fit_workspaces)

    if Plot:
        msdfitPlotSeq(msdWS, xlabel)
    if Save:
        msd_path = os.path.join(workdir, msdWS+'.nxs')                  # path name for nxs file
        SaveNexusProcessed(InputWorkspace=msdWS, Filename=msd_path, Title=msdWS)
        if Verbose:
            logger.notice('Output msd file : '+msd_path)

    EndTime('msdFit')
    return fit_workspaces

def plotInput(inputfiles,spectra=[]):
    OneSpectra = False
    if len(spectra) != 2:
        spectra = [spectra[0], spectra[0]]
        OneSpectra = True
    workspaces = []
    for file in inputfiles:
        root = LoadNexus(Filename=file)
        if not OneSpectra:
            GroupDetectors(root, root, DetectorList=range(spectra[0],spectra[1]+1) )
        workspaces.append(root)
    if len(workspaces) > 0:
        graph = mp.plotSpectrum(workspaces,0)
        graph.activeLayer().setTitle(", ".join(workspaces))

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

def subractCanWorkspace(sample, can, output_name, rebin_can=False):
    '''Subtract the can workspace from the sample workspace.
    Optionally rebin the can to match the sample.

    @param sample :: sample workspace to use subract from
    @param can :: can workspace to subtract
    @param rebin_can :: whether to rebin the can first.
    @return corrected sample workspace
    '''

    if rebin_can:
        logger.warning("Sample and Can do not match. Rebinning Can to match Sample.")
        RebinToWorkspace(WorkspaceToRebin=can, WorkspaceToMatch=sample, OutputWorkspace=can)

    try:
        Minus(LHSWorkspace=sample, RHSWorkspace=can, OutputWorkspace=output_name)
    except ValueError:
        raise ValueError("Sample and Can energy ranges do not match. \
                         Do they have the same binning?")


def applyCorrections(inputWS, canWS, corr, rebin_can=False, Verbose=False):
    '''Through the PolynomialCorrection algorithm, makes corrections to the
    input workspace based on the supplied correction values.'''
    # Corrections are applied in Lambda (Wavelength)

    efixed = getEfixed(inputWS)                # Get efixed
    ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='Wavelength',
        EMode='Indirect', EFixed=efixed)

    nameStem = corr[:-4]
    if canWS != '':
        (instr, can_run) = getInstrRun(canWS)
        corrections = [nameStem+'_ass', nameStem+'_assc', nameStem+'_acsc', nameStem+'_acc']
        CorrectedWS = sam_name +'Correct_'+ can_run
        ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='Wavelength',
            EMode='Indirect', EFixed=efixed)
    else:
        corrections = [nameStem+'_ass']
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

            subractCanWorkspace(CorrectedSampleWS, CorrectedCanWS, CorrectedSampleWS, rebin_can=rebin_can)

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
    ConvertSpectrumAxis(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS+'_rqw',
        Target='ElasticQ', EMode='Indirect', EFixed=efixed)

    RenameWorkspace(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS+'_red')

    if canWS != '':
        ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='DeltaE',
            EMode='Indirect', EFixed=efixed)

    DeleteWorkspace('Fit_NormalisedCovarianceMatrix')
    DeleteWorkspace('Fit_Parameters')
    DeleteWorkspace('Fit_Workspace')
    return CorrectedWS

def abscorFeeder(sample, container, geom, useCor, corrections, Verbose=False, RebinCan=False, ScaleOrNotToScale=False, factor=1, Save=False,
        PlotResult='None', PlotContrib=False):
    '''Load up the necessary files and then passes them into the main
    applyCorrections routine.'''
    StartTime('ApplyCorrections')
    workdir = config['defaultsave.directory']
    s_hist,sxlen = CheckHistZero(sample)

    diffraction_run = checkUnitIs(sample, 'dSpacing')
    sam_name = getWSprefix(sample)
    ext = '_red'

    if not diffraction_run:
        efixed = getEfixed(sample)

    if container != '':
        CheckHistSame(sample, 'Sample', container, 'Container')

        if not diffraction_run:
            CheckAnalysers(sample, container, Verbose)

        if diffraction_run and not checkUnitIs(container, 'dSpacing'):
            raise ValueError("Sample and Can must both have the same units.")

        (instr, can_run) = getInstrRun(container)

        scaled_container = "__apply_corr_scaled_container"
        if ScaleOrNotToScale:
            #use temp workspace so we don't modify original data
            Scale(InputWorkspace=container, OutputWorkspace=scaled_container, Factor=factor, Operation='Multiply')

            if Verbose:
                logger.notice('Container scaled by %f' % factor)
        else:
            CloneWorkspace(InputWorkspace=container, OutputWorkspace=scaled_container)

    if useCor:
        if diffraction_run:
            raise NotImplementedError("Applying absorption corrections is not currently supported for diffraction data.")

        if Verbose:
            text = 'Correcting sample ' + sample
            if scaled_container != '':
                text += ' with ' + scaled_container
            logger.notice(text)

        cor_result = applyCorrections(sample, container, corrections, RebinCan, Verbose)
        rws = mtd[cor_result + ext]
        outNm = cor_result + '_Result_'

        if Save:
            cred_path = os.path.join(workdir,cor_result + ext + '.nxs')
            SaveNexusProcessed(InputWorkspace=cor_result + ext, Filename=cred_path)
            if Verbose:
                logger.notice('Output file created : '+cred_path)
        calc_plot = [cor_result + ext, sample]
        res_plot = cor_result+'_rqw'
    else:
        if ( scaled_container == '' ):
            sys.exit('ERROR *** Invalid options - nothing to do!')
        else:
            sub_result = sam_name +'Subtract_'+ can_run
            if Verbose:
                logger.notice('Subtracting '+container+' from '+sample)

            subractCanWorkspace(sample, scaled_container, sub_result, rebin_can=RebinCan)

            if not diffraction_run:
                ConvertSpectrumAxis(InputWorkspace=sub_result, OutputWorkspace=sub_result+'_rqw',
                    Target='ElasticQ', EMode='Indirect', EFixed=efixed)

            RenameWorkspace(InputWorkspace=sub_result, OutputWorkspace=sub_result+'_red')
            rws = mtd[sub_result+'_red']
            outNm= sub_result + '_Result_'

            if Save:
                sred_path = os.path.join(workdir,sub_result + ext + '.nxs')
                SaveNexusProcessed(InputWorkspace=sub_result + ext, Filename=sred_path)
                if Verbose:
                    logger.notice('Output file created : ' + sred_path)

            if not diffraction_run:
                res_plot = sub_result + '_rqw'
            else:
                res_plot = sub_result + '_red'

    if (PlotResult != 'None'):
        plotCorrResult(res_plot, PlotResult)

    if ( scaled_container != '' ):
        sws = mtd[sample]
        cws = mtd[scaled_container]
        names = 'Sample,Can,Calc'

        x_unit = 'DeltaE'
        if diffraction_run:
            x_unit = 'dSpacing'

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
                Nspec=3, UnitX=x_unit, VerticalAxisUnit='Text', VerticalAxisValues=names)

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

        DeleteWorkspace(cws)
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
