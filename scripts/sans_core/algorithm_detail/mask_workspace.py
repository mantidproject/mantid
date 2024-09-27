# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod

from mantid.kernel import Logger

from sans_core.algorithm_detail.mask_functions import SpectraBlock
from sans_core.algorithm_detail.xml_shapes import add_cylinder, add_outside_cylinder, create_phi_mask, create_line_mask
from sans_core.common.constants import EMPTY_NAME
from sans_core.common.enums import SANSInstrument
from sans_core.common.file_information import find_full_file_path, get_instrument_paths_for_sans_file
from sans_core.common.general_functions import create_unmanaged_algorithm


# ------------------------------------------------------------------
# Free functions
# ------------------------------------------------------------------
def mask_bins(mask_info, workspace, detector_type):
    """
    Masks the bins on a workspace

    There are two parts to this:
    1. A general time mask is applied to all spectra
    2. A detector-specific time mask is applied depending on the detector_type
    :param mask_info: a SANSStateMask object
    :param workspace: the workspace which is about to be masked.
    :param detector_type: the detector which is currently being investigated and which (potentially) requires
                          additional masking
    :return: the workspace
    """
    # Mask the bins with the general setting
    bin_mask_general_start = mask_info.bin_mask_general_start
    bin_mask_general_stop = mask_info.bin_mask_general_stop
    # Mask the bins with the detector-specific setting
    bin_mask_start = mask_info.detectors[detector_type.value].bin_mask_start
    bin_mask_stop = mask_info.detectors[detector_type.value].bin_mask_stop

    # Combine the settings and run the binning
    start_mask = []
    stop_mask = []
    if bin_mask_general_start and bin_mask_general_stop:
        start_mask.extend(bin_mask_general_start)
        stop_mask.extend(bin_mask_general_stop)

    if bin_mask_start and bin_mask_stop:
        start_mask.extend(bin_mask_start)
        stop_mask.extend(bin_mask_stop)

    mask_name = "MaskBins"
    mask_options = {"InputWorkspace": workspace}
    mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
    for start, stop in zip(start_mask, stop_mask):
        # Bin mask should be operated in place
        mask_alg.setProperty("InputWorkspace", workspace)
        mask_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
        mask_alg.setProperty("OutputWorkspace", workspace)
        mask_alg.setProperty("XMin", start)
        mask_alg.setProperty("XMax", stop)
        mask_alg.execute()
        workspace = mask_alg.getProperty("OutputWorkspace").value
    return workspace


def mask_cylinder(mask_info, workspace):
    """
    Masks  a (hollow) cylinder around (0,0)

    Two radii can be specified for the cylinder mask. An inner radius (radius_min) and an outer radius(radius_max)
    which specify a hollow cylinder mask.
    :param mask_info: a SANSStateMask object.
    :param workspace: the workspace which is about to be masked
    :return: the masked workspace.
    """
    radius_min = mask_info.radius_min
    radius_max = mask_info.radius_max

    xml = []
    # Set up the inner radius of the cylinder
    if radius_min is not None and radius_min > 0.0:
        add_cylinder(xml, radius_min, 0, 0, "beam_stop")

    # Set up the outer radius of the cylinder
    if radius_max is not None and radius_max > 0.0:
        add_outside_cylinder(xml, radius_max, 0, 0, "beam_area")

    # Mask the cylinder shape if there is anything to mask, else don't do anything
    if xml:
        mask_name = "MaskDetectorsInShape"
        mask_options = {"Workspace": workspace}
        mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
        for shape in xml:
            mask_alg.setProperty("Workspace", workspace)
            mask_alg.setProperty("ShapeXML", shape)
            mask_alg.execute()
            workspace = mask_alg.getProperty("Workspace").value
    return workspace


def mask_with_mask_files(mask_info, inst_info, workspace):
    """
    Apply mask files to the workspace

    Rolling our own MaskDetectors wrapper since masking is broken in a couple
    of places that affect us here.
    Calling MaskDetectors(Workspace=ws_name, MaskedWorkspace=mask_ws_name) is
    not something we can do because the algorithm masks by ws index rather than
    detector id, and unfortunately for SANS the detector table is not the same
    for MaskingWorkspaces as it is for the workspaces containing the data to be
    masked.  Basically, we get a mirror image of what we expect.  Instead, we
    have to extract the det IDs and use those via the DetectorList property.
    :param mask_info: a SANSStateMask object.
    :param workspace: the workspace to be masked.
    :return: the masked workspace.
    """
    mask_files = mask_info.mask_files
    if mask_files:
        idf_path = inst_info.idf_path

        # Mask loader
        load_name = "LoadMask"
        load_options = {"Instrument": idf_path, "OutputWorkspace": EMPTY_NAME}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        mask_alg = create_unmanaged_algorithm("MaskDetectors")

        file_paths = [find_full_file_path(i) for i in mask_files]
        # Find full file path returns an empty string, so we need to remake it
        missing_file_paths = [mask_files[i] for i, path in enumerate(file_paths) if not path]

        if missing_file_paths:
            err_str = "The following mask files are missing:"
            err_str += "\n".join(missing_file_paths)
            raise FileNotFoundError(err_str)

        # Masker
        for mask_file in file_paths:
            # Get the detector ids which need to be masked
            load_alg.setProperty("InputFile", mask_file)
            load_alg.execute()
            masking_workspace = load_alg.getProperty("OutputWorkspace").value
            # Could use MaskDetectors directly with masking_workspace but it does not
            # support MPI. Use a two step approach via a and b instead.
            # a) Extract detectors to mask from MaskWorkspace
            det_ids = masking_workspace.getMaskedDetectors()
            # b) Mask the detector ids on the instrument
            mask_alg.setProperty("Workspace", workspace)
            mask_alg.setProperty("DetectorList", det_ids)
            mask_alg.execute()
            workspace = mask_alg.getProperty("Workspace").value
    return workspace


def mask_spectra(mask_info, workspace, spectra_block, detector_type):
    """
    Masks particular spectra on the workspace.

    There are several spectra specifications which need to be evaluated
    1. General singular spectrum numbers
    2. General spectrum ranges
    3. Detector-specific horizontal singular strips
    4. Detector-specific horizontal range strips
    5. Detector-specific vertical singular strips
    6. Detector-specific vertical range strips
    7. Blocks
    8. Cross Blocks
    :param mask_info: a SANSStateMask object.
    :param workspace: the workspace to be masked.
    :param spectra_block: a SpectraBlock object, which contains instrument information to
                          calculate the selected spectra.
    :param detector_type: the selected detector type
    :return: the masked workspace.
    """

    total_spectra = []

    # All masks are detector-specific, hence we pull out only the relevant part
    detector = mask_info.detectors[detector_type.value]

    # ----------------------
    # Single spectra
    # -----------------------
    single_spectra = detector.single_spectra
    if single_spectra:
        total_spectra.extend(single_spectra)

    # ----------------------
    # Spectrum range
    # -----------------------
    spectrum_range_start = detector.spectrum_range_start
    spectrum_range_stop = detector.spectrum_range_stop
    if spectrum_range_start and spectrum_range_stop:
        for start, stop in zip(spectrum_range_start, spectrum_range_stop):
            total_spectra.extend(list(range(start, stop + 1)))

    # ---------------------------
    # Horizontal single spectrum
    # ---------------------------
    single_horizontal_strip_masks = detector.single_horizontal_strip_mask
    if single_horizontal_strip_masks:
        for single_horizontal_strip_mask in single_horizontal_strip_masks:
            total_spectra.extend(spectra_block.get_block(single_horizontal_strip_mask, 0, 1, None))

    # ---------------------------
    # Vertical single spectrum
    # ---------------------------
    single_vertical_strip_masks = detector.single_vertical_strip_mask
    if single_vertical_strip_masks:
        for single_vertical_strip_mask in single_vertical_strip_masks:
            total_spectra.extend(spectra_block.get_block(0, single_vertical_strip_mask, None, 1))

    # ---------------------------
    # Horizontal spectrum range
    # ---------------------------
    range_horizontal_strip_start = detector.range_horizontal_strip_start
    range_horizontal_strip_stop = detector.range_horizontal_strip_stop
    if range_horizontal_strip_start and range_horizontal_strip_stop:
        for start, stop in zip(range_horizontal_strip_start, range_horizontal_strip_stop):
            number_of_strips = abs(stop - start) + 1
            total_spectra.extend(spectra_block.get_block(start, 0, number_of_strips, None))

    # ---------------------------
    # Vertical spectrum range
    # ---------------------------
    range_vertical_strip_start = detector.range_vertical_strip_start
    range_vertical_strip_stop = detector.range_vertical_strip_stop
    if range_vertical_strip_start and range_vertical_strip_stop:
        for start, stop in zip(range_vertical_strip_start, range_vertical_strip_stop):
            number_of_strips = abs(stop - start) + 1
            total_spectra.extend(spectra_block.get_block(0, start, None, number_of_strips))

    # ---------------------------
    # Blocks
    # ---------------------------
    block_horizontal_start = detector.block_horizontal_start
    block_horizontal_stop = detector.block_horizontal_stop
    block_vertical_start = detector.block_vertical_start
    block_vertical_stop = detector.block_vertical_stop

    if block_horizontal_start and block_horizontal_stop and block_vertical_start and block_vertical_stop:
        for h_start, h_stop, v_start, v_stop in zip(
            block_horizontal_start, block_horizontal_stop, block_vertical_start, block_vertical_stop
        ):
            x_dim = abs(v_stop - v_start) + 1
            y_dim = abs(h_stop - h_start) + 1
            total_spectra.extend(spectra_block.get_block(h_start, v_start, y_dim, x_dim))

    # ---------------------------
    # Blocks Cross
    # ---------------------------
    block_cross_horizontal = detector.block_cross_horizontal
    block_cross_vertical = detector.block_cross_vertical

    if block_cross_horizontal and block_cross_vertical:
        for horizontal, vertical in zip(block_cross_horizontal, block_cross_vertical):
            total_spectra.extend(spectra_block.get_block(horizontal, vertical, 1, 1))

    if not total_spectra:
        return workspace

    # Perform the masking
    ws_spectra_list = workspace.getSpectrumNumbers()
    # Any gaps in the spectra list we skip over attempting to mask
    filtered_mask_spectra = [spec for spec in total_spectra if spec in ws_spectra_list]

    if len(filtered_mask_spectra) != len(total_spectra):
        log = Logger("SANS - Mask Workspace")
        log.warning(
            "Skipped masking some spectrum numbers that do not exist in the workspace. Re-run"
            " with logging set to information for more details"
        )
        log.information("The following spectrum numbers do not exist in the ws (cropped to component):")
        for i in list(set(total_spectra) - set(filtered_mask_spectra)):
            log.information(str(i))

    mask_name = "MaskSpectra"
    mask_options = {"InputWorkspace": workspace, "InputWorkspaceIndexType": "SpectrumNumber", "OutputWorkspace": "__dummy"}
    mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
    mask_alg.setProperty("InputWorkspaceIndexSet", list(set(filtered_mask_spectra)))
    mask_alg.setProperty("OutputWorkspace", workspace)
    mask_alg.execute()
    workspace = mask_alg.getProperty("OutputWorkspace").value
    return workspace


def mask_angle(mask_info, workspace):
    """
    Creates a pizza slice mask on the detector around (0,0)

    :param mask_info: a SANSStateMask object
    :param workspace: the workspace which is to be masked.
    :return: a masked workspace
    """
    phi_mirror = mask_info.use_mask_phi_mirror
    phi_min = mask_info.phi_min
    phi_max = mask_info.phi_max
    if phi_min is not None and phi_max is not None and phi_mirror is not None:
        # Check for edge cases for the mirror
        if phi_mirror:
            if phi_min > phi_max:
                phi_min, phi_max = phi_max, phi_min

            if phi_max - phi_min == 180.0:
                phi_min = -90.0
                phi_max = 90.0

        # Create the phi mask and apply it if anything was created
        phi_mask = create_phi_mask("unique phi", [0, 0, 0], phi_min, phi_max, phi_mirror)

        if phi_mask:
            mask_name = "MaskDetectorsInShape"
            mask_options = {"Workspace": workspace, "ShapeXML": phi_mask}
            mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
            mask_alg.execute()
            workspace = mask_alg.getProperty("Workspace").value
    return workspace


def mask_beam_stop(mask_info, workspace):
    """
    The beam stop is being masked here.

    :param mask_info: a SANSStateMask object.
    :param workspace: the workspace which is to be masked.
    :return: a masked workspace
    """
    beam_stop_arm_width = mask_info.beam_stop_arm_width
    beam_stop_arm_angle = mask_info.beam_stop_arm_angle
    beam_stop_arm_pos1 = mask_info.beam_stop_arm_pos1
    beam_stop_arm_pos2 = mask_info.beam_stop_arm_pos2

    if not beam_stop_arm_width or not beam_stop_arm_angle:
        return workspace

    lab_ipf_key = "low-angle-detector-name"

    lab_component_name = workspace.getInstrument().getStringParameter(lab_ipf_key)
    if not lab_component_name:
        raise KeyError("{0} was not found in the IPF file for this instrument")
    lab_component_name = lab_component_name[0]

    comp_info = workspace.componentInfo()
    detector_index = comp_info.indexOfAny(lab_component_name)
    detector_pos = comp_info.position(detector_index)
    z_position = detector_pos.getZ()

    start_point = [beam_stop_arm_pos1, beam_stop_arm_pos2, z_position]
    line_mask = create_line_mask(start_point, 100.0, beam_stop_arm_width, beam_stop_arm_angle)

    mask_name = "MaskDetectorsInShape"
    mask_options = {"Workspace": workspace, "ShapeXML": line_mask}
    mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
    mask_alg.execute()
    workspace = mask_alg.getProperty("Workspace").value
    return workspace


# ------------------------------------------------------------------
# Masker classes
# ------------------------------------------------------------------


class Masker(metaclass=ABCMeta):
    def __init__(self):
        super(Masker, self).__init__()

    @abstractmethod
    def mask_workspace(self, mask_info, inst_info, workspace_to_mask, detector_type):
        pass


class NullMasker(Masker):
    def __init__(self):
        super(NullMasker, self).__init__()

    def mask_workspace(self, mask_info, inst_info, workspace_to_mask, detector_type):
        return workspace_to_mask


class MaskerISIS(Masker):
    def __init__(self, spectra_block):
        super(MaskerISIS, self).__init__()
        self._spectra_block = spectra_block

    def mask_workspace(self, mask_info, inst_info, workspace_to_mask, detector_type):
        """
        Performs the different types of masks that are currently available for ISIS reductions.

        :param mask_info: a SANSStateMask object.
        :param workspace_to_mask: the workspace to mask.
        :param detector_type: the detector type which is currently used , i.e. HAB or LAB
        :param progress: a Progress object
        :return: a masked workspace.
        """
        # Perform bin masking
        workspace_to_mask = mask_bins(mask_info, workspace_to_mask, detector_type)

        # Perform cylinder masking
        workspace_to_mask = mask_cylinder(mask_info, workspace_to_mask)

        # Apply the xml mask files
        workspace_to_mask = mask_with_mask_files(mask_info, inst_info, workspace_to_mask)

        # Mask spectrum list
        workspace_to_mask = mask_spectra(mask_info, workspace_to_mask, self._spectra_block, detector_type)

        # Mask angle
        workspace_to_mask = mask_angle(mask_info, workspace_to_mask)

        # Mask beam stop
        return mask_beam_stop(mask_info, workspace_to_mask)


def create_masker(state, detector_type):
    """
    Provides the appropriate masker.

    :param state: a SANSState object
    :param detector_type: either HAB or LAB
    :return: the corresponding slicer
    """
    data_info = state.data
    instrument = data_info.instrument

    # TODO remove this shim

    if (
        instrument is SANSInstrument.LARMOR
        or instrument is SANSInstrument.LOQ
        or instrument is SANSInstrument.SANS2D
        or instrument is SANSInstrument.ZOOM
    ):
        run_number = data_info.sample_scatter_run_number
        file_name = data_info.sample_scatter
        _, ipf_path = get_instrument_paths_for_sans_file(file_name)
        spectra_block = SpectraBlock(ipf_path, run_number, instrument, detector_type)
        masker = MaskerISIS(spectra_block)
    else:
        masker = NullMasker()
        NotImplementedError("create_masker: Other instruments are not implemented yet.")
    return masker
