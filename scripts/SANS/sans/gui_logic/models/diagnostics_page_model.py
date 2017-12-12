from sans.common.general_functions import parse_diagnostic_settings
from sans.algorithm_detail.load_data import load_isis, get_loader_strategy
from sans.common.enums import (SANSFacility, SANSInstrument, SANSDataType, DetectorType)
from sans.common.file_information import (SANSFileInformationFactory, FileType, get_extension_for_file_type,
                                          find_full_file_path)
from sans.state.data import (StateData)
from mantid.simpleapi import SumRowColumn, SumSpectra
from sans.common.general_functions import (create_child_algorithm, append_to_sans_file_tag)
from sans.common.constants import EMPTY_NAME
from sans.common.enums import IntegralEnum, DetectorType
from mantid.api import AlgorithmPropertyWithValue

try:
    import mantidplot
except (Exception, Warning):
    mantidplot = None


def run_integral(file, period, range, mask, integral, detector):
    ranges = parse_range(range)
    import pydevd
    pydevd.settrace('localhost', port=5434, stdoutToServer=True, stderrToServer=True)
    input_workspace = load_workspace(file, period)
    input_workspace = crop_workspace(DetectorType.to_string(detector), input_workspace)

    if mask:
        input_workspace = apply_mask(input_workspace)

    output_workspaces = run_algorithm(input_workspace, ranges, integral)
    output_graph = plot_graph(output_workspaces)


def parse_range(range):
    if range:
        return parse_diagnostic_settings(range)
    else:
        return [[AlgorithmPropertyWithValue.EMPTY_INT, AlgorithmPropertyWithValue.EMPTY_INT]]

def load_workspace(file, period):
    if not period:
        period = StateData.ALL_PERIODS
    file_information_factory = SANSFileInformationFactory()
    file_information = file_information_factory.create_sans_file_information(file)

    workspace, workspace_monitor = load_isis(SANSDataType.SampleScatter, file_information, period, True, '', '')
    return workspace[SANSDataType.SampleScatter][0]

def crop_workspace(component, workspace):
    crop_name = "SANSCrop"
    crop_options = {"InputWorkspace": workspace,
                    "OutputWorkspace": EMPTY_NAME,
                    "Component": component}
    crop_alg = create_child_algorithm('', crop_name, **crop_options)
    crop_alg.execute()
    return crop_alg.getProperty("OutputWorkspace").value

def apply_mask(input_workspace):
    pass

def run_algorithm(input_workspace, ranges, integral):
    output_workspaces = []
    for range in ranges:
        output_workspace = generate_output_workspace_name(input_workspace, range, integral)
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

def generate_output_workspace_name(input_workspace, range, integral):
    return 'w{}r{}i{}'.format(input_workspace, range, integral)

def plot_graph(workspace):
    if mantidplot:
        return mantidplot.plotSpectrum(workspace, 0)
    else:
        return None