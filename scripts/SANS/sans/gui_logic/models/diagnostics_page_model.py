from sans.common.general_functions import parse_diagnostic_settings
from sans.algorithm_detail.load_data import load_isis, get_loader_strategy
from sans.common.enums import (SANSFacility, SANSInstrument, SANSDataType, DetectorType)
from sans.common.file_information import (SANSFileInformationFactory, FileType, get_extension_for_file_type,
                                          find_full_file_path)
from sans.state.data import (StateData)
from mantid.simpleapi import SumRowColumn, SumSpectra
from sans.common.general_functions import (create_child_algorithm, append_to_sans_file_tag, create_managed_non_child_algorithm)
from sans.common.constants import EMPTY_NAME
from sans.common.enums import IntegralEnum, DetectorType
from mantid.api import AlgorithmPropertyWithValue
from sans.algorithm_detail.batch_execution import set_output_workspaces_on_load_algorithm, get_workspace_from_algorithm

try:
    import mantidplot
except (Exception, Warning):
    mantidplot = None


def run_integral(range, mask, integral, detector, state):
    ranges = parse_range(range)
    input_workspace = load_workspace(state)
    input_workspace_name = input_workspace.name()
    input_workspace = crop_workspace(DetectorType.to_string(detector), input_workspace)

    if mask:
        input_workspace = apply_mask(state, input_workspace, DetectorType.to_string(detector))

    output_workspaces = run_algorithm(input_workspace, ranges, integral, mask, detector, input_workspace_name)
    output_graph = plot_graph(output_workspaces)


def parse_range(range):
    if range:
        return parse_diagnostic_settings(range)
    else:
        return [[AlgorithmPropertyWithValue.EMPTY_INT, AlgorithmPropertyWithValue.EMPTY_INT]]

def load_workspace(state):
    use_optimizations = True
    # Load the data
    state_serialized = state.property_manager
    load_name = "SANSLoad"
    load_options = {"SANSState": state_serialized,
                    "PublishToCache": use_optimizations,
                    "UseCached": use_optimizations,
                    "MoveWorkspace": False}

    # Set the output workspaces
    set_output_workspaces_on_load_algorithm(load_options, state)

    load_alg = create_managed_non_child_algorithm(load_name, **load_options)
    load_alg.execute()

    return get_workspace_from_algorithm(load_alg, 'SampleScatterWorkspace')

def crop_workspace(component, workspace):
    crop_name = "SANSCrop"
    crop_options = {"InputWorkspace": workspace,
                    "OutputWorkspace": EMPTY_NAME,
                    "Component": component}
    crop_alg = create_child_algorithm('', crop_name, **crop_options)
    crop_alg.execute()
    return crop_alg.getProperty("OutputWorkspace").value

def run_algorithm(input_workspace, ranges, integral, mask, detector, input_workspace_name):
    output_workspaces = []
    for range in ranges:
        output_workspace = generate_output_workspace_name(range, integral, mask, detector, input_workspace_name)
        output_workspaces.append(output_workspace)

        hv_min = range[0]
        hv_max = range[1]

        if integral == IntegralEnum.Horizontal:
            SumRowColumn(InputWorkspace=input_workspace, OutputWorkspace=output_workspace, Orientation='D_H',
                         HOverVMin=hv_min, HOverVMax=hv_max)
        elif integral == IntegralEnum.Vertical:
            SumRowColumn(InputWorkspace=input_workspace, OutputWorkspace=output_workspace, Orientation='D_V',
                         HOverVMin=hv_min, HOverVMax=hv_max)
        elif integral == IntegralEnum.Time:
            SumSpectra(InputWorkspace=input_workspace, OutputWorkspace=output_workspace)

    return output_workspaces

def generate_output_workspace_name(range, integral, mask, detector, input_workspace_name):
    integral_string = IntegralEnum.to_string(integral)
    detector_string = DetectorType.to_string(detector)
    return 'Run:{}, Range:{}, Direction:{}, Detector:{}, Mask:{}'.format(input_workspace_name, range, integral_string, detector_string, mask)

def plot_graph(workspace):
    if mantidplot:
        return mantidplot.plotSpectrum(workspace, 0)
    else:
        return None

def apply_mask(state, workspace, component):
    state_serialized = state.property_manager
    mask_name = "SANSMaskWorkspace"
    mask_options = {"SANSState": state_serialized,
                    "Workspace": workspace,
                    "Component": component}
    mask_alg = create_child_algorithm('', mask_name, **mask_options)
    mask_alg.execute()
    return mask_alg.getProperty("Workspace").value