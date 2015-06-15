#pylint: disable=invalid-name
from IndirectImport import import_mantidplot
MTD_PLOT = import_mantidplot()
from IndirectCommon import *

import math, re, os.path, numpy as np
from mantid.simpleapi import *
from mantid.api import TextAxis
from mantid import *


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


def confitSeq(inputWS, func, startX, endX, ftype, bgd,
              temperature=None, specMin=0, specMax=None, convolve=True,
              minimizer='Levenberg-Marquardt', max_iterations=500,
              Plot='None', Save=False):
    StartTime('ConvFit')

    bgd = bgd[:-2]

    num_spectra = mtd[inputWS].getNumberHistograms()
    if specMin < 0 or specMax >= num_spectra:
        raise ValueError("Invalid spectrum range: %d - %d" % (specMin, specMax))

    using_delta_func = ftype[:5] == 'Delta'
    lorentzians = ftype[5:6] if using_delta_func else ftype[:1]

    logger.information('Input files: ' + str(inputWS))
    logger.information('Fit type: Delta=%s; Lorentzians=%s' % (
                       str(using_delta_func), str(lorentzians)))
    logger.information('Background type: ' + bgd)

    output_workspace = '%sconv_%s%s_s%d_to_%d' % (
                       getWSprefix(inputWS), ftype, bgd, specMin, specMax)

    #convert input workspace to get Q axis
    temp_fit_workspace = "__convfit_fit_ws"
    convertToElasticQ(inputWS, temp_fit_workspace)

    #fit all spectra in workspace
    input_params = [temp_fit_workspace+',i%d' % i
                    for i in xrange(specMin, specMax+1)]

    fit_args = dict()
    if 'DS' in ftype or 'DC' in ftype:
        fit_args['PassWSIndexToFunction'] = True

    PlotPeakByLogValue(Input=';'.join(input_params),
                       OutputWorkspace=output_workspace,
                       Function=func,
                       StartX=startX,
                       EndX=endX,
                       FitType='Sequential',
                       CreateOutput=True,
                       OutputCompositeMembers=True,
                       ConvolveMembers=convolve,
                       MaxIterations=max_iterations,
                       Minimizer=minimizer,
                       **fit_args)

    DeleteWorkspace(output_workspace + '_NormalisedCovarianceMatrices')
    DeleteWorkspace(output_workspace + '_Parameters')
    DeleteWorkspace(temp_fit_workspace)

    wsname = output_workspace + '_Result'

    if 'DS' in ftype:
        parameter_names = ['Height', 'Intensity', 'Radius', 'Diffusion', 'Shift']
    elif 'DC' in ftype:
        parameter_names = ['Height', 'Intensity', 'Radius', 'Decay', 'Shift']
    else:
        parameter_names = ['Height', 'Amplitude', 'FWHM', 'EISF']

    if using_delta_func:
        calculateEISF(output_workspace)

    convertParametersToWorkspace(output_workspace, "axis-1", parameter_names, wsname)

    #set x units to be momentum transfer
    axis = mtd[wsname].getAxis(0)
    axis.setUnit("MomentumTransfer")

    # Handle sample logs
    temp_correction = temperature is not None

    CopyLogs(InputWorkspace=inputWS, OutputWorkspace=wsname)

    sample_logs = [('sam_workspace', inputWS),
                   ('convolve_members', convolve),
                   ('fit_program', 'ConvFit'),
                   ('background', bgd),
                   ('delta_function', using_delta_func),
                   ('lorentzians', lorentzians),
                   ('temperature_correction', temp_correction)]

    if temp_correction:
        sample_logs.append(('temperature_value', temperature))

    log_names = [log[0] for log in sample_logs]
    log_values = [log[1] for log in sample_logs]
    AddSampleLogMultiple(Workspace=wsname,
                         LogNames=log_names,
                         LogValues=log_values)

    CopyLogs(InputWorkspace=wsname, OutputWorkspace=output_workspace + "_Workspaces")

    RenameWorkspace(InputWorkspace=output_workspace,
                    OutputWorkspace=output_workspace + "_Parameters")
    fit_workspaces = mtd[output_workspace + '_Workspaces'].getNames()
    for i, workspace in enumerate(fit_workspaces):
        RenameWorkspace(workspace,
                        OutputWorkspace='%s_%d_Workspace' % (output_workspace, i + specMin))

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

    return wsname


##############################################################################
# FuryFit
##############################################################################

def furyfitSeq(inputWS, func, ftype, startx, endx,
               spec_min=0, spec_max=None,
               intensities_constrained=False,
               minimizer='Levenberg-Marquardt', max_iterations=500,
               Save=False, Plot='None'):
    StartTime('FuryFit')

    fit_type = ftype[:-2]
    logger.information('Option: ' + fit_type)
    logger.information(func)

    tmp_fit_workspace = "__furyfit_fit_ws"
    CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=tmp_fit_workspace, XMin=startx, XMax=endx)

    num_hist = mtd[inputWS].getNumberHistograms()
    if spec_max is None:
        spec_max = num_hist - 1

    # Name stem for generated workspace
    output_workspace = '%sfury_%s%d_to_%d' % (getWSprefix(inputWS), ftype, spec_min, spec_max)

    ConvertToHistogram(tmp_fit_workspace, OutputWorkspace=tmp_fit_workspace)
    convertToElasticQ(tmp_fit_workspace)

    # Build input string for PlotPeakByLogValue
    input_str = [tmp_fit_workspace + ',i%d' % i for i in range(spec_min, spec_max + 1)]
    input_str = ';'.join(input_str)

    PlotPeakByLogValue(Input=input_str,
                       OutputWorkspace=output_workspace,
                       Function=func,
                       Minimizer=minimizer,
                       MaxIterations=max_iterations,
                       StartX=startx,
                       EndX=endx,
                       FitType='Sequential',
                       CreateOutput=True)

    # Remove unsused workspaces
    DeleteWorkspace(output_workspace + '_NormalisedCovarianceMatrices')
    DeleteWorkspace(output_workspace + '_Parameters')

    fit_group = output_workspace + '_Workspaces'
    params_table = output_workspace + '_Parameters'
    RenameWorkspace(output_workspace, OutputWorkspace=params_table)

    # Create *_Result workspace
    result_workspace = output_workspace + "_Result"
    parameter_names = ['A0', 'Intensity', 'Tau', 'Beta']
    convertParametersToWorkspace(params_table, "axis-1", parameter_names, result_workspace)

    # Set x units to be momentum transfer
    axis = mtd[result_workspace].getAxis(0)
    axis.setUnit("MomentumTransfer")

    # Process generated workspaces
    wsnames = mtd[fit_group].getNames()
    for i, workspace in enumerate(wsnames):
        output_ws = output_workspace + '_%d_Workspace' % i
        RenameWorkspace(workspace, OutputWorkspace=output_ws)

    sample_logs  = {'start_x': startx, 'end_x': endx, 'fit_type': fit_type,
                    'intensities_constrained': intensities_constrained, 'beta_constrained': False}

    CopyLogs(InputWorkspace=inputWS, OutputWorkspace=fit_group)
    CopyLogs(InputWorkspace=inputWS, OutputWorkspace=result_workspace)

    log_names = [item[0] for item in sample_logs]
    log_values = [item[1] for item in sample_logs]
    AddSampleLogMultiple(Workspace=result_workspace, LogNames=log_names, LogValues=log_values)
    AddSampleLogMultiple(Workspace=fit_group, LogNames=log_names, LogValues=log_values)

    if Save:
        save_workspaces = [result_workspace, fit_group]
        furyFitSaveWorkspaces(save_workspaces)

    if Plot != 'None' :
        furyfitPlotSeq(result_workspace, Plot)

    EndTime('FuryFit')
    return result_workspace


def furyfitMult(inputWS, function, ftype, startx, endx,
                spec_min=0, spec_max=None, intensities_constrained=False,
                minimizer='Levenberg-Marquardt', max_iterations=500,
                Save=False, Plot='None'):
    StartTime('FuryFit Multi')

    nHist = mtd[inputWS].getNumberHistograms()
    output_workspace = getWSprefix(inputWS) + 'fury_1Smult_s0_to_' + str(nHist-1)

    option = ftype[:-2]
    logger.information('Option: '+option)
    logger.information('Function: '+function)

    #prepare input workspace for fitting
    tmp_fit_workspace = "__furyfit_fit_ws"
    if spec_max is None:
        CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=tmp_fit_workspace,
                      XMin=startx, XMax=endx,
                      StartWorkspaceIndex=spec_min)
    else:
        CropWorkspace(InputWorkspace=inputWS, OutputWorkspace=tmp_fit_workspace,
                      XMin=startx, XMax=endx,
                      StartWorkspaceIndex=spec_min, EndWorkspaceIndex=spec_max)

    ConvertToHistogram(tmp_fit_workspace, OutputWorkspace=tmp_fit_workspace)
    convertToElasticQ(tmp_fit_workspace)

    #fit multi-domian functino to workspace
    multi_domain_func, kwargs = createFuryMultiDomainFunction(function, tmp_fit_workspace)
    Fit(Function=multi_domain_func,
        InputWorkspace=tmp_fit_workspace,
        WorkspaceIndex=0,
        Output=output_workspace,
        CreateOutput=True,
        Minimizer=minimizer,
        MaxIterations=max_iterations,
        **kwargs)

    params_table = output_workspace + '_Parameters'
    transposeFitParametersTable(params_table)

    #set first column of parameter table to be axis values
    x_axis = mtd[tmp_fit_workspace].getAxis(1)
    axis_values = x_axis.extractValues()
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

    log_names = [item[0] for item in sample_logs]
    log_values = [item[1] for item in sample_logs]
    AddSampleLogMultiple(Workspace=result_workspace, LogNames=log_names, LogValues=log_values)
    AddSampleLogMultiple(Workspace=fit_group, LogNames=log_names, LogValues=log_values)

    DeleteWorkspace(tmp_fit_workspace)

    if Save:
        save_workspaces = [result_workspace]
        furyFitSaveWorkspaces(save_workspaces)

    if Plot != 'None':
        furyfitPlotSeq(result_workspace, Plot)

    EndTime('TransformToIqtFit Multi')
    return result_workspace


def createFuryMultiDomainFunction(function, input_ws):
    multi= 'composite=MultiDomainFunction,NumDeriv=true;'
    comp = '(composite=CompositeFunction,NumDeriv=true,$domains=i;' + function + ');'

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
    for workspace in save_workspaces:
        #save workspace to default directory
        fpath = os.path.join(workdir, workspace+'.nxs')
        SaveNexusProcessed(InputWorkspace=workspace, Filename=fpath)
        logger.information(workspace + ' output to file : '+fpath)


def furyfitPlotSeq(ws, plot):
    if plot == 'All':
        param_names = ['Intensity', 'Tau', 'Beta']
    else:
        param_names = [plot]

    plotParameters(ws, *param_names)

