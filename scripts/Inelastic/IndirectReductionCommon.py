# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import AppendSpectra, ApplyDiffCal, ConvertUnits, CreateGroupingWorkspace, DeleteWorkspace, Load, Rebin
from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup, AlgorithmManager
from mantid.dataobjects import GroupingWorkspace
from mantid import mtd, logger, config

try:
    from mantid.simpleapi import plotSpectrum
except ImportError:
    logger.warning("plotSpectrum not available")
    plotSpectrum = None

import os
import numpy as np
from math import expm1, log
from typing import List, Tuple


# -------------------------------------------------------------------------------


def create_range_from(range_str, delimiter):
    """
    Creates a range from the specified string, by splitting by the specified
    delimiter.

    :param range_str:   The range string, in the format A-B where A is the lower
                        bound of the range, - is the delimiter and B is the upper
                        bound of the range.
    :param delimiter:   The range delimiter.
    :return:            The range created from the range string.
    """
    lower, upper = range_str.split(delimiter, 1)
    return range(int(lower), int(upper) + 1)


def create_file_range_parser(instrument):
    """
    Creates a parser which takes a specified file range string of the
    format A-B, and returns a list of the files in that range preceded
    by the specified instrument name.

    :param instrument:  The instrument name.
    :return:            A file range parser.
    """

    def parser(file_range):
        file_range = file_range.strip()
        # Check whether this is a range or single file
        try:
            if "-" in file_range:
                return [[instrument + str(run) for run in create_range_from(file_range, "-")]]
            elif ":" in file_range:
                return [[instrument + str(run)] for run in create_range_from(file_range, ":")]
            elif "+" in file_range:
                return [[instrument + run for run in file_range.split("+")]]
            else:
                return [[instrument + str(int(file_range))]]
        except ValueError:
            return [[file_range]]

    return parser


def load_file_ranges(file_ranges, ipf_filename, spec_min, spec_max, sum_files=True, load_logs=True, load_opts=None):
    """
    Loads a set of files from specified file ranges and extracts just the spectra we
    care about (i.e. detector range and monitor).

    @param file_ranges List of data file ranges
    @param ipf_filename File path/name for the instrument parameter file to load
    @param spec_min Minimum spectra ID to load
    @param spec_max Maximum spectra ID to load
    @param sum_files Sum loaded files
    @param load_logs Load log files when loading runs
    @param load_opts Additional options to be passed to load algorithm

    @return List of loaded workspace names and flag indicating chopped data
    """

    instrument = os.path.splitext(os.path.basename(ipf_filename))[0]
    instrument = instrument.split("_")[0]
    parse_file_range = create_file_range_parser(instrument)
    file_ranges = [file_range for range_str in file_ranges for file_range in range_str.split(",")]
    file_groups = [file_group for file_range in file_ranges for file_group in parse_file_range(file_range)]
    file_groups = flatten_groups(file_groups)

    workspace_names = []
    chopped_data = False

    for file_group in file_groups:
        created_workspaces, chopped_data, _ = load_files(file_group, ipf_filename, spec_min, spec_max, sum_files, load_logs, load_opts)
        workspace_names.extend(created_workspaces)

    return workspace_names, chopped_data


def flatten_groups(file_groups):
    """
    If the list of groups to reduce is a list of list with one element per list, it can miss the sum file algorithm unless the list
    is flattened to a single group

    @param file_groups list of groups to reduce separately

    @return Individual list with one group or list of lists if the groups have more than one member
    """
    sum_individual_workspaces = sum([len(group) for group in file_groups])
    if len(file_groups) != sum_individual_workspaces:
        return file_groups

    file_groups = [ws[0] for ws in file_groups]
    return [file_groups]


def load_files(data_files, ipf_filename, spec_min, spec_max, sum_files=False, load_logs=True, load_opts=None, find_masked_detectors=False):
    """
    Loads a set of files and extracts just the spectra we care about (i.e. detector range and monitor).

    @param data_files List of data file names
    @param ipf_filename File path/name for the instrument parameter file to load
    @param spec_min Minimum spectra ID to load
    @param spec_max Maximum spectra ID to load
    @param sum_files Sum loaded files
    @param load_logs Load log files when loading runs
    @param load_opts Additional options to be passed to load algorithm
    @param find_masked_detectors True if you want to find the masked detectors for the data files (summed)

    @return List of loaded workspace names and flag indicating chopped data
    """
    workspace_names, chopped_data = _load_files(data_files, ipf_filename, spec_min, spec_max, load_logs, load_opts)

    masked_detectors = get_detectors_to_mask_from_groups(workspace_names) if find_masked_detectors else None

    # Sum files if needed
    if sum_files and len(data_files) > 1:
        if chopped_data:
            workspace_names = sum_chopped_runs(workspace_names)
        else:
            workspace_names = sum_regular_runs(workspace_names)

    logger.information("Summed workspace names: %s" % (str(workspace_names)))

    return workspace_names, chopped_data, masked_detectors


def _load_files(file_specifiers, ipf_filename, spec_min, spec_max, load_logs=True, load_opts=None):
    """
    Loads a set of files and extracts just the spectra we care about (i.e. detector range and monitor).

    @param file_specifiers List of data file specifiers
    @param ipf_filename File path/name for the instrument parameter file to load
    @param spec_min Minimum spectra ID to load
    @param spec_max Maximum spectra ID to load
    @param load_logs Load log files when loading runs
    @param load_opts Additional options to be passed to load algorithm

    @return List of loaded workspace names and flag indicating chopped data
    """
    delete_monitors = False

    if load_opts is None:
        load_opts = {}

    if "DeleteMonitors" in load_opts:
        delete_monitors = load_opts["DeleteMonitors"]
        load_opts.pop("DeleteMonitors")

    workspace_names = []
    chopped_data = False

    for file_specifier in file_specifiers:
        # The filename without path and extension will be the workspace name
        ws_name = os.path.splitext(os.path.basename(str(file_specifier)))[0]
        logger.debug("Loading file %s as workspace %s" % (file_specifier, ws_name))
        do_load(file_specifier, ws_name, ipf_filename, load_logs, load_opts)
        workspace = mtd[ws_name]

        # Add the workspace to the list of workspaces
        workspace_names.append(ws_name)

        # Get the spectrum number for the monitor
        instrument = workspace.getInstrument()
        monitor_param = instrument.getNumberParameter("Workflow.Monitor1-SpectrumNumber")

        if monitor_param:
            monitor_index = int(monitor_param[0])
            logger.debug("Workspace %s monitor 1 spectrum number :%d" % (ws_name, monitor_index))

            workspaces, chopped_data = chop_workspace(workspace, monitor_index)
            crop_workspaces(workspaces, spec_min, spec_max, not delete_monitors, monitor_index)

    logger.information("Loaded workspace names: %s" % (str(workspace_names)))
    logger.information("Chopped data: %s" % (str(chopped_data)))

    if delete_monitors:
        load_opts["DeleteMonitors"] = True

    return workspace_names, chopped_data


# -------------------------------------------------------------------------------


def do_load(file_specifier, output_ws_name, ipf_filename, load_logs, load_opts):
    """
    Loads the files, passing the given file specifier in the load command.

    :param file_specifier:  The file specifier (single file, range or sum)
    :param output_ws_name:  The name of the output workspace to create
    :param ipf_filename:    The instrument parameter file to load with
    :param load_opts:       Additional loading options
    :param load_logs:       If True, load logs
    """
    from mantid.simpleapi import LoadVesuvio, LoadParameterFile

    if "VESUVIO" in ipf_filename:
        # Load all spectra. They are cropped later
        LoadVesuvio(Filename=str(file_specifier), OutputWorkspace=output_ws_name, SpectrumList="1-198", **load_opts)
    else:
        Load(Filename=file_specifier, OutputWorkspace=output_ws_name, LoadLogFiles=load_logs, **load_opts)

    # Load the instrument parameters
    LoadParameterFile(Workspace=output_ws_name, Filename=ipf_filename)


# -------------------------------------------------------------------------------


def chop_workspace(workspace, monitor_index):
    """
    Chops the specified workspace if its maximum x-value exceeds its instrument
    parameter, 'Workflow.ChopDataIfGreaterThan'.

    :param workspace:     The workspace to chop
    :param monitor_index: The index of the monitor spectra in the workspace.
    :return:              A tuple of the list of output workspace names and a boolean
                          specifying whether the workspace was chopped.
    """
    from mantid.simpleapi import ChopData

    workspace_name = workspace.name()

    # Chop data if required
    try:
        chop_threshold = workspace.getInstrument().getNumberParameter("Workflow.ChopDataIfGreaterThan")[0]
        x_max = workspace.readX(0)[-1]
        chopped_data = x_max > chop_threshold
    except IndexError:
        logger.warning("Chop threshold not found in instrument parameters")
        chopped_data = False
    logger.information("Workspace {0} need data chop: {1}".format(workspace_name, str(chopped_data)))

    if chopped_data:
        ChopData(
            InputWorkspace=workspace,
            OutputWorkspace=workspace_name,
            MonitorWorkspaceIndex=monitor_index,
            IntegrationRangeLower=5000.0,
            IntegrationRangeUpper=10000.0,
            NChops=5,
        )
        return mtd[workspace_name].getNames(), True
    else:
        return [workspace_name], False


# -------------------------------------------------------------------------------


def crop_workspaces(workspace_names, spec_min, spec_max, extract_monitors=True, monitor_index=0):
    """
    Crops the workspaces with the specified workspace names, from the specified minimum
    spectra to the specified maximum spectra.

    :param workspace_names:     The names of the workspaces to crop
    :param spec_min:            The minimum spectra of the cropping region
    :param spec_max:            The maximum spectra of the cropping region
    :param extract_monitors:    If True, extracts monitors from the workspaces
    :param monitor_index:       The index of the monitors in the workspaces
    """
    from mantid.simpleapi import ExtractSingleSpectrum, CropWorkspace

    for workspace_name in workspace_names:
        if extract_monitors:
            # Get the monitor spectrum
            monitor_ws_name = workspace_name + "_mon"
            ExtractSingleSpectrum(InputWorkspace=workspace_name, OutputWorkspace=monitor_ws_name, WorkspaceIndex=monitor_index)

        # Crop to the detectors required
        workspace = mtd[workspace_name]

        try:
            CropWorkspace(
                InputWorkspace=workspace_name,
                OutputWorkspace=workspace_name,
                StartWorkspaceIndex=workspace.getIndexFromSpectrumNumber(int(spec_min)),
                EndWorkspaceIndex=workspace.getIndexFromSpectrumNumber(int(spec_max)),
            )
        except RuntimeError:
            raise RuntimeError(
                "The spectra min {0} or spectra max {1} could not be found in workspace {2}.".format(
                    str(spec_min), str(spec_max), workspace_name
                )
            )


# -------------------------------------------------------------------------------


def sum_regular_runs(workspace_names):
    """
    Sum runs with single workspace data.

    @param workspace_names List of names of input workspaces
    @return List of names of workspaces
    """
    from mantid.simpleapi import MergeRuns, Scale, AddSampleLog, DeleteWorkspace

    # Use the first workspace name as the result of summation
    summed_detector_ws_name = workspace_names[0]
    summed_monitor_ws_name = workspace_names[0] + "_mon"

    # Get a list of the run numbers for the original data
    run_numbers = add_run_numbers(workspace_names)

    # Generate lists of the detector and monitor workspaces
    detector_workspaces = ",".join(workspace_names)
    monitor_workspaces = ",".join([ws_name + "_mon" for ws_name in workspace_names])

    # Merge the raw workspaces
    MergeRuns(InputWorkspaces=detector_workspaces, OutputWorkspace=summed_detector_ws_name)
    MergeRuns(InputWorkspaces=monitor_workspaces, OutputWorkspace=summed_monitor_ws_name)

    # Delete old workspaces
    for idx in range(1, len(workspace_names)):
        DeleteWorkspace(workspace_names[idx])
        DeleteWorkspace(workspace_names[idx] + "_mon")

    # Derive the scale factor based on number of merged workspaces
    scale_factor = 1.0 / len(workspace_names)
    logger.information("Scale factor for summed workspaces: %f" % scale_factor)

    # Scale the new detector and monitor workspaces
    Scale(InputWorkspace=summed_detector_ws_name, OutputWorkspace=summed_detector_ws_name, Factor=scale_factor)
    Scale(InputWorkspace=summed_monitor_ws_name, OutputWorkspace=summed_monitor_ws_name, Factor=scale_factor)

    # Add the list of run numbers to the result workspace as a sample log
    AddSampleLog(Workspace=workspace_names[0], LogName="multi_run_reduction", LogType="String", LogText="regular_runs")
    AddSampleLog(Workspace=workspace_names[0], LogName="run_number", LogType="String", LogText=run_numbers)

    # Only have the one workspace now
    return [summed_detector_ws_name]


# -------------------------------------------------------------------------------


def sum_chopped_runs(workspace_names):
    """
    Sum runs with chopped data.
    """
    from mantid.simpleapi import MergeRuns, Scale, DeleteWorkspace, AddSampleLog

    try:
        num_merges = len(mtd[workspace_names[0]].getNames())
    except:
        raise RuntimeError("Not all runs have been chopped, cannot sum.")

    merges = list()

    run_numbers = add_run_numbers(workspace_names)

    # Generate a list of workspaces to be merged
    for idx in range(0, num_merges):
        merges.append({"detector": list(), "monitor": list()})

        for ws_name in workspace_names:
            detector_ws_name = mtd[ws_name].getNames()[idx]
            monitor_ws_name = detector_ws_name + "_mon"

            merges[idx]["detector"].append(detector_ws_name)
            merges[idx]["monitor"].append(monitor_ws_name)

    for merge in merges:
        # Merge the chopped run segments
        MergeRuns(InputWorkspaces=",".join(merge["detector"]), OutputWorkspace=merge["detector"][0])
        MergeRuns(InputWorkspaces=",".join(merge["monitor"]), OutputWorkspace=merge["monitor"][0])

        # Scale the merged runs
        merge_size = len(merge["detector"])
        factor = 1.0 / merge_size
        Scale(InputWorkspace=merge["detector"][0], OutputWorkspace=merge["detector"][0], Factor=factor, Operation="Multiply")
        Scale(InputWorkspace=merge["monitor"][0], OutputWorkspace=merge["monitor"][0], Factor=factor, Operation="Multiply")

        # Remove the old workspaces
        for idx in range(1, merge_size):
            DeleteWorkspace(merge["detector"][idx])
            DeleteWorkspace(merge["monitor"][idx])

    # Add the list of run numbers to the result workspace as a sample log
    AddSampleLog(Workspace=workspace_names[0], LogName="multi_run_reduction", LogType="String", LogText="chopped_runs")
    AddSampleLog(Workspace=workspace_names[0], LogName="run_number", LogType="String", LogText=run_numbers)

    # Only have the one workspace now
    return [workspace_names[0]]


# --------------------------------------------------------------------------------
def add_run_numbers(workspace_names):
    run_numbers = []
    for ws_name in workspace_names:
        ws = mtd[ws_name]
        if ws.isGroup():
            number = ws[0].getRunNumber()
        else:
            number = ws.getRunNumber()
        run_numbers.append(number)

    # Get a list of the run numbers for the original data
    return ",".join([str(number) for number in run_numbers])


# ---------------------------------------------------------------------------------


def get_ipf_parameters_from_run(run_number, instrument, analyser, reflection, parameters):
    from IndirectCommon import getInstrumentParameter

    ipf_filename = os.path.join(
        config["instrumentDefinition.directory"], instrument + "_" + analyser + "_" + reflection + "_Parameters.xml"
    )

    results = dict()
    try:
        run_workspace = "__temp"
        do_load(instrument + str(run_number), run_workspace, ipf_filename, False, {})

        for parameter in parameters:
            results[parameter] = getInstrumentParameter(run_workspace, parameter)

        DeleteWorkspace(run_workspace)
    except ValueError or RuntimeError:
        pass

    return results


# -------------------------------------------------------------------------------


def add_workspace_names(workspace_names, workspace):
    if isinstance(workspace, WorkspaceGroup):
        workspace_names.extend(workspace.getNames())
    else:
        workspace_names.append(workspace.name())
    return workspace_names


def get_all_workspace_names(group_names):
    workspace_names = []
    for group_name in group_names:
        workspace_names = add_workspace_names(workspace_names, AnalysisDataService.retrieve(group_name))
    return workspace_names


def get_detectors_to_mask_from_groups(group_names):
    return get_detectors_to_mask(get_all_workspace_names(group_names))


def get_detectors_to_mask(workspace_names):
    masked_detectors = []
    for workspace_name in workspace_names:
        bad_detectors = [detector for detector in identify_bad_detectors(workspace_name) if detector not in masked_detectors]
        masked_detectors.extend(bad_detectors)

    return sorted(masked_detectors)


def identify_bad_detectors(workspace_name):
    """
    Identify detectors which should be masked

    @param workspace_name Name of workspace to use to get masking detectors
    @return List of spectra numbers to mask
    """
    from mantid.simpleapi import IdentifyNoisyDetectors

    instrument = mtd[workspace_name].getInstrument()

    try:
        masking_type = instrument.getStringParameter("Workflow.Masking")[0]
    except IndexError:
        masking_type = "None"

    logger.information("Masking type: %s" % masking_type)

    masked_spec = list()

    if masking_type == "IdentifyNoisyDetectors":
        workspace_mask = IdentifyNoisyDetectors(InputWorkspace=workspace_name, StoreInADS=False)

        # Convert workspace to a list of spectra
        num_spec = workspace_mask.getNumberHistograms()
        masked_spec = [workspace_mask.getSpectrum(i).getSpectrumNo() for i in range(0, num_spec) if workspace_mask.readY(i)[0] == 0.0]

    logger.debug("Masked spectra for workspace %s: %s" % (workspace_name, str(masked_spec)))

    return masked_spec


# -------------------------------------------------------------------------------


def unwrap_monitor(workspace_name):
    """
    Unwrap monitor if required based on value of Workflow.UnwrapMonitor parameter

    @param workspace_name Name of workspace
    @return True if the monitor was unwrapped
    """
    from mantid.simpleapi import UnwrapMonitor, RemoveBins, FFTSmooth

    monitor_workspace_name = workspace_name + "_mon"
    instrument = mtd[monitor_workspace_name].getInstrument()

    # Determine if the monitor should be unwrapped
    try:
        unwrap = instrument.getStringParameter("Workflow.UnwrapMonitor")[0]

        if unwrap == "Always":
            should_unwrap = True
        elif unwrap == "BaseOnTimeRegime":
            mon_time = mtd[monitor_workspace_name].readX(0)[0]
            det_time = mtd[workspace_name].readX(0)[0]
            should_unwrap = mon_time == det_time
        else:
            should_unwrap = False

    except IndexError:
        should_unwrap = False

    logger.debug("Need to unwrap monitor for %s: %s" % (workspace_name, str(should_unwrap)))

    if should_unwrap:
        sample = instrument.getSample()
        sample_to_source = sample.getPos() - instrument.getSource().getPos()
        radius = mtd[workspace_name].getDetector(0).getDistance(sample)
        z_dist = sample_to_source.getZ()
        l_ref = z_dist + radius

        logger.debug("For workspace %s: radius=%d, z_dist=%d, l_ref=%d" % (workspace_name, radius, z_dist, l_ref))

        _, join = UnwrapMonitor(InputWorkspace=monitor_workspace_name, OutputWorkspace=monitor_workspace_name, LRef=l_ref)

        RemoveBins(
            InputWorkspace=monitor_workspace_name,
            OutputWorkspace=monitor_workspace_name,
            XMin=join - 0.001,
            XMax=join + 0.001,
            Interpolation="Linear",
        )

        try:
            FFTSmooth(InputWorkspace=monitor_workspace_name, OutputWorkspace=monitor_workspace_name, WorkspaceIndex=0, IgnoreXBins=True)
        except ValueError:
            raise ValueError("Uneven bin widths are not supported.")

    return should_unwrap


# -------------------------------------------------------------------------------


def process_monitor_efficiency(workspace_name):
    """
    Process monitor efficiency for a given workspace.

    @param workspace_name Name of workspace to process monitor for
    """
    from mantid.simpleapi import OneMinusExponentialCor

    monitor_workspace_name = workspace_name + "_mon"
    instrument = mtd[workspace_name].getInstrument()

    try:
        area = instrument.getNumberParameter("Workflow.Monitor1-Area")[0]
        thickness = instrument.getNumberParameter("Workflow.Monitor1-Thickness")[0]
        attenuation = instrument.getNumberParameter("Workflow.Monitor1-Attenuation")[0]
    except IndexError:
        raise ValueError("Cannot get monitor details form parameter file")

    if area == -1 or thickness == -1 or attenuation == -1:
        logger.information("For workspace %s, skipping monitor efficiency" % workspace_name)
        return

    OneMinusExponentialCor(
        InputWorkspace=monitor_workspace_name, OutputWorkspace=monitor_workspace_name, C=attenuation * thickness, C1=area
    )


# -------------------------------------------------------------------------------


def scale_monitor(workspace_name):
    """
    Scale monitor intensity by a factor given as the Workflow.MonitorScalingFactor parameter.

    @param workspace_name Name of workspace to process monitor for
    """
    from mantid.simpleapi import Scale

    monitor_workspace_name = workspace_name + "_mon"
    instrument = mtd[workspace_name].getInstrument()

    try:
        scale_factor = instrument.getNumberParameter("Workflow.Monitor1-ScalingFactor")[0]
    except IndexError:
        logger.information("No monitor scaling factor found for workspace %s" % workspace_name)
        return

    if scale_factor != 1.0:
        Scale(
            InputWorkspace=monitor_workspace_name, OutputWorkspace=monitor_workspace_name, Factor=1.0 / scale_factor, Operation="Multiply"
        )


# -------------------------------------------------------------------------------


def scale_detectors(workspace_name, e_mode="Indirect"):
    """
    Scales detectors by monitor intensity.

    @param workspace_name Name of detector workspace
    @param e_mode Energy mode (Indirect for spectroscopy, Elastic for diffraction)
    """
    from mantid.simpleapi import ConvertUnits, RebinToWorkspace, Divide

    monitor_workspace_name = workspace_name + "_mon"

    ConvertUnits(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Target="Wavelength", EMode=e_mode)

    RebinToWorkspace(WorkspaceToRebin=workspace_name, WorkspaceToMatch=monitor_workspace_name, OutputWorkspace=workspace_name)

    Divide(LHSWorkspace=workspace_name, RHSWorkspace=monitor_workspace_name, OutputWorkspace=workspace_name)


# -------------------------------------------------------------------------------


def create_group_from_spectra_list(group_detectors, spectra_list):
    group_detectors.setProperty("SpectraList", spectra_list)
    group_detectors.setProperty("OutputWorkspace", "__temp")
    group_detectors.execute()
    return group_detectors.getProperty("OutputWorkspace").value


def create_individual_spectra_groups(group_detectors, grouping_string):
    return [create_group_from_spectra_list(group_detectors, [i]) for i in create_range_from(grouping_string, ":")]


def add_group_from_string(groups, group_detectors, grouping_string):
    if ":" in grouping_string:
        groups.extend(create_individual_spectra_groups(group_detectors, grouping_string))
    elif "-" in grouping_string:
        groups.append(create_group_from_spectra_list(group_detectors, list(create_range_from(grouping_string, "-"))))
    elif "+" in grouping_string:
        groups.append(create_group_from_spectra_list(group_detectors, [int(i) for i in grouping_string.split("+")]))
    else:
        groups.append(create_group_from_spectra_list(group_detectors, [int(grouping_string)]))


def conjoin_workspaces(*workspaces):
    conjoined = workspaces[0]
    for workspace in workspaces[1:]:
        conjoined = AppendSpectra(conjoined, workspace, StoreInADS=False)
    return conjoined


def group_on_string(group_detectors, grouping_string):
    grouping_string.replace(" ", "")
    groups = []
    for sub_string in grouping_string.split(","):
        add_group_from_string(groups, group_detectors, sub_string)
    return conjoin_workspaces(*groups)


def group_spectra(
    workspace: str | MatrixWorkspace,
    method: str,
    group_file: str = None,
    group_ws: GroupingWorkspace = None,
    group_string: str = None,
    number_of_groups: int = None,
    spectra_range: List[float] = None,
) -> MatrixWorkspace:
    """
    Groups spectra in a given workspace according to the Workflow.GroupingMethod property.

    @param workspace A workspace object, or the name of a workspace in the ADS
    @param method Grouping method (IPF, All, Individual, File, Workspace)
    @param group_file *.map file for GroupDetectors, or *.cal file for DiffractionFocussing
    @param group_ws Workspace for Workspace method
    @param group_string String for custom method - comma separated list or range
    @param number_of_groups The number of groups to split the spectra into
    @param spectra_range The min and max spectra numbers
    """
    return group_spectra_of(
        mtd[workspace] if isinstance(workspace, str) else workspace,
        method,
        group_file,
        group_ws,
        group_string,
        number_of_groups,
        spectra_range,
    )


def group_spectra_of(
    workspace: MatrixWorkspace,
    method: str,
    group_file: str = None,
    group_ws: GroupingWorkspace = None,
    group_string: str = None,
    number_of_groups: int = None,
    spectra_range: List[int] = None,
) -> MatrixWorkspace:
    """
    Groups spectra in a given workspace according to the Workflow.GroupingMethod property.

    @param workspace Workspace to group spectra of
    @param method Grouping method (IPF, All, Individual, File, Workspace)
    @param group_file *.map file for GroupDetectors, or *.cal file for DiffractionFocussing
    @param group_ws Workspace for Workspace method
    @param group_string String for custom method - comma separated list or range
    @param number_of_groups The number of groups to split the spectra into
    @param spectra_range The min and max spectra numbers
    """

    instrument = workspace.getInstrument()
    group_detectors = AlgorithmManager.create("GroupDetectors")
    group_detectors.setChild(True)
    group_detectors.setProperty("InputWorkspace", workspace)
    group_detectors.setProperty("Behaviour", "Average")

    # If grouping as per he IPF is desired
    if method == "IPF":
        # Get the grouping method from the parameter file
        try:
            grouping_method = instrument.getStringParameter("Workflow.GroupingMethod")[0]
        except IndexError:
            grouping_method = "Individual"

    else:
        # Otherwise use the value of GroupingMethod
        grouping_method = method

    logger.information("Grouping method for workspace %s is %s" % (workspace.name(), grouping_method))

    if grouping_method == "Individual":
        # Nothing to do here
        return workspace

    elif grouping_method == "All":
        # Get a list of all spectra minus those which are masked
        num_spec = workspace.getNumberHistograms()
        spectra_list = [spec for spec in range(0, num_spec)]

        # Apply the grouping
        group_detectors.setProperty("WorkspaceIndexList", spectra_list)

    elif grouping_method == "File":
        # Get the filename for the grouping file
        if group_file is not None:
            # If grouping file is a *.cal file
            if group_file.endswith(".cal"):
                group_ws = create_grouping_workspace(workspace, group_file)
                group_detectors.setProperty("CopyGroupingFromWorkspace", group_ws)
                group_detectors.execute()
                return group_detectors.getProperty("OutputWorkspace").value

            grouping_file = group_file
            group_detectors.setProperty("ExcludeGroupNumbers", [0])
        else:
            try:
                grouping_file = instrument.getStringParameter("Workflow.GroupingFile")[0]
            except IndexError:
                raise RuntimeError("Cannot get grouping file from properties or IPF.")

        # If the file is not found assume it is in the grouping files directory
        if not os.path.isfile(grouping_file):
            grouping_file = os.path.join(config.getString("groupingFiles.directory"), grouping_file)

        # If it is still not found just give up
        if not os.path.isfile(grouping_file):
            raise RuntimeError("Cannot find grouping file: %s" % grouping_file)

        # Apply the grouping
        group_detectors.setProperty("MapFile", grouping_file)

    elif grouping_method == "Workspace":
        # Apply the grouping
        group_detectors.setProperty("CopyGroupingFromWorkspace", group_ws)

    elif grouping_method == "Custom":
        return group_on_string(group_detectors, group_string)
    elif grouping_method == "Groups":
        group_string = create_detector_grouping_string(number_of_groups, spectra_range[0], spectra_range[1])
        return group_on_string(group_detectors, group_string)
    else:
        raise RuntimeError("Invalid grouping method %s for workspace %s" % (grouping_method, workspace.name()))

    group_detectors.execute()
    return group_detectors.getProperty("OutputWorkspace").value


def create_range_string(minimum: int, maximum: int) -> str:
    return f"{minimum}-{maximum}"


def create_grouping_string(group_size: int, number_of_groups: int, spectra_min: int) -> str:
    grouping_string = create_range_string(spectra_min, spectra_min + group_size - 1)
    for i in range(spectra_min + group_size, spectra_min + group_size * number_of_groups, group_size):
        grouping_string += "," + create_range_string(i, i + group_size - 1)
    return grouping_string


def create_detector_grouping_string(number_of_groups: int, spectra_min: int, spectra_max: int) -> str:
    assert number_of_groups > 0, "Number of groups must be greater than zero."
    assert spectra_min <= spectra_max, "Spectra min cannot be larger than spectra max."
    assert number_of_groups <= spectra_max - spectra_min + 1, "Number of groups must be less or equal to the number of spectra."
    number_of_spectra = 1 + spectra_max - spectra_min

    grouping_string = create_grouping_string(int(number_of_spectra / number_of_groups), number_of_groups, spectra_min)
    remainder = number_of_spectra % number_of_groups
    if remainder != 0:
        grouping_string += f",{create_range_string(spectra_min + number_of_spectra - remainder, spectra_min + number_of_spectra - 1)}"
    return grouping_string


def create_grouping_workspace(workspace: MatrixWorkspace, cal_file: str) -> GroupingWorkspace:
    group_ws, _, _ = CreateGroupingWorkspace(InstrumentName=workspace.getInstrument().getName(), OldCalFilename=cal_file, StoreInADS=False)
    return group_ws


def _excluded_detector_ids(grouping_workspace: GroupingWorkspace) -> List[int]:
    """
    Finds the detector IDs which are not included in the grouping. These detector IDs will be in a group with a negative group ID.
    @param grouping_workspace The GroupingWorkspace containing the detector grouping information.
    @return A list of detector IDs which are excluded from the grouping.
    """
    excluded_ids = []
    for group_id in grouping_workspace.getGroupIDs():
        if group_id < 0:
            excluded_ids.extend(grouping_workspace.getDetectorIDsOfGroup(int(group_id)))
    return excluded_ids


# -------------------------------------------------------------------------------


def fold_chopped(workspace_name):
    """
    Folds multiple frames of a data set into one workspace.

    @param workspace_name Name of the group to fold
    """
    from mantid.simpleapi import MergeRuns, DeleteWorkspace, CreateWorkspace, Divide

    workspaces = mtd[workspace_name].getNames()
    merged_ws = workspace_name + "_merged"
    MergeRuns(InputWorkspaces=",".join(workspaces), OutputWorkspace=merged_ws)

    scaling_ws = "__scaling_ws"
    unit = mtd[workspace_name].getItem(0).getAxis(0).getUnit().unitID()

    ranges = []
    for ws in mtd[workspace_name].getNames():
        x_min = mtd[ws].dataX(0)[0]
        x_max = mtd[ws].dataX(0)[-1]
        ranges.append((x_min, x_max))
        DeleteWorkspace(Workspace=ws)

    data_x = mtd[merged_ws].readX(0)
    data_y = []
    data_e = []

    for i in range(0, mtd[merged_ws].blocksize()):
        y_val = 0.0
        for rng in ranges:
            if rng[0] <= data_x[i] <= rng[1]:
                y_val += 1.0

        data_y.append(y_val)
        data_e.append(0.0)

    CreateWorkspace(OutputWorkspace=scaling_ws, DataX=data_x, DataY=data_y, DataE=data_e, UnitX=unit)

    Divide(LHSWorkspace=merged_ws, RHSWorkspace=scaling_ws, OutputWorkspace=workspace_name)

    DeleteWorkspace(Workspace=merged_ws)
    DeleteWorkspace(Workspace=scaling_ws)


# -------------------------------------------------------------------------------


def rename_reduction(workspace_name, multiple_files):
    """
    Renames a workspace according to the naming policy in the Workflow.NamingConvention parameter.

    @param workspace_name Name of workspace
    @param multiple_files Insert the multiple file marker
    @return New name of workspace
    """
    from mantid.simpleapi import RenameWorkspace
    import string

    is_multi_frame = isinstance(mtd[workspace_name], WorkspaceGroup)

    # Get the instrument, run number and title
    if is_multi_frame:
        instrument = mtd[workspace_name].getItem(0).getInstrument()
        run_number = mtd[workspace_name].getItem(0).getRun()["run_number"].value
        run_title = mtd[workspace_name].getItem(0).getRun()["run_title"].value.strip()
    else:
        instrument = mtd[workspace_name].getInstrument()
        run_number = mtd[workspace_name].getRun()["run_number"].value
        run_title = mtd[workspace_name].getRun()["run_title"].value.strip()

    # Get the naming convention parameter form the parameter file
    try:
        convention = instrument.getStringParameter("Workflow.NamingConvention")[0]
    except IndexError:
        # Default to run title if naming convention parameter not set
        convention = "RunTitle"
    logger.information("Naming convention for workspace %s is %s" % (workspace_name, convention))
    logger.information("Run number for workspace %s is %s" % (workspace_name, run_number))
    logger.information("Run title for workspace %s is %s" % (workspace_name, run_title))

    inst_name = instrument.getName()
    inst_name = inst_name.lower()

    if multiple_files:
        multi_run_marker = "_multi"
        split_runs = [int(run) for run in run_number.split(",")]
        run_number = str(min(split_runs)) + "-" + str(max(split_runs))
    else:
        multi_run_marker = ""

    if convention == "None":
        new_name = workspace_name

    elif convention == "RunTitle":
        valid = "-_.() %s%s" % (string.ascii_letters, string.digits)
        formatted_title = "".join([c for c in run_title if c in valid])
        new_name = "%s%s%s-%s" % (inst_name.lower(), run_number, multi_run_marker, formatted_title)

    elif convention == "AnalyserReflection":
        analyser = instrument.getStringParameter("analyser")[0]
        reflection = instrument.getStringParameter("reflection")[0]
        new_name = "%s%s%s_%s%s_red" % (inst_name.lower(), run_number, multi_run_marker, analyser, reflection)

    else:
        raise RuntimeError("No valid naming convention for workspace %s" % workspace_name)

    logger.information("New name for %s workspace: %s" % (workspace_name, new_name))

    RenameWorkspace(InputWorkspace=workspace_name, OutputWorkspace=new_name)

    return new_name


# -------------------------------------------------------------------------------


def plot_reduction(workspace_name, plot_type):
    """
    Plot a given workspace based on the Plot property.

    @param workspace_name Name of workspace to plot
    @param plot_type Type of plot to create
    """

    if plot_type == "Spectra" or plot_type == "Both":
        num_spectra = mtd[workspace_name].getNumberHistograms()
        if plotSpectrum is None:
            logger.error("plotSpectrum is not available. Please check your Mantid installation.")
            raise RuntimeError("plotSpectrum is not available. Please check your Mantid installation.")

        try:
            plotSpectrum(workspace_name, range(0, num_spectra))
        except RuntimeError:
            logger.notice("Spectrum plotting canceled by user")

    can_plot_contour = mtd[workspace_name].getNumberHistograms() > 1
    if (plot_type == "Contour" or plot_type == "Both") and can_plot_contour:
        from mantidqt.plotting.functions import pcolormesh

        pcolormesh(workspace_name)


# -------------------------------------------------------------------------------


def save_reduction(workspace_names, formats, x_units="DeltaE"):
    """
    Saves the workspaces to the default save directory.

    @param workspace_names List of workspace names to save
    @param formats List of formats to save in
    @param x_units X units
    """
    from mantid.simpleapi import (
        SaveSPE,
        SaveNexusProcessed,
        SaveNXSPE,
        SaveAscii,
        Rebin,
        DeleteWorkspace,
        ConvertSpectrumAxis,
        SaveDaveGrp,
    )

    for workspace_name in workspace_names:
        if "spe" in formats:
            SaveSPE(InputWorkspace=workspace_name, Filename=workspace_name + ".spe")

        if "nxs" in formats:
            SaveNexusProcessed(InputWorkspace=workspace_name, Filename=workspace_name + ".nxs")

        if "nxspe" in formats:
            SaveNXSPE(InputWorkspace=workspace_name, Filename=workspace_name + ".nxspe")

        if "ascii" in formats:
            _save_ascii(workspace_name, workspace_name + ".dat")

        if "aclimax" in formats:
            if x_units == "DeltaE_inWavenumber":
                bins = "24, -0.005, 4000"  # cm-1
            else:
                bins = "3, -0.005, 500"  # meV

            Rebin(InputWorkspace=workspace_name, OutputWorkspace=workspace_name + "_aclimax_save_temp", Params=bins)
            SaveAscii(InputWorkspace=workspace_name + "_aclimax_save_temp", Filename=workspace_name + "_aclimax.dat", Separator="Tab")
            DeleteWorkspace(Workspace=workspace_name + "_aclimax_save_temp")

        if "davegrp" in formats:
            ConvertSpectrumAxis(
                InputWorkspace=workspace_name, OutputWorkspace=workspace_name + "_davegrp_save_temp", Target="ElasticQ", EMode="Indirect"
            )
            SaveDaveGrp(InputWorkspace=workspace_name + "_davegrp_save_temp", Filename=workspace_name + ".grp")
            DeleteWorkspace(Workspace=workspace_name + "_davegrp_save_temp")


# -------------------------------------------------------------------------------


def get_multi_frame_rebin(workspace_name, rebin_string):
    """
    Creates a rebin string for rebinning multiple frames data.

    @param workspace_name Name of multiple frame workspace group
    @param rebin_string Original rebin string

    @return New rebin string
    @return Maximum number of bins in input workspaces
    """

    multi_frame = isinstance(mtd[workspace_name], WorkspaceGroup)

    if rebin_string is not None and multi_frame:
        rebin_string_comp = rebin_string.split(",")
        if len(rebin_string_comp) >= 5:
            rebin_string_2 = ",".join(rebin_string_comp[2:])
        else:
            rebin_string_2 = rebin_string

        bin_counts = [mtd[ws].blocksize() for ws in mtd[workspace_name].getNames()]
        num_bins = np.amax(bin_counts)

        return rebin_string_2, num_bins

    return None, None


# -------------------------------------------------------------------------------


def _get_x_range_when_bins_vary(workspace: MatrixWorkspace, grouping_workspace: GroupingWorkspace) -> Tuple[float]:
    """
    Finds the minimum and maximum X values for a workspace with varying bins. It will only search for the min
    and max x values in spectra which are included in the grouping provided by the GroupingWorkspace.
    @param workspace The workspace to search for the min and max X.
    @param grouping_workspace The workspace containing detector grouping information.
    @return A minimum and maximum X value.
    """
    excluded_ids = _excluded_detector_ids(grouping_workspace)

    min_value, max_value = float("inf"), float("-inf")
    spec_info = workspace.spectrumInfo()
    for i in range(workspace.getNumberHistograms()):
        if spec_info.isMasked(i) or not spec_info.hasDetectors(i):
            continue
        detector_ids = workspace.getSpectrum(i).getDetectorIDs()
        if detector_ids[0] in excluded_ids:
            continue
        x = workspace.extractX()[i]
        min_value = x.min() if x.min() < min_value else min_value
        max_value = x.max() if x.max() > max_value else max_value

    assert min_value > 0, "The minimum x value in a dSpacing workspace should be a positive number."
    assert max_value == float("-inf") or max_value > 0, "The maximum x value in a dSpacing workspace should be a positive number."

    return min_value, max_value


def rebin_logarithmic(workspace: MatrixWorkspace, calibration_file: str) -> MatrixWorkspace:
    """
    Performs logarithmic rebinning on the provided workspace. The rebinning parameters are calculated
    based on the spectra which are included in the grouping provided in the calibration file.
    @param workspace The workspace to be logarithmically rebinned.
    @param calibration_file The *.cal file to use to find which detectors are excluded from the grouping.
    @return A rebinned workspace.
    """
    grouping_workspace = create_grouping_workspace(workspace, calibration_file)

    min_value, max_value = _get_x_range_when_bins_vary(workspace, grouping_workspace)

    if min_value == float("inf") or max_value == float("-inf"):
        raise RuntimeError("No selected Detectors found in .cal file for input range.")

    step = expm1((log(max_value) - log(min_value)) / workspace.blocksize())

    return Rebin(InputWorkspace=workspace, Params=[min_value, step, max_value], BinningMode="Logarithmic", StoreInADS=False)


def rebin_reduction(workspace_name, rebin_string, multi_frame_rebin_string, num_bins):
    """
    @param workspace_name Name of workspace to rebin
    @param rebin_string Rebin parameters
    @param multi_frame_rebin_string Rebin string for multiple frame rebinning
    @param num_bins Max number of bins in input frames
    """
    from mantid.simpleapi import Rebin, SortXAxis, RemoveSpectra

    if rebin_string is not None:
        if multi_frame_rebin_string is not None and num_bins is not None:
            # Multi frame data
            if mtd[workspace_name].blocksize() == num_bins:
                Rebin(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Params=rebin_string)
            else:
                Rebin(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Params=multi_frame_rebin_string)
        else:
            # Regular data
            RemoveSpectra(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, RemoveSpectraWithNoDetector=True)
            SortXAxis(InputWorkspace=workspace_name, OutputWorkspace=workspace_name)
            Rebin(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Params=rebin_string)
    else:
        try:
            # If user does not want to rebin then just ensure uniform binning across spectra
            # extract the binning parameters from the first spectrum.
            # there is probably a better way to calculate the binning parameters, but this
            # gets the right answer.
            xaxis = mtd[workspace_name].readX(0)
            params = []
            for i, x in enumerate(xaxis):
                params.append(x)
                if i < len(xaxis) - 1:
                    params.append(xaxis[i + 1] - x)  # delta
            Rebin(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Params=params)
        except RuntimeError:
            logger.warning("Rebinning failed, will try to continue anyway.")


# -------------------------------------------------------------------------------


def calibrate(workspace, calibration_file: str):
    """
    Calibrates the workspace using the calibration file, and converts from TOF to dSpacing.

    @param workspace The workspace or workspace name to be calibrated.
    @param calibration_file The calibration file to use.

    @return The calibrated workspace.
    """
    ApplyDiffCal(InstrumentWorkspace=workspace, CalibrationFile=calibration_file, StoreInADS=False, EnableLogging=False)

    converted = ConvertUnits(
        InputWorkspace=workspace,
        Target="dSpacing",
        StoreInADS=False,
        EnableLogging=False,
    )

    ApplyDiffCal(InstrumentWorkspace=converted, ClearCalibration=True, StoreInADS=False, EnableLogging=False)
    return converted


# -------------------------------------------------------------------------------

# ========== Child Algorithms ==========


def mask_detectors(workspace, masked_indices):
    """
    Masks the detectors at the specified indices in the specified
    workspace.

    :param workspace:       The workspace whose detectors to mask.
    :param masked_indices:  The spectra indices to mask.
    """
    mask_detectors_alg = AlgorithmManager.createUnmanaged("MaskDetectors")
    mask_detectors_alg.setChild(True)
    mask_detectors_alg.initialize()
    mask_detectors_alg.setProperty("Workspace", workspace)
    mask_detectors_alg.setProperty("SpectraList", masked_indices)
    mask_detectors_alg.execute()


def _save_ascii(workspace, file_name):
    """
    Saves the specified workspace into a file with the specified name,
    in ASCII-format.

    :param workspace:   The workspace to save.
    :param file_name:   The name of the file to save the workspace into.
    """
    # Changed to version 2 to enable re-loading of files into mantid
    save_ascii_alg = AlgorithmManager.createUnmanaged("SaveAscii", 2)
    save_ascii_alg.setChild(True)
    save_ascii_alg.initialize()
    save_ascii_alg.setProperty("InputWorkspace", workspace)
    save_ascii_alg.setProperty("Filename", file_name + ".dat")
    save_ascii_alg.execute()
