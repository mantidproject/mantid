# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CropToComponent, logger, mtd
from mantid.api import WorkspaceGroup, MatrixWorkspace
from enum import IntEnum
from math import fabs
from os import path
import re


class AcqMode(IntEnum):
    """
    |        |  MONO  |  KINETIC  |  TOF  |  REVENT  |
    | X Unit |  Empty |   Empty   | Lambda|    TOF   |
    | X Size |    1   |    >1     |  >1   |    >1    |
    | IsHist?|    n   |     n     |   y   |     y    |
    """

    MONO = 1  # standard monochromatic SANS
    KINETIC = 2  # kinetic monochromatic SANS
    TOF = 3  # TOF SANS [D33 only] (equidistant (LTOF) or non-equidistant (VTOF))
    REVENT = 4  # rebinned monochromatic event data


# Empty run rumber to act as a placeholder where a sample measurement is missing.
EMPTY_TOKEN = "000000"
# Quite often some samples are not measured at the some distances.
# Still, it is preferred to reduce them together in one table.
# Below is an example of why the blanks are needed:
#    | samples D1 | samples D2 | samples D3 | sample TR |
#    |     R1     |     R2     |     R3     |    R4     |
#    |     R10    |     0      |     R11    |    R12    |
# Here the sample 2 is not measured at distance 2.
# But in order to reduce it together in a vectorized way and apply the transmissions
# we will need to inject a blank measurement in the place of 0, with the same number of frames
# as the sample measurement (so 1 if mono, >1 if kinetic mono).


def return_numors_from_path(run_list):
    """Returns numors used to create a given workspace, including binary operations performed.

    Args:
        run_list: (str) containing full paths of all inputs, including binary operations on nexus files
    """
    regex_all = r"(\+)"
    p = re.compile(regex_all)
    list_entries = []
    binary_op = []
    prev_pos = 0
    for obj in p.finditer(run_list):
        list_entries.append(run_list[prev_pos : obj.span()[0]])
        prev_pos = obj.span()[1]
        binary_op.append(obj.group())
    list_entries.append(run_list[prev_pos:])  # add the last remaining file
    list_entries = [path.split(entry)[1] for entry in list_entries]
    binary_op.append("")  # there is one fewer binary operator than there are numors
    list_entries = [entry + operation for entry, operation in zip(list_entries, binary_op)]
    return "".join(list_entries)


def _return_numors_from_ws(ws_name):
    """Returns numor lists from the provided workspace name.

    Args:
        ws_name: (str) workspace name to extract numor_list from
    """
    numors = str()
    if ws_name in mtd:
        try:
            numors = mtd[ws_name].getRun().getLogData("numor_list").value
        except RuntimeError:  # numor list not set
            numors = ""
    return numors


def add_correction_information(ws, parameters):
    """Adds information regarding corrections and inputs to the provided workspace using the parameters dictionary.

    Args:
    ws: (str) workspace name to which information is to be added
    parameters: (dict) dictionary containing parameter name to be added and its value
    """
    for param in parameters:
        mtd[ws].getRun().addProperty(param, parameters[param], True)


def add_correction_numors(ws, stransmission, container, absorber, beam, flux, solvent, reference, sensitivity):
    """Adds numors used for corrections and inputs to the provided workspace.

    Args:
        ws: (str) workspace name to which information is to be added
        stransmission: (str) workspace name with sample transmission
        container: (str) workspace name with container
        absorber: (str) workspace name with absorber
        beam: (str) workspace name with beam
        flux: (str) workspace name with flux
        solvent: (str) workspace name with solvent
        reference: (str) workspace name with reference
        sensitivity: (str) workspace name with sensitivity
    """
    mtd[ws].getRun().addProperty("sample_transmission_numors", _return_numors_from_ws(stransmission), True)
    mtd[ws].getRun().addProperty("container_numors", _return_numors_from_ws(container), True)
    mtd[ws].getRun().addProperty("absorber_numors", _return_numors_from_ws(absorber), True)
    mtd[ws].getRun().addProperty("beam_numors", _return_numors_from_ws(beam), True)
    mtd[ws].getRun().addProperty("flux_numors", _return_numors_from_ws(flux), True)
    mtd[ws].getRun().addProperty("solvent_numors", _return_numors_from_ws(solvent), True)
    mtd[ws].getRun().addProperty("reference_numors", _return_numors_from_ws(reference), True)
    mtd[ws].getRun().addProperty("sensitivity_numors", _return_numors_from_ws(sensitivity), True)


def get_run_number(value):
    """
    Extracts the run number from the first run out of the string value of a
    multiple file property of numors
    """
    return path.splitext(path.basename(value.split(",")[0].split("+")[0]))[0]


def needs_processing(property_value, process_reduction_type):
    """
    Checks whether a given unary reduction needs processing or is already cached
    in ADS with expected name.
    @param property_value: the string value of the corresponding MultipleFile
                           input property
    @param process_reduction_type: the reduction_type of process
    """
    do_process = False
    ws_name = ""
    if property_value:
        run_number = get_run_number(property_value)
        ws_name = run_number + "_" + process_reduction_type
        if mtd.doesExist(ws_name):
            if isinstance(mtd[ws_name], WorkspaceGroup):
                run = mtd[ws_name][0].getRun()
            else:
                run = mtd[ws_name].getRun()
            if run.hasProperty("ProcessedAs"):
                process = run.getLogData("ProcessedAs").value
                if process == process_reduction_type:
                    logger.notice("Reusing {0} workspace: {1}".format(process_reduction_type, ws_name))
                else:
                    logger.warning("{0} workspace found, but processed differently: {1}".format(process_reduction_type, ws_name))
                    do_process = True
            else:
                logger.warning("{0} workspace found, but missing the ProcessedAs flag: {1}".format(process_reduction_type, ws_name))
                do_process = True
        else:
            do_process = True
    return [do_process, ws_name]


def needs_loading(property_value, loading_reduction_type):
    """
    Checks whether a given unary input needs loading or is already loaded in
    ADS.
    @param property_value: the string value of the corresponding FileProperty
    @param loading_reduction_type : the reduction_type of input to load
    """
    loading = False
    ws_name = ""
    if property_value:
        ws_name = path.splitext(path.basename(property_value))[0]
        if mtd.doesExist(ws_name):
            logger.notice("Reusing {0} workspace: {1}".format(loading_reduction_type, ws_name))
        else:
            loading = True
    return [loading, ws_name]


def create_name(ws):
    DISTANCE_LOG = "L2"  # the distance of the main detector
    COLLIMATION_LOG = "collimation.actual_position"
    WAVELENGTH_LOG1 = "wavelength"
    WAVELENGTH_LOG2 = "selector.wavelength"
    logs = mtd[ws].run()
    distance = logs[DISTANCE_LOG].value
    collimation = logs[COLLIMATION_LOG].value
    if WAVELENGTH_LOG1 in logs:
        wavelength = logs[WAVELENGTH_LOG1].value
    elif WAVELENGTH_LOG2 in logs:
        wavelength = logs[WAVELENGTH_LOG2].value
    return "d{:.1f}m_c{:.1f}m_w{:.1f}A".format(distance, collimation, wavelength)


def check_axis_match(ws1: MatrixWorkspace, ws2: MatrixWorkspace) -> bool:
    """
    Checks if the axis binning between the two workspaces are close enough

    Args:
        ws1 (MatrixWorkspace): left-hand side of the comparison
        ws2 (MatrixWorkspace): right-hand side of the comparison
    Return:
        Whether the workspace axes are close enough to each other
    """
    ax1 = ws1.readX(0)
    ax2 = ws2.readX(0)
    tolerance = 0.01  # no unit
    diff = abs(ax2 - ax1)
    if any(diff > tolerance):
        logger.warning(f"Binning inconsistent out of tolerance between: {ws1.name()} and {ws2.name()}.")


def check_distances_match(ws1, ws2):
    """
    Checks if the detector distance between two workspaces are close enough
    @param ws1 : workspace 1
    @param ws2 : workspace 2
    """
    tolerance = 0.01  # m
    l2_1 = ws1.getRun().getLogData("L2").value
    l2_2 = ws2.getRun().getLogData("L2").value
    r1 = ws1.getRunNumber()
    r2 = ws2.getRunNumber()
    if fabs(l2_1 - l2_2) > tolerance:
        logger.warning(f"Distance difference out of tolerance {r1}: {l2_1}, {r2}: {l2_2}")


def get_wavelength(ws):
    run = ws.getRun()
    if "wavelength" in run:
        return run["wavelength"].value
    elif "selector.wavelength" in run:
        return run["selector.wavelength"]
    else:
        raise RuntimeError("Unable to find the wavelength in workspace " + ws.name())


def check_wavelengths_match(ws1, ws2):
    """
    Checks if the wavelength difference between the data is close enough
    @param ws1 : workspace 1
    @param ws2 : workspace 2
    """
    tolerance = 0.01  # AA
    wavelength_1 = get_wavelength(ws1)
    wavelength_2 = get_wavelength(ws2)
    r1 = ws1.getRunNumber()
    r2 = ws2.getRunNumber()
    if fabs(wavelength_1 - wavelength_2) > tolerance:
        logger.warning(f"Wavelength difference out of tolerance {r1}: {wavelength_1}, {r2}: {wavelength_2}")


def check_processed_flag(ws, exp_value):
    """
    Returns true if the workspace is processed as expected, false otherwise
    @param ws : workspace
    @param exp_value : the expected value of the ProcessedAs log
    """
    if ws.getRun().getLogData("ProcessedAs").value != exp_value:
        logger.warning(f"{exp_value} workspace is not processed as such.")


def cylinder_xml(radius):
    """
    Returns XML for an infinite cylinder with axis of z (incident beam) and given radius [m]
    @param radius : the radius of the cylinder [m]
    @return : XML string for the geometry shape
    """
    return (
        '<infinite-cylinder id="flux"><centre x="0.0" y="0.0" z="0.0"/><axis x="0.0" y="0.0" z="1.0"/>'
        '<radius val="{0}"/></infinite-cylinder>'.format(radius)
    )


def monitor_id(instrument):
    """
    Returns the pair of the real monitor ID and the other, blank monitor ID for the given instrument
    TODO: These could rather be defined in the IPFs
    """
    if instrument == "D33":
        return [500000, 500001]
    elif instrument == "D16":
        return [500001, 500000]
    else:
        return [100000, 100001]


def blank_monitor_ws_neg_index(instrument):
    """Returns the negative index of the spectra corresponding to the empty monitor that will host acq times"""
    if instrument != "D16":
        return -1
    else:
        return -2


def real_monitor_ws_neg_index(instrument):
    """Returns the negative index of the spectra corresponding to the non-empty monitor"""
    if instrument != "D16":
        return -2
    else:
        return -1


def main_detector_distance(run, instrument):
    """
    Returns the main detector distance from the sample, based on the instrument
    TODO: these can be moved to the IPFs
    """
    if instrument == "D11" or instrument == "D11lr" or instrument == "D22" or instrument == "D22lr":
        return run["detector.det_calc"].value
    if instrument == "D11B":
        return run["Detector 1.det_calc"].value
    if instrument == "D22B":
        return run["Detector 1.det1_calc"].value
    if instrument == "D33":
        return run["Detector.det2_calc"].value
    raise RuntimeError("Unable to find the main detector distance, is the instrument supported?")


def get_vertical_grouping_pattern(ws):
    """
    Provides vertical grouping pattern and crops to the main detector panel where counts from the beam are measured.
    Used for fitting the horizontal incident beam profile for q resolution calculation.
    TODO: These are static and can be turned to grouping files in instrument/Grouping folder
    :param ws: Empty beam workspace.
    """
    inst_name = mtd[ws].getInstrument().getName()
    min_id = 0
    if "D11" in inst_name:
        if "lr" in inst_name:
            step = 128
            max_id = 16384
        elif "B" in inst_name:
            CropToComponent(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames="detector_center")
            max_id = 49152
            step = 192
        else:
            step = 256
            max_id = 65536
    elif "D22" in inst_name:
        max_id = 32768
        step = 256
        if "lr" in inst_name:
            step = 128
            max_id = 16384
        elif "B" in inst_name:
            CropToComponent(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames="detector_back")
    elif "D33" in inst_name:
        CropToComponent(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames="back_detector")
        max_id = 32768
        step = 128
    else:
        logger.warning("Instruments other than D11, D22, and D33 are not yet supported for direct beam width fitting.")
        return
    return ",".join(["{}-{}".format(start, start + step - 1) for start in range(min_id, max_id, step)])
