#pylint: disable=invalid-name
from mantid.api import *
from mantid import mtd, logger

import os

#-------------------------------------------------------------------------------

def load_files(data_files, ipf_filename, spec_min, spec_max, sum_files):
    """
    Loads a set of files and extracts just the spectra we care about (i.e. detector range and monitor).

    @param data_files List of data file names
    @param ipf_filename FIle path/name for the instrument parameter file to load
    @param spec_min Minimum spectra ID to load
    @param spec_max Maximum spectra ID to load
    @param sum Sum loaded files

    @return List of loaded workspace names and flag indicating chopped data
    """
    from mantid.simpleapi import (Load, LoadParameterFile, ChopData,
                                  ExtractSingleSpectrum, CropWorkspace)

    workspace_names = []

    for filename in data_files:
        # The filename without path and extension will be the workspace name
        ws_name = os.path.splitext(os.path.basename(filename))[0]
        logger.debug('Loading file %s as workspace %s' % (filename, ws_name))

        Load(Filename=filename,
             OutputWorkspace=ws_name)

        # Load the instrument parameters
        LoadParameterFile(Workspace=ws_name,
                          Filename=ipf_filename)

        # Add the workspace to the list of workspaces
        workspace_names.append(ws_name)

        # Get the spectrum number for the monitor
        instrument = mtd[ws_name].getInstrument()
        monitor_index = int(instrument.getNumberParameter('Workflow.Monitor1-SpectrumNumber')[0])
        logger.debug('Workspace %s monitor 1 spectrum number :%d' % (ws_name, monitor_index))

        # Chop data if required
        try:
            chop_threshold = mtd[ws_name].getInstrument().getNumberParameter('Workflow.ChopDataIfGreaterThan')[0]
            x_max = mtd[ws_name].readX(0)[-1]
            chopped_data =  x_max > chop_threshold
        except IndexError:
            chopped_data = False
        logger.information('Workspace %s need data chop: %s' % (ws_name, str(chopped_data)))

        workspaces = [ws_name]
        if chopped_data:
            ChopData(InputWorkspace=ws_name,
                     OutputWorkspace=ws_name,
                     MonitorWorkspaceIndex=monitor_index,
                     IntegrationRangeLower=5000.0,
                     IntegrationRangeUpper=10000.0,
                     NChops=5)
            workspaces = mtd[ws_name].getNames()

        for chop_ws_name in workspaces:
            # Get the monitor spectrum
            monitor_ws_name = chop_ws_name + '_mon'
            ExtractSingleSpectrum(InputWorkspace=chop_ws_name,
                                  OutputWorkspace=monitor_ws_name,
                                  WorkspaceIndex=monitor_index)

            # Crop to the detectors required
            CropWorkspace(InputWorkspace=chop_ws_name, OutputWorkspace=chop_ws_name,
                          StartWorkspaceIndex=int(spec_min) - 1,
                          EndWorkspaceIndex=int(spec_max) - 1)

    logger.information('Loaded workspace names: %s' % (str(workspace_names)))
    logger.information('Chopped data: %s' % (str(chopped_data)))

    # Sum files if needed
    if sum_files:
        if chopped_data:
            workspace_names = sum_chopped_runs(workspace_names)
        else:
            workspace_names = sum_regular_runs(workspace_names)

    logger.information('Summed workspace names: %s' % (str(workspace_names)))

    return workspace_names, chopped_data

#-------------------------------------------------------------------------------

def sum_regular_runs(workspace_names):
    """
    Sum runs with single workspace data.

    @param workspace_names List of names of input workspaces
    @return List of names of workspaces
    """
    from mantid.simpleapi import (MergeRuns, Scale, AddSampleLog)

    # Use the first workspace name as the result of summation
    summed_detector_ws_name = workspace_names[0]
    summed_monitor_ws_name = workspace_names[0] + '_mon'

    # Get a list of the run numbers for the original data
    run_numbers = ','.join([str(mtd[ws_name].getRunNumber()) for ws_name in workspace_names])

    # Generate lists of the detector and monitor workspaces
    detector_workspaces = ','.join(workspace_names)
    monitor_workspaces = ','.join([ws_name + '_mon' for ws_name in workspace_names])

    # Merge the raw workspaces
    MergeRuns(InputWorkspaces=detector_workspaces,
              OutputWorkspace=summed_detector_ws_name)
    MergeRuns(InputWorkspaces=monitor_workspaces,
              OutputWorkspace=summed_monitor_ws_name)

    # Delete old workspaces
    for idx in range(1, len(workspace_names)):
        DeleteWorkspace(workspace_names[idx])
        DeleteWorkspace(workspace_names[idx] + '_mon')

    # Derive the scale factor based on number of merged workspaces
    scale_factor = 1.0 / len(workspace_names)
    logger.information('Scale factor for summed workspaces: %f' % scale_factor)

    # Scale the new detector and monitor workspaces
    Scale(InputWorkspace=summed_detector_ws_name,
          OutputWorkspace=summed_detector_ws_name,
          Factor=scale_factor)
    Scale(InputWorkspace=summed_monitor_ws_name,
          OutputWorkspace=summed_monitor_ws_name,
          Factor=scale_factor)

    # Add the list of run numbers to the result workspace as a sample log
    AddSampleLog(Workspace=summed_detector_ws_name, LogName='multi_run_numbers',
                 LogType='String', LogText=run_numbers)

    # Only have the one workspace now
    return [summed_detector_ws_name]

#-------------------------------------------------------------------------------

def sum_chopped_runs(workspace_names):
    """
    Sum runs with chopped data.
    """
    from mantid.simpleapi import (MergeRuns, Scale, DeleteWorkspace)

    try:
        num_merges = len(mtd[workspace_names[0]].getNames())
    except:
        raise RuntimeError('Not all runs have been chopped, cannot sum.')

    merges = list()

    # Generate a list of workspaces to be merged
    for idx in range(0, num_merges):
        merges.append({'detector':list(), 'monitor':list()})

        for ws_name in workspace_names:
            detector_ws_name = mtd[ws_name].getNames()[idx]
            monitor_ws_name = detector_ws_name + '_mon'

            merges[idx]['detector'].append(detector_ws_name)
            merges[idx]['monitor'].append(monitor_ws_name)

    for merge in merges:
        # Merge the chopped run segments
        MergeRuns(InputWorkspaces=','.join(merge['detector']),
                  OutputWorkspace=merge['detector'][0])
        MergeRuns(InputWorkspaces=','.join(merge['monitor']),
                  OutputWorkspace=merge['monitor'][0])

        # Scale the merged runs
        merge_size = len(merge['detector'])
        factor = 1.0 / merge_size
        Scale(InputWorkspace=merge['detector'][0],
              OutputWorkspace=merge['detector'][0],
              Factor=factor,
              Operation='Multiply')
        Scale(InputWorkspace=merge['monitor'][0],
              OutputWorkspace=merge['monitor'][0],
              Factor=factor,
              Operation='Multiply')

        # Remove the old workspaces
        for idx in range(1, merge_size):
            DeleteWorkspace(merge['detector'][idx])
            DeleteWorkspace(merge['monitor'][idx])

    # Only have the one workspace now
    return [workspace_names[0]]

#-------------------------------------------------------------------------------

def identify_bad_detectors(workspace_name):
    """
    Identify detectors which should be masked

    @param workspace_name Name of worksapce to use ot get masking detectors
    @return List of masked spectra
    """
    from mantid.simpleapi import (IdentifyNoisyDetectors, DeleteWorkspace)

    instrument = mtd[workspace_name].getInstrument()

    try:
        masking_type = instrument.getStringParameter('Workflow.Masking')[0]
    except IndexError:
        masking_type = 'None'

    logger.information('Masking type: %s' % (masking_type))

    masked_spec = list()

    if masking_type == 'IdentifyNoisyDetectors':
        ws_mask = '__workspace_mask'
        IdentifyNoisyDetectors(InputWorkspace=workspace_name,
                               OutputWorkspace=ws_mask)

        # Convert workspace to a list of spectra
        num_spec = mtd[ws_mask].getNumberHistograms()
        masked_spec = [spec for spec in range(0, num_spec) if mtd[ws_mask].readY(spec)[0] == 0.0]

        # Remove the temporary masking workspace
        DeleteWorkspace(ws_mask)

    logger.debug('Masked specta for workspace %s: %s' % (workspace_name, str(masked_spec)))

    return masked_spec

#-------------------------------------------------------------------------------

def unwrap_monitor(workspace_name):
    """
    Unwrap monitor if required based on value of Workflow.UnwrapMonitor parameter

    @param workspace_name Name of workspace
    @return True if the monitor was unwrapped
    """
    from mantid.simpleapi import (UnwrapMonitor, RemoveBins, FFTSmooth)

    monitor_workspace_name = workspace_name + '_mon'
    instrument = mtd[monitor_workspace_name].getInstrument()

    # Determine if the monitor should be unwrapped
    try:
        unwrap = instrument.getStringParameter('Workflow.UnwrapMonitor')[0]

        if unwrap == 'Always':
            should_unwrap = True
        elif unwrap == 'BaseOnTimeRegime':
            mon_time = mtd[monitor_workspace_name].readX(0)[0]
            det_time = mtd[workspace_name].readX(0)[0]
            logger.notice(str(mon_time) + " " + str(det_time))
            should_unwrap = mon_time == det_time
        else:
            should_unwrap = False

    except IndexError:
        should_unwrap = False

    logger.debug('Need to unwrap monitor for %s: %s' % (workspace_name, str(should_unwrap)))

    if should_unwrap:
        sample = instrument.getSample()
        sample_to_source = sample.getPos() - instrument.getSource().getPos()
        radius = mtd[workspace_name].getDetector(0).getDistance(sample)
        z_dist = sample_to_source.getZ()
        l_ref = z_dist + radius

        logger.debug('For workspace %s: radius=%d, z_dist=%d, l_ref=%d' %
                     (workspace_name, radius, z_dist, l_ref))

        _, join = UnwrapMonitor(InputWorkspace=monitor_workspace_name,
                                OutputWorkspace=monitor_workspace_name,
                                LRef=l_ref)

        RemoveBins(InputWorkspace=monitor_workspace_name,
                   OutputWorkspace=monitor_workspace_name,
                   XMin=join - 0.001, XMax=join + 0.001,
                   Interpolation='Linear')

        try:
            FFTSmooth(InputWorkspace=monitor_workspace_name,
                      OutputWorkspace=monitor_workspace_name,
                      WorkspaceIndex=0)
        except ValueError:
            raise ValueError('Uneven bin widths are not supported.')

    return should_unwrap

#-------------------------------------------------------------------------------

def process_monitor_efficiency(workspace_name):
    """
    Process monitor efficiency for a given workspace.

    @param workspace_name Name of workspace to process monitor for
    """
    from mantid.simpleapi import OneMinusExponentialCor

    monitor_workspace_name = workspace_name + '_mon'
    instrument = mtd[workspace_name].getInstrument()

    try:
        area = instrument.getNumberParameter('Workflow.Monitor1-Area')[0]
        thickness = instrument.getNumberParameter('Workflow.Monitor1-Thickness')[0]
        attenuation = instrument.getNumberParameter('Workflow.Monitor1-Attenuation')[0]
    except IndexError:
        raise ValueError('Cannot get monitor details form parameter file')

    if area == -1 or thickness == -1 or attenuation == -1:
        logger.information('For workspace %s, skipping monitor efficiency' % (workspace_name))
        return

    OneMinusExponentialCor(InputWorkspace=monitor_workspace_name,
                           OutputWorkspace=monitor_workspace_name,
                           C=attenuation * thickness,
                           C1=area)

#-------------------------------------------------------------------------------

def scale_monitor(workspace_name):
    """
    Scale monitor intensity by a factor given as the Workflow.MonitorScalingFactor parameter.

    @param workspace_name Name of workspace to process monitor for
    """
    from mantid.simpleapi import Scale

    monitor_workspace_name = workspace_name + '_mon'
    instrument = mtd[workspace_name].getInstrument()

    try:
        scale_factor = instrument.getNumberParameter('Workflow.Monitor1-ScalingFactor')[0]
    except IndexError:
        logger.information('No monitor scaling factor found for workspace %s' % workspace_name)
        return

    if scale_factor != 1.0:
        Scale(InputWorkspace=monitor_workspace_name,
              OutputWorkspace=monitor_workspace_name,
              Factor=1.0 / scale_factor,
              Operation='Multiply')

#-------------------------------------------------------------------------------
