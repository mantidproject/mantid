#pylint: disable=invalid-name,too-many-locals,too-many-arguments

from IndirectImport import import_mantidplot
MTD_PLOT = import_mantidplot()
from IndirectCommon import *

import math, re, os.path, numpy as np
from mantid.simpleapi import *
from mantid.api import TextAxis
from mantid import *


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

