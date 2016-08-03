from abc import (ABCMeta, abstractmethod)
from SANS2.Common.SANSEnumerations import (SANSInstrument, convert_detector_type_to_string)
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm
from SANS2.Common.SANSFileInformation import find_full_file_path
from SANS2.State.SANSStateFunctions import get_instrument_from_state_data
from SANS.Mask.XMLShapes import (add_cylinder, add_outside_cylinder, create_phi_mask, create_line_mask)
from SANS.Mask.MaskFunctions import (yield_masked_det_ids, SpectraBlock)


# ------------------------------------------------------------------
# Free functions
# ------------------------------------------------------------------
def mask_bins(mask_info, workspace, detector_type):
    # Mask the bins with the general setting
    bin_mask_general_start = mask_info.bin_mask_general_start
    bin_mask_general_stop = mask_info.bin_mask_general_stop
    # Mask the bins with the detector-specific setting
    bin_mask_start = mask_info.detectors[convert_detector_type_to_string(detector_type)].bin_mask_start
    bin_mask_stop = mask_info.detectors[convert_detector_type_to_string(detector_type)].bin_mask_stop

    # Combine the settings and run the binning
    start_mask = bin_mask_general_start.extend(bin_mask_start)
    stop_mask = bin_mask_general_stop.extend(bin_mask_stop)

    mask_name = "MaskBins"
    mask_options = {"InputWorkspace": workspace}
    mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
    for start, stop in zip(start_mask, stop_mask):
        mask_alg.setProperty("InputWorkspace", workspace)
        mask_alg.setProperty("OutputWorkspace", "dummy")
        mask_alg.setProperty("XMin", start)
        mask_alg.setProperty("XMax", stop)
        mask_alg.execute()
        workspace = mask_alg.getProperty("OutputWorkspace").value
    return workspace


def mask_cylinder(mask_info, workspace):
    radius_min = mask_info.radius_min
    radius_max = mask_info.radius_max

    xml = []
    # Set up the inner radius of the cylinder
    if radius_min is not None and radius_min > 0.0:
        add_cylinder(xml, radius_min, 0, 0, 'beam_stop')

    # Set up the outer radius of the cylinder
    if radius_max is not None and radius_max > 0.0:
        add_outside_cylinder(xml, radius_max, 0, 0, 'beam_area')

    # Mask the cylinder shape if there is anything to mask, else don't do anything
    if xml:
        mask_name = "MaskDetectorsInShape"
        mask_options = {"InputWorkspace": workspace}
        mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
        for shape in xml:
            mask_alg.setProperty("InputWorkspace", workspace)
            mask_alg.setProperty("OutputWorkspace", "dummy")
            mask_alg.setProperty("ShapeXML", shape)
            mask_alg.execute()
            workspace = mask_alg.getProperty("OutputWorkspace").value
    return workspace


def mask_with_mask_files(mask_info, workspace):
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
    :param mask_info: The mask state
    :param workspace: The workspace to be masked
    :return: The workspace with the mask files applied to it
    """
    mask_files = mask_info.mask_files
    if mask_files:
        idf_path = mask_info.idf_path

        # Mask loader
        load_name = "LoadMask"
        load_options = {"Instrument": idf_path,
                        "OutputWorkspace": "dummy"}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)

        # Masker
        mask_name = "MaskDetectors"
        mask_options = {"Workspace": workspace}
        mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
        for mask_file in mask_files:
            mask_file = find_full_file_path(mask_file)

            # Get the detector ids which need to be masked
            load_alg.setProperty("InputFile", mask_file)
            load_alg.execute()
            masking_workspace = load_alg.getProperty("OutputWorkspace").value
            detector_list = list(yield_masked_det_ids(masking_workspace))

            # Mask the detector ids on the original workspace
            mask_alg.setProperty("DetectorList", detector_list)
            mask_alg.execute()
        workspace = mask_alg.getProperty("Workspace").value
    return workspace


def mask_spectra(mask_info, workspace, spectra_block, detector_type):
    # There are several spectra sepcifications which need to be evaluated
    # 1. General singular spectrum numbers
    # 2. General spectrum ranges
    # 3. Detector-specific horizontal singular strips
    # 4. Detector-specific horizontal range strips
    # 5. Detector-specific vertical singular strips
    # 6. Detector-specific vertical range strips
    # 7. Blocks
    # 8. Cross Blocks
    total_spectra = []

    # ----------------------
    # Single spectra
    # -----------------------
    single_spectra = mask_info.single_spectra
    if single_spectra:
        total_spectra.extend(single_spectra)

    # ----------------------
    # Spectrum range
    # -----------------------
    spectrum_range_start = mask_info.spectrum_range_start
    spectrum_range_stop = mask_info.spectrum_range_stop
    if spectrum_range_start and spectrum_range_stop:
        for start, stop in zip(spectrum_range_start, spectrum_range_stop):
            total_spectra.extend(range(start, stop + 1))

    # Detector specific masks
    detector = mask_info.detectors[convert_detector_type_to_string(detector_type)]

    # ---------------------------
    # Horizontal single spectrum
    # ---------------------------
    single_horizontal_strip_mask = detector.single_horizontal_strip_mask
    if single_horizontal_strip_mask:
        total_spectra.extend(single_horizontal_strip_mask)

    # ---------------------------
    # Vertical single spectrum
    # ---------------------------
    single_vertical_strip_mask = detector.single_vertical_strip_mask
    if single_vertical_strip_mask:
        total_spectra.extend(single_vertical_strip_mask)

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
        for h_start, h_stop, v_start, v_stop in zip(block_horizontal_start, block_horizontal_stop,
                                                    block_vertical_start, block_vertical_stop):
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
            x_dim = 1
            y_dim = 1
            total_spectra.extend(spectra_block.get_block(horizontal, vertical, y_dim, x_dim))

    # Perform the masking
    if total_spectra:
        mask_name = "MaskDetectors"
        mask_options = {"Workspace": workspace,
                        "SpectraList": set(total_spectra)}
        mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
        mask_alg.execute()
        workspace = mask_alg.getProperty("Workspace").value
    return workspace


def mask_angle(mask_info, workspace):
    """ Creates a pizza slice mask on the detector"""
    phi_mirror = mask_info.use_mask_phi_mirror
    phi_min = mask_info.phi_min
    phi_max = mask_info.phi_max

    if phi_min and phi_max and phi_mirror:
        # Check for edge cases for the mirror
        if phi_mirror:
            if phi_min > phi_max:
                phi_min, phi_max = phi_max, phi_min

            if phi_max - phi_min == 180.0:
                phi_min = -90.0
                phi_max = 90.0

        # Create the phi mask and apply it if anything was created
        phi_mask = create_phi_mask('unique phi', [0, 0, 0], phi_min, phi_max, phi_mirror)
        if phi_mask:
            mask_name = "MaskDetectorsInShape"
            mask_options = {"Workspace": workspace,
                            "ShapeXML": phi_mask}
            mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
            mask_alg.execute()
            workspace = mask_alg.getProperty("OutputWorkspace").value
    return workspace


def mask_beam_stop(mask_info, workspace, instrument):
    """ The beam stop is being masked here. Note that this is only implemented for SANS2D"""
    beam_stop_arm_width = mask_info.beam_stop_arm_width
    beam_stop_arm_angle = mask_info.beam_stop_arm_angle
    beam_stop_arm_pos1 = mask_info.beam_stop_arm_pos1
    beam_stop_arm_pos2 = mask_info.beam_stop_arm_pos2

    if beam_stop_arm_width and beam_stop_arm_angle:
        if instrument is SANSInstrument.SANS2D:
            detector = workspace.getInstrument().getComponentByName('rear-detector')
            z_position = detector.getPos().getZ()
            start_point = [beam_stop_arm_pos1, beam_stop_arm_pos2, z_position]
            line_mask = create_line_mask(start_point, 1e6, beam_stop_arm_width, beam_stop_arm_angle)

            mask_name = "MaskDetectorsInShape"
            mask_options = {"Workspace": workspace,
                            "ShapeXML": line_mask}
            mask_alg = create_unmanaged_algorithm(mask_name, **mask_options)
            mask_alg.execute()
            workspace = mask_alg.getProperty("Workspace").value
    return workspace


# ------------------------------------------------------------------
# Masker classes
# ------------------------------------------------------------------

class Masker(object):
    __metaclass__ = ABCMeta

    def __init__(self):
        super(Masker, self).__init__()

    @abstractmethod
    def mask_workspace(self, mask_info, workspace_to_mask, detector_type):
        pass


class NullMasker(Masker):
    __metaclass__ = ABCMeta

    def __init__(self):
        super(NullMasker, self).__init__()

    def mask_workspace(self, mask_info, workspace_to_mask, detector_type):
        _ = mask_info
        return workspace_to_mask


class MaskerISIS(Masker):
    def __init__(self, spectra_block, instrument):
        super(MaskerISIS, self).__init__()
        self._spectra_block = spectra_block
        self._instrument = instrument

    def mask_workspace(self, mask_info, workspace_to_mask, detector_type):
        # Perform bin masking
        workspace_to_mask = mask_bins(mask_info, workspace_to_mask, detector_type)

        # Perform cylinder masking
        workspace_to_mask = mask_cylinder(mask_info, workspace_to_mask)

        # Apply the xml mask files
        workspace_to_mask = mask_with_mask_files(mask_info, workspace_to_mask)

        # Mask spectrum list
        workspace_to_mask = mask_spectra(mask_info, workspace_to_mask, self._spectra_block, detector_type)

        # Mask angle
        workspace_to_mask = mask_angle(mask_info, workspace_to_mask)

        # Mask beam stop
        mask_beam_stop(mask_info, workspace_to_mask, self._instrument)


class MaskFactory(object):
    def __init__(self):
        super(MaskFactory, self).__init__()

    @staticmethod
    def create_masker(state):
        """
        Provides the appropriate masker.

        :param state: a SANSState object
        :return: the corresponding slicer
        """
        data_info = state.data
        instrument = get_instrument_from_state_data(data_info)

        if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.LOQ or \
                        instrument is SANSInstrument.SANS2D:
            spectra_block = SpectraBlock(data_info)
            masker = MaskerISIS(spectra_block, instrument)
        else:
            masker = NullMasker()
            NotImplementedError("MaskFactory: Other instruments are not implemented yet.")
        return masker
