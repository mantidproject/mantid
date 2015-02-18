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

def confitSeq(inputWS, func, startX, endX, ftype, bgd, temperature=None, specMin=0, specMax=None, convolve=True, Plot='None', Save=False):
    StartTime('ConvFit')

    bgd = bgd[:-2]

    num_spectra = mtd[inputWS].getNumberHistograms()
    if specMin < 0 or specMax >= num_spectra:
        raise ValueError("Invalid spectrum range: %d - %d" % (specMin, specMax))

    using_delta_func = ftype[:5] == 'Delta'
    lorentzians = ftype[5:6] if using_delta_func else ftype[:1]

    logger.information('Input files : '+str(inputWS))
    logger.information('Fit type : Delta = ' + str(using_delta_func) + ' ; Lorentzians = ' + str(lorentzians))
    logger.information('Background type : ' + bgd)

    output_workspace = getWSprefix(inputWS) + 'conv_' + ftype + bgd + '_s' + str(specMin) + "_to_" + str(specMax)

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
                       ConvolveMembers=convolve)

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
    AddSampleLog(Workspace=wsname, LogName='convolve_members', LogType='String', LogText=str(convolve))
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
        logger.information('Creating file : '+ o_path)
        SaveNexusProcessed(InputWorkspace=wsname, Filename=o_path)

    if Plot == 'All':
        plotParameters(wsname, *parameter_names)
    elif Plot != 'None':
        plotParameters(wsname, Plot)

    EndTime('ConvFit')

##############################################################################
# FuryFit
##############################################################################

def furyfitSeq(inputWS, func, ftype, startx, endx, spec_min=0, spec_max=None, intensities_constrained=False, Save=False, Plot='None'):

  StartTime('FuryFit')

  fit_type = ftype[:-2]
  logger.information('Option: ' + fit_type)
  logger.information(func)

  tmp_fit_workspace = "__furyfit_fit_ws"
  CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=tmp_fit_workspace, XMin=startx, XMax=endx)

  num_hist = mtd[inputWS].getNumberHistograms()
  if spec_max is None:
    spec_max = num_hist - 1

  # name stem for generated workspace
  output_workspace = getWSprefix(inputWS) + 'fury_' + ftype + str(spec_min) + "_to_" + str(spec_max)

  ConvertToHistogram(tmp_fit_workspace, OutputWorkspace=tmp_fit_workspace)
  convertToElasticQ(tmp_fit_workspace)

  #build input string for PlotPeakByLogValue
  input_str = [tmp_fit_workspace + ',i%d' % i for i in range(spec_min, spec_max + 1)]
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
  params = [startx, endx, fit_type]
  for i, ws in enumerate(wsnames):
    output_ws = output_workspace + '_%d_Workspace' % i
    RenameWorkspace(ws, OutputWorkspace=output_ws)

  sample_logs  = {'start_x': startx, 'end_x': endx, 'fit_type': fit_type,
                  'intensities_constrained': intensities_constrained, 'beta_constrained': False}

  CopyLogs(InputWorkspace=inputWS, OutputWorkspace=fit_group)
  CopyLogs(InputWorkspace=inputWS, OutputWorkspace=result_workspace)

  addSampleLogs(fit_group, sample_logs)
  addSampleLogs(result_workspace, sample_logs)

  if Save:
    save_workspaces = [result_workspace, fit_group]
    furyFitSaveWorkspaces(save_workspaces)

  if Plot != 'None' :
    furyfitPlotSeq(result_workspace, Plot)

  EndTime('FuryFit')
  return result_workspace


def furyfitMult(inputWS, function, ftype, startx, endx, spec_min=0, spec_max=None, intensities_constrained=False, Save=False, Plot='None'):
  StartTime('FuryFit Multi')

  nHist = mtd[inputWS].getNumberHistograms()
  output_workspace = getWSprefix(inputWS) + 'fury_1Smult_s0_to_' + str(nHist-1)

  option = ftype[:-2]
  logger.information('Option: '+option)
  logger.information('Function: '+function)

  #prepare input workspace for fitting
  tmp_fit_workspace = "__furyfit_fit_ws"
  if spec_max is None:
      CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=tmp_fit_workspace, XMin=startx, XMax=endx,
                    StartWorkspaceIndex=spec_min)
  else:
      CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=tmp_fit_workspace, XMin=startx, XMax=endx,
                    StartWorkspaceIndex=spec_min, EndWorkspaceIndex=spec_max)

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
    furyFitSaveWorkspaces(save_workspaces)

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


def furyFitSaveWorkspaces(save_workspaces):
  workdir = getDefaultWorkingDirectory()
  for ws in save_workspaces:
    #save workspace to default directory
    fpath = os.path.join(workdir, ws+'.nxs')
    SaveNexusProcessed(InputWorkspace=ws, Filename=fpath)
    logger.information(ws + ' output to file : '+fpath)


def furyfitPlotSeq(ws, plot):
    if plot == 'All':
        param_names = ['Intensity', 'Tau', 'Beta']
    else:
        param_names = [plot]

    plotParameters(ws, *param_names)


##############################################################################
# Corrections
##############################################################################

def CubicFit(inputWS, spec):
    '''
    Uses the Mantid Fit Algorithm to fit a quadratic to the inputWS
    parameter. Returns a list containing the fitted parameter values.
    '''

    function = 'name=Quadratic, A0=1, A1=0, A2=0'
    fit = Fit(Function=function, InputWorkspace=inputWS, WorkspaceIndex=spec,
              CreateOutput=True, Output='Fit')
    table = mtd['Fit_Parameters']
    A0 = table.cell(0,1)
    A1 = table.cell(1,1)
    A2 = table.cell(2,1)
    Abs = [A0, A1, A2]
    logger.information('Group '+str(spec)+' of '+inputWS+' ; fit coefficients are : '+str(Abs))
    return Abs


def subractCanWorkspace(sample, can, output_name, rebin_can=False):
    '''
    Subtract the can workspace from the sample workspace.
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


def applyCorrections(inputWS, canWS, corr, rebin_can=False):
    '''
    Through the PolynomialCorrection algorithm, makes corrections to the
    input workspace based on the supplied correction values.
    '''
    # Corrections are applied in Lambda (Wavelength)

    diffraction_run = checkUnitIs(inputWS, 'dSpacing')

    if diffraction_run:
        ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='Wavelength')
    else:
        efixed = getEfixed(inputWS)                # Get efixed
        theta, Q = GetThetaQ(inputWS)
        ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='Wavelength',
                     EMode='Indirect', EFixed=efixed)

    sam_name = getWSprefix(inputWS)
    nameStem = corr[:-4]
    corrections = mtd[corr].getNames()
    if mtd.doesExist(canWS):
        (instr, can_run) = getInstrRun(canWS)
        CorrectedWS = sam_name +'Correct_'+ can_run

        if diffraction_run:
            ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='Wavelength')
        else:
            ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='Wavelength',
                         EMode='Indirect', EFixed=efixed)
    else:
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
        logger.information(str(i) + str(mtd[CorrectedSampleWS].readX(0)))
        if len(corrections) == 1:
            Ass = CubicFit(corrections[0], i)
            PolynomialCorrection(InputWorkspace=CorrectedSampleWS, OutputWorkspace=CorrectedSampleWS,
                                 Coefficients=Ass, Operation='Divide')
            if i == 0:
                CloneWorkspace(InputWorkspace=CorrectedSampleWS, OutputWorkspace=CorrectedWS)
            else:
                ConjoinWorkspaces(InputWorkspace1=CorrectedWS, InputWorkspace2=CorrectedSampleWS)
        else:
            if mtd.doesExist(canWS):
                ExtractSingleSpectrum(InputWorkspace=canWS, OutputWorkspace=CorrectedCanWS,
                                      WorkspaceIndex=i)
                Acc = CubicFit(corrections[3], i)
                PolynomialCorrection(InputWorkspace=CorrectedCanWS, OutputWorkspace=CorrectedCanWS,
                                     Coefficients=Acc, Operation='Divide')
                Acsc = CubicFit(corrections[2], i)
                PolynomialCorrection(InputWorkspace=CorrectedCanWS, OutputWorkspace=CorrectedCanWS,
                                     Coefficients=Acsc, Operation='Multiply')

                subractCanWorkspace(CorrectedSampleWS, CorrectedCanWS, CorrectedSampleWS, rebin_can=rebin_can)

            Assc = CubicFit(corrections[1], i)
            PolynomialCorrection(InputWorkspace=CorrectedSampleWS, OutputWorkspace=CorrectedSampleWS,
                Coefficients=Assc, Operation='Divide')
            if i == 0:
                CloneWorkspace(InputWorkspace=CorrectedSampleWS, OutputWorkspace=CorrectedWS)
            else:
                ConjoinWorkspaces(InputWorkspace1=CorrectedWS, InputWorkspace2=CorrectedSampleWS,
                                  CheckOverlapping=False)

    if diffraction_run:
        ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='dSpacing')
        ConvertUnits(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS, Target='dSpacing')
    else:
        ConvertUnits(InputWorkspace=inputWS, OutputWorkspace=inputWS, Target='DeltaE',
                     EMode='Indirect', EFixed=efixed)
        ConvertUnits(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS, Target='DeltaE',
                     EMode='Indirect', EFixed=efixed)
        # Convert the spectrum axis to Q if not already in it
        sample_v_unit = mtd[CorrectedWS].getAxis(1).getUnit().unitID()
        logger.debug('COrrected workspace vertical axis is in %s' % sample_v_unit)
        if sample_v_unit != 'MomentumTransfer':
            ConvertSpectrumAxis(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS+'_rqw',
                                Target='ElasticQ', EMode='Indirect', EFixed=efixed)
        else:
            CloneWorkspace(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS + '_rqw')

    RenameWorkspace(InputWorkspace=CorrectedWS, OutputWorkspace=CorrectedWS+'_red')

    shape = mtd[corrections[0]].getRun().getLogData('sample_shape').value

    AddSampleLog(Workspace=CorrectedWS+'_red', LogName='corrections_file', LogType='String',
                 LogText=corrections[0][:-4])
    AddSampleLog(Workspace=CorrectedWS+'_red', LogName='sample_shape', LogType='String',
                 LogText=shape)

    if mtd.doesExist(canWS):
        if diffraction_run:
            ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='dSpacing')
        else:
            ConvertUnits(InputWorkspace=canWS, OutputWorkspace=canWS, Target='DeltaE',
                         EMode='Indirect', EFixed=efixed)

    DeleteWorkspace('Fit_NormalisedCovarianceMatrix')
    DeleteWorkspace('Fit_Parameters')
    DeleteWorkspace('Fit_Workspace')
    return CorrectedWS


def abscorFeeder(sample, container, geom, useCor, corrections, RebinCan=False, ScaleOrNotToScale=False, factor=1, Save=False,
        PlotResult='None', PlotContrib=False):
    '''
    Load up the necessary files and then passes them into the main
    applyCorrections routine.
    '''

    StartTime('ApplyCorrections')
    workdir = config['defaultsave.directory']
    s_hist,sxlen = CheckHistZero(sample)

    CloneWorkspace(sample, OutputWorkspace='__apply_corr_cloned_sample')
    sample = '__apply_corr_cloned_sample'
    scaled_container = "__apply_corr_scaled_container"

    diffraction_run = checkUnitIs(sample, 'dSpacing')
    sam_name = getWSprefix(sample)
    ext = '_red'

    if not diffraction_run:
        efixed = getEfixed(sample)

    if container != '':
        CheckHistSame(sample, 'Sample', container, 'Container')

        if not diffraction_run:
            CheckAnalysers(sample, container)

        if diffraction_run and not checkUnitIs(container, 'dSpacing'):
            raise ValueError("Sample and Can must both have the same units.")

        (instr, can_run) = getInstrRun(container)

        if ScaleOrNotToScale:
            #use temp workspace so we don't modify original data
            Scale(InputWorkspace=container, OutputWorkspace=scaled_container, Factor=factor, Operation='Multiply')
            logger.information('Container scaled by %f' % factor)

        else:
            CloneWorkspace(InputWorkspace=container, OutputWorkspace=scaled_container)

    if useCor:
        text = 'Correcting sample ' + sample
        if container != '':
            text += ' with ' + container
        logger.information(text)

        cor_result = applyCorrections(sample, scaled_container, corrections, RebinCan)
        rws = mtd[cor_result + ext]
        outNm = cor_result + '_Result_'

        if Save:
            cred_path = os.path.join(workdir,cor_result + ext + '.nxs')
            SaveNexusProcessed(InputWorkspace=cor_result + ext, Filename=cred_path)
            logger.information('Output file created : '+cred_path)
        calc_plot = [cor_result + ext, sample]

        if not diffraction_run:
            res_plot = cor_result + '_rqw'
        else:
            res_plot = cor_result + '_red'

    else:
        if scaled_container == '':
            raise RuntimeError('Invalid options - nothing to do!')
        else:
            sub_result = sam_name + 'Subtract_' + can_run
            logger.information('Subtracting ' + container + ' from ' + sample)

            subractCanWorkspace(sample, scaled_container, sub_result, rebin_can=RebinCan)

            if not diffraction_run:
                ConvertSpectrumAxis(InputWorkspace=sub_result, OutputWorkspace=sub_result+'_rqw',
                    Target='ElasticQ', EMode='Indirect', EFixed=efixed)

            red_ws_name = sub_result + '_red'
            RenameWorkspace(InputWorkspace=sub_result, OutputWorkspace=red_ws_name)
            CopyLogs(InputWorkspace=sample, OutputWorkspace=red_ws_name)

            rws = mtd[red_ws_name]
            outNm= sub_result + '_Result_'

            if Save:
                sred_path = os.path.join(workdir,sub_result + ext + '.nxs')
                SaveNexusProcessed(InputWorkspace=sub_result + ext, Filename=sred_path)
                logger.information('Output file created : ' + sred_path)

            if not diffraction_run:
                res_plot = sub_result + '_rqw'
            else:
                res_plot = sub_result + '_red'

    if PlotResult != 'None':
        plotCorrResult(res_plot, PlotResult)

    if mtd.doesExist(scaled_container):
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
            dataX = np.append(dataX, np.array(cws.readX(i)))
            dataY = np.append(dataY, np.array(cws.readY(i)))
            dataE = np.append(dataE, np.array(cws.readE(i)))
            dataX = np.append(dataX, np.array(rws.readX(i)))
            dataY = np.append(dataY, np.array(rws.readY(i)))
            dataE = np.append(dataE, np.array(rws.readE(i)))
            fout = outNm + str(i)

            CreateWorkspace(OutputWorkspace=fout, DataX=dataX, DataY=dataY, DataE=dataE,
                            Nspec=3, UnitX=x_unit, VerticalAxisUnit='Text', VerticalAxisValues=names)

            if i == 0:
                group = fout
            else:
                group += ',' + fout

        CopyLogs(InputWorkspace=sample, OutputWorkspace=fout)
        GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=outNm[:-1])
        if PlotContrib:
            plotCorrContrib(outNm+'0', [0, 1, 2])
        if Save:
            res_path = os.path.join(workdir,outNm[:-1] + '.nxs')
            SaveNexusProcessed(InputWorkspace=outNm[:-1], Filename=res_path)
            logger.information('Output file created : '+res_path)

        DeleteWorkspace(cws)

    EndTime('ApplyCorrections')
    return res_plot


def plotCorrResult(inWS, PlotResult):
    nHist = mtd[inWS].getNumberHistograms()
    if PlotResult == 'Spectrum' or PlotResult == 'Both':
        if nHist >= 10:                       #only plot up to 10 hists
            nHist = 10
        plot_list = []
        for i in range(0, nHist):
            plot_list.append(i)
        res_plot=mp.plotSpectrum(inWS, plot_list)
    if PlotResult == 'Contour' or PlotResult == 'Both':
        if nHist >= 5:                        #needs at least 5 hists for a contour
            mp.importMatrixWorkspace(inWS).plotGraph2D()


def plotCorrContrib(plot_list, n):
    con_plot = mp.plotSpectrum(plot_list, n)
