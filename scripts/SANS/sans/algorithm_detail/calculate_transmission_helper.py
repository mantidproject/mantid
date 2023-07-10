# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import ExperimentInfo
from sans.common.general_functions import create_unmanaged_algorithm, sanitise_instrument_name
from sans.common.constants import EMPTY_NAME


def apply_flat_background_correction_to_detectors(workspace, flat_background_correction_start, flat_background_correction_stop):
    """
    Applies the flat background correction to all detectors which are not monitors

    :param workspace: the workspace which contains detector spectra which will be corrected.
    :param flat_background_correction_start: the start of the flat background region
    :param flat_background_correction_stop: the end of the flat background region
    :return: a corrected workspace
    """
    if flat_background_correction_start is not None and flat_background_correction_stop is not None:
        flat_name = "CalculateFlatBackground"
        flat_options = {
            "InputWorkspace": workspace,
            "Mode": "Mean",
            "StartX": flat_background_correction_start,
            "EndX": flat_background_correction_stop,
            "SkipMonitors": True,
        }
        flat_alg = create_unmanaged_algorithm(flat_name, **flat_options)
        flat_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        flat_alg.setProperty("OutputWorkspace", workspace)
        flat_alg.execute()
        workspace = flat_alg.getProperty("OutputWorkspace").value
    return workspace


def apply_flat_background_correction_to_monitors(
    workspace,
    monitor_indices,
    background_TOF_monitor_start,
    background_TOF_monitor_stop,
    background_TOF_general_start,
    background_TOF_general_stop,
):
    """
    Applies the flat background correction to some monitors


    :param workspace: the workspace which contains monitor spectra which will be corrected.
    :param monitor_indices: the workspace indices of the monitors which will be corrected.
    :param background_TOF_monitor_start: a dictionary where the keys are spectrum numbers of monitors (as strings) and
                                         the values are the start time of the flat background correction.
    :param background_TOF_monitor_stop: a dictionary where the keys are spectrum numbers of monitors (as strings) and
                                        the values are the stop time of the flat background correction.
    :param background_TOF_general_start: the start value of the general background region. This is used if
                                         the monitor-specific setting does not exist
    :param background_TOF_general_stop: the stop value of the general background region. This is used if
                                         the monitor-specific setting does not exist
    :return: a corrected workspace.
    """
    for workspace_index in monitor_indices:
        # Get the flat background region for this monitor.
        spectrum = workspace.getSpectrum(workspace_index)
        spectrum_number = spectrum.getSpectrumNo()
        monitor_key = str(spectrum_number)
        if (
            monitor_key not in background_TOF_monitor_start
            and monitor_key not in background_TOF_monitor_stop
            and background_TOF_general_start is None
            and background_TOF_general_stop is None
        ):
            continue
        tof_start = (
            background_TOF_monitor_start[monitor_key] if monitor_key in background_TOF_monitor_start else background_TOF_general_start
        )
        tof_stop = background_TOF_monitor_stop[monitor_key] if monitor_key in background_TOF_monitor_stop else background_TOF_general_stop

        flat_name = "CalculateFlatBackground"
        flat_options = {
            "InputWorkspace": workspace,
            "Mode": "Mean",
            "StartX": tof_start,
            "EndX": tof_stop,
            "WorkspaceIndexList": workspace_index,
        }
        flat_alg = create_unmanaged_algorithm(flat_name, **flat_options)
        flat_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        flat_alg.setProperty("OutputWorkspace", workspace)
        flat_alg.execute()
        workspace = flat_alg.getProperty("OutputWorkspace").value
    return workspace


def get_workspace_indices_for_monitors(workspace):
    """
    Creates a generator of workspaces indices corresponding to spectra which are actually monitors

    :param workspace: workspace to check for monitors.
    :return: a generator for workspace indices.
    """
    for index in range(workspace.getNumberHistograms()):
        detector = workspace.getDetector(index)
        if detector.isMonitor():
            yield index


def get_detector_id_for_spectrum_number(workspace, spectrum_number):
    """
    Gets the detector id of a spectrum for a given spectrum number.

    :param workspace: the workspace with the relevant spectrum.
    :param spectrum_number: the spectrum number.
    :return: the corresponding detector id.
    """
    try:
        workspace_index = workspace.getIndexFromSpectrumNumber(spectrum_number)
        detector = workspace.getDetector(workspace_index)
        detector_id = detector.getID()
    except RuntimeError:
        detector_id = None
    return detector_id


def get_idf_path_from_workspace(workspace):
    """
    Gets the full IDF path from a workspace.

    It queries the workspace for the start time and instrument name. It gets the IDF path from the ExperimentInfo.
    :param workspace: the workspace for which we want the full IDF path.
    :return: the full IDF path for the instrument of the workspace.
    """
    run = workspace.run()
    instrument = workspace.getInstrument()
    instrument_name = instrument.getName()
    instrument_name = sanitise_instrument_name(instrument_name)
    if run.hasProperty("start_time"):
        time = run.getProperty("start_time").value
        idf_path = ExperimentInfo.getInstrumentFilename(instrument_name, time)
    elif run.hasProperty("run_start"):
        time = run.getProperty("run_start").value
        idf_path = ExperimentInfo.getInstrumentFilename(instrument_name, time)
    else:
        idf_path = None
    return idf_path


def get_masked_det_ids_from_mask_file(mask_file_path, idf_path):
    """
    Given a mask file and the (necessary) path to the corresponding IDF, will
    load in the file and return a list of detector IDs that are masked.

    TODO: Investigate if there is a better way of finding the detector ids from a mask file. This is a minor performance
          bottleneck and does not seem quite right
         * Check if parsing the file provides a better performance

    :param mask_file_path: the path of the mask file to read in
    :param idf_path: the path to the corresponding IDF. Necessary so that we
                       know exactly which instrument to use, and therefore know
                       the correct detector IDs.
    :return the list of detector IDs that were masked in the file
    """
    mask_name = "LoadMask"
    mask_options = {"Instrument": idf_path, "InputFile": mask_file_path, "OutputWorkspace": EMPTY_NAME}
    mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
    mask_alg.execute()
    workspace = mask_alg.getProperty("OutputWorkspace").value
    return list(yield_masked_det_ids(workspace))


def yield_masked_det_ids(masking_workspace):
    """
    For some reason Detector.isMasked() does not work for MaskingWorkspaces.
    We use masking_ws.readY(ws_index)[0] == 1 instead.

    :param masking_workspace: a mask workspace
    :return: a list of detector ids
    """
    for ws_index in range(masking_workspace.getNumberHistograms()):
        if masking_workspace.readY(ws_index)[0] == 1:
            yield masking_workspace.getDetector(ws_index).getID()


def get_masked_det_ids(workspace):
    """
    Given a workspace, will return a list of all the IDs that correspond to
    detectors that have been masked.

    :param workspace : the workspace to extract the det IDs from
    :return: a list of IDs for masked detectors
    """
    for ws_index in range(workspace.getNumberHistograms()):
        try:
            detector = workspace.getDetector(ws_index)
        except RuntimeError:
            # Skip the rest after finding the first spectra with no detectors,
            # which is a big speed increase for SANS2D.
            break
        if detector.isMasked():
            yield detector.getID()


def infinite_cylinder_xml(id_name, centre, radius, axis):
    """
    Creates a mask for an infinite cylinder along the z axis
    :param id_name: the id name
    :param centre: a collection with three entries defining the centre
    :param radius: the cylinder radius
    :param axis: a collection with three entries defining the axis
    :return: the infinite cylinder masking xml
    """
    return (
        '<infinite-cylinder id="'
        + str(id_name)
        + '">'
        + '<centre x="'
        + str(centre[0])
        + '" y="'
        + str(centre[1])
        + '" z="'
        + str(centre[2])
        + '" />'
        + '<axis x="'
        + str(axis[0])
        + '" y="'
        + str(axis[1])
        + '" z="'
        + str(axis[2])
        + '" />'
        + '<radius val="'
        + str(radius)
        + '" />'
        + "</infinite-cylinder>\n"
    )


def mask_with_cylinder(workspace, radius, x_centre, y_centre, algebra):
    """
    Mask a cylinder on the input workspace.

    :param workspace: the workspace to mask
    :param radius: the masking radius
    :param x_centre: the x position of the masking radius
    :param y_centre: the y position of the masking radius
    :param algebra: a masking algebra
    """
    xml_def = infinite_cylinder_xml("shape", [x_centre, y_centre, 0.0], radius, [0, 0, 1])
    xml_def += '<algebra val="' + algebra + 'shape" />'

    mask_name = "MaskDetectorsInShape"
    mask_options = {"Workspace": workspace, "ShapeXML": xml_def}
    mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
    mask_alg.execute()
    return mask_alg.getProperty("Workspace").value


def get_region_of_interest(workspace, radius=None, roi_files=None, mask_files=None):
    """
    Calculate the various contributions to the "region of interest", used in the
    transmission calculation.

    The region of interest can be made up of a circle of detectors (with a given radius)
    around the beam centre, and/or one or more mask files, and/or the main detector bank.
    Note that the mask files wont actually be used for masking, we're just piggy-backing
    on the functionality that they provide. Note that in the case of a radius, we have
    to ensure that we do not use a workspace which already has masked detectors, since
    they would contribute to the ROI.
    :param workspace: the workspace which is used for the transmission calculation
    :param radius: the radius of the region of interest
    :param roi_files: a list of roi files. Spectra in the ROI contribute to the
                      transmission calculation.
    :param mask_files: a list of mask files. Spectra in the Mask explicitly do not
                       contribute to the transmission calculation.
    :return: a list of spectrum numbers
    """
    trans_roi = []

    if radius is not None:
        # Mask out a cylinder with the given radius in a copy of the workspace.
        # The centre position of the Cylinder does not require a shift, as all
        # components have been shifted already, when the workspaces were loaded
        clone_name = "CloneWorkspace"
        clone_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME}
        clone_alg = create_unmanaged_algorithm(clone_name, **clone_options)
        clone_alg.execute()
        cloned_workspace = clone_alg.getProperty("OutputWorkspace").value

        # Mask the cylinder around a centre of (0, 0)
        mask_with_cylinder(cloned_workspace, radius, 0.0, 0.0, "")

        # Extract the masked detector ID's.
        trans_roi += get_masked_det_ids(cloned_workspace)

    idf_path = get_idf_path_from_workspace(workspace)

    if roi_files is not None and idf_path is not None:
        for roi_file in roi_files:
            trans_roi += get_masked_det_ids_from_mask_file(roi_file, idf_path)

    masked_ids = []
    if mask_files is not None and idf_path is not None:
        for mask_file in mask_files:
            masked_ids += get_masked_det_ids_from_mask_file(mask_file, idf_path)

    # Detector ids which are not allowed and specified by "masked_ids" need to
    # be removed from the trans_roi list
    # Remove duplicates and sort.
    return sorted(set(trans_roi) - set(masked_ids))
