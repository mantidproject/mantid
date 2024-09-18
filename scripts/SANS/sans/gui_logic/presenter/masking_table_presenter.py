# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""  The presenter associated with the masking table view. """

import copy
from collections import namedtuple

from ui.sans_isis.masking_table import MaskingTable

from mantid.api import AnalysisDataService
from mantid.kernel import Logger
from sans.common.enums import DetectorType
from sans.gui_logic.models.async_workers.masking_table_async import MaskingTableAsync
from mantidqt.widgets.instrumentview.presenter import InstrumentViewPresenter

masking_information = namedtuple("masking_information", "first, second, third")


class MaskingTablePresenter(object):
    DISPLAY_WORKSPACE_NAME = "__sans_mask_display_dummy_workspace"

    class ConcreteMaskingTableListener(MaskingTable.MaskingTableListener):
        def __init__(self, presenter):
            super(MaskingTablePresenter.ConcreteMaskingTableListener, self).__init__()
            self._presenter = presenter

        def on_row_changed(self):
            pass

        def on_update_rows(self):
            self._presenter.on_update_rows()

        def on_display(self):
            self._presenter.on_display()

    def __init__(self, parent_presenter):
        self._view = None
        self._parent_presenter = parent_presenter
        self._worker = MaskingTableAsync(parent_presenter=self)
        self._logger = Logger("SANS")

    def on_display(self):
        # Get the state information for the selected row.
        # Disable the button
        self._view.set_display_mask_button_to_processing()
        try:
            row_index = self._view.get_current_row()
            state = self.get_state(row_index)
        except Exception as e:
            self.on_processing_error_masking_display(e)
            return

        if not state:
            self._logger.error(
                "You can only show a masked workspace if a user file has been loaded and a"
                "valid sample scatter entry has been provided in the selected row."
            )
            self._view.set_display_mask_button_to_normal()
            return

        # Run the task
        self.display_masking_information(state)
        state_copy = copy.copy(state)
        self._worker.load_and_mask_workspace(state_copy, self.DISPLAY_WORKSPACE_NAME)

    def on_processing_successful_masking_display(self, result):
        # Display masked workspace
        self._display(result)

    def on_processing_finished_masking_display(self):
        # Enable button
        self._view.set_display_mask_button_to_normal()

    def on_processing_error_masking_display(self, error):
        self._logger.error("There has been an error. See more: {}".format(error))
        # Enable button
        self._view.set_display_mask_button_to_normal()

    def on_processing_error(self, error):
        pass

    def on_update_rows(self):
        """
        Update the row selection in the combobox
        """
        current_row_index = self._view.get_current_row()
        valid_row_indices = self._parent_presenter.get_row_indices()

        new_row_index = -1
        if current_row_index in valid_row_indices:
            new_row_index = current_row_index
        elif len(valid_row_indices) > 0:
            new_row_index = valid_row_indices[0]

        self._view.update_rows(valid_row_indices)

        if new_row_index != -1:
            self.set_row(new_row_index)

    def set_row(self, index):
        self._view.set_row(index)

    def set_view(self, view):
        if view:
            self._view = view

            # Set up row selection listener
            listener = MaskingTablePresenter.ConcreteMaskingTableListener(self)
            self._view.add_listener(listener)

            # Set the default gui
            self._set_default_gui()

    def _set_default_gui(self):
        self._view.update_rows([])
        self.display_masking_information(state=None)

    def get_state(self, index, file_lookup=True, suppress_warnings=False):
        return self._parent_presenter.get_state_for_row(index, file_lookup=file_lookup, suppress_warnings=suppress_warnings)

    @staticmethod
    def _append_single_spectrum_mask(spectrum_mask, container, detector_name, prefix):
        if spectrum_mask:
            for item in spectrum_mask:
                detail = prefix + str(item)
                container.append(masking_information(first="Spectrum", second=detector_name, third=detail))

    @staticmethod
    def _append_strip_spectrum_mask(strip_mask_start, strip_mask_stop, container, detector_name, prefix):
        if strip_mask_start and strip_mask_stop:
            for start, stop in zip(strip_mask_start, strip_mask_stop):
                detail = prefix + str(start) + ">" + prefix + str(stop)
                container.append(masking_information(first="Strip", second=detector_name, third=detail))

    @staticmethod
    def _append_block_spectrum_mask(
        horizontal_mask_start, horizontal_mask_stop, vertical_mask_start, vertical_mask_stop, container, detector_name
    ):
        if horizontal_mask_start and horizontal_mask_stop and vertical_mask_start and vertical_mask_stop:
            for h_start, h_stop, v_start, v_stop in zip(
                horizontal_mask_start, horizontal_mask_stop, vertical_mask_start, vertical_mask_stop
            ):
                detail = "H{}>H{}+V{}>V{}".format(h_start, h_stop, v_start, v_stop)
                container.append(masking_information(first="Strip", second=detector_name, third=detail))

    @staticmethod
    def _append_spectrum_block_cross_mask(horizontal_mask, vertical_mask, container, detector_name):
        if horizontal_mask and vertical_mask:
            for h, v in zip(horizontal_mask, vertical_mask):
                detail = "H{}+V{}".format(h, v)
                container.append(masking_information(first="Strip", second=detector_name, third=detail))

    @staticmethod
    def _get_spectrum_masks(mask_detector_info):
        detector_name = mask_detector_info.detector_name
        spectrum_masks = []

        # -------------------------------
        # Get the vertical spectrum masks
        # -------------------------------
        single_vertical_strip_mask = mask_detector_info.single_vertical_strip_mask
        MaskingTablePresenter._append_single_spectrum_mask(single_vertical_strip_mask, spectrum_masks, detector_name, "V")

        range_vertical_strip_start = mask_detector_info.range_vertical_strip_start
        range_vertical_strip_stop = mask_detector_info.range_vertical_strip_stop
        MaskingTablePresenter._append_strip_spectrum_mask(
            range_vertical_strip_start, range_vertical_strip_stop, spectrum_masks, detector_name, "V"
        )

        # ---------------------------------
        # Get the horizontal spectrum masks
        # ---------------------------------
        single_horizontal_strip_mask = mask_detector_info.single_horizontal_strip_mask
        MaskingTablePresenter._append_single_spectrum_mask(single_horizontal_strip_mask, spectrum_masks, detector_name, "H")

        range_horizontal_strip_start = mask_detector_info.range_horizontal_strip_start
        range_horizontal_strip_stop = mask_detector_info.range_horizontal_strip_stop
        MaskingTablePresenter._append_strip_spectrum_mask(
            range_horizontal_strip_start, range_horizontal_strip_stop, spectrum_masks, detector_name, "H"
        )

        # ---------------------------------
        # Get the block masks
        # ---------------------------------
        block_horizontal_start = mask_detector_info.block_horizontal_start
        block_horizontal_stop = mask_detector_info.block_horizontal_stop
        block_vertical_start = mask_detector_info.block_vertical_start
        block_vertical_stop = mask_detector_info.block_vertical_stop
        MaskingTablePresenter._append_block_spectrum_mask(
            block_horizontal_start, block_horizontal_stop, block_vertical_start, block_vertical_stop, spectrum_masks, detector_name
        )

        block_cross_horizontal = mask_detector_info.block_cross_horizontal
        block_cross_vertical = mask_detector_info.block_cross_vertical
        MaskingTablePresenter._append_spectrum_block_cross_mask(block_cross_horizontal, block_cross_vertical, spectrum_masks, detector_name)

        # ---------------------------------
        # Get spectrum masks
        # ---------------------------------
        single_spectra = mask_detector_info.single_spectra
        MaskingTablePresenter._append_single_spectrum_mask(single_spectra, spectrum_masks, detector_name, "S")

        spectrum_range_start = mask_detector_info.spectrum_range_start
        spectrum_range_stop = mask_detector_info.spectrum_range_stop
        MaskingTablePresenter._append_strip_spectrum_mask(spectrum_range_start, spectrum_range_stop, spectrum_masks, detector_name, "S")

        return spectrum_masks

    @staticmethod
    def _get_time_masks_general(mask_info):
        container = []
        bin_mask_general_start = mask_info.bin_mask_general_start
        bin_mask_general_stop = mask_info.bin_mask_general_stop
        if bin_mask_general_start and bin_mask_general_stop:
            for start, stop in zip(bin_mask_general_start, bin_mask_general_stop):
                detail = "{}-{}".format(start, stop)
                container.append(masking_information(first="Time", second="", third=detail))
        return container

    @staticmethod
    def _get_time_masks(mask_info):
        container = []
        bin_mask_start = mask_info.bin_mask_start
        bin_mask_stop = mask_info.bin_mask_stop
        detector_name = mask_info.detector_name
        if bin_mask_start and bin_mask_stop:
            for start, stop in zip(bin_mask_start, bin_mask_stop):
                detail = "{}-{}".format(start, stop)
                container.append(masking_information(first="Time", second=detector_name, third=detail))
        return container

    @staticmethod
    def _get_arm_mask(mask_info):
        container = []
        beam_stop_arm_width = mask_info.beam_stop_arm_width
        beam_stop_arm_angle = mask_info.beam_stop_arm_angle
        beam_stop_arm_pos1 = mask_info.beam_stop_arm_pos1 if mask_info.beam_stop_arm_pos1 else 0.0
        beam_stop_arm_pos2 = mask_info.beam_stop_arm_pos2 if mask_info.beam_stop_arm_pos2 else 0.0
        if beam_stop_arm_width and beam_stop_arm_angle:
            detail = "LINE {}, {}, {}, {}".format(beam_stop_arm_width, beam_stop_arm_angle, beam_stop_arm_pos1, beam_stop_arm_pos2)
            container.append(masking_information(first="Arm", second="", third=detail))
        return container

    @staticmethod
    def _get_phi_mask(mask_info):
        container = []
        phi_min = mask_info.phi_min
        phi_max = mask_info.phi_max
        use_mask_phi_mirror = mask_info.use_mask_phi_mirror
        if phi_min and phi_max:
            if use_mask_phi_mirror:
                detail = "L/PHI {} {}".format(phi_min, phi_max)
            else:
                detail = "L/PHI/NOMIRROR{} {}".format(phi_min, phi_max)
            container.append(masking_information(first="Phi", second="", third=detail))
        return container

    @staticmethod
    def _get_mask_files(mask_info):
        container = []
        mask_files = mask_info.mask_files
        if mask_files:
            for mask_file in mask_files:
                container.append(masking_information(first="Mask file", second="", third=mask_file))
        return container

    @staticmethod
    def _get_radius(mask_info):
        container = []
        radius_min = mask_info.radius_min
        radius_max = mask_info.radius_max

        if radius_min:
            detail = "infinite-cylinder, r = {}".format(radius_min)
            container.append(masking_information(first="Beam stop", second="", third=detail))

        if radius_max:
            detail = "infinite-cylinder, r = {}".format(radius_max)
            container.append(masking_information(first="Corners", second="", third=detail))
        return container

    def _generate_masking_information(self, state):
        if state is None:
            return []
        mask_info = state.mask
        masks = []

        mask_info_lab = mask_info.detectors[DetectorType.LAB.value]
        mask_info_hab = mask_info.detectors[DetectorType.HAB.value] if DetectorType.HAB.value in mask_info.detectors else None

        # Add the radius mask
        radius_mask = self._get_radius(mask_info)
        masks.extend(radius_mask)

        # Add the spectrum masks for LAB
        spectrum_masks_lab = self._get_spectrum_masks(mask_info_lab)
        masks.extend(spectrum_masks_lab)

        # Add the spectrum masks for HAB
        if mask_info_hab:
            spectrum_masks_hab = self._get_spectrum_masks(mask_info_hab)
            masks.extend(spectrum_masks_hab)

        # Add the general time mask
        time_masks_general = self._get_time_masks_general(mask_info)
        masks.extend(time_masks_general)

        # Add the time masks for LAB
        time_masks_lab = self._get_time_masks(mask_info_lab)
        masks.extend(time_masks_lab)

        # Add the time masks for HAB
        if mask_info_hab:
            time_masks_hab = self._get_time_masks(mask_info_hab)
            masks.extend(time_masks_hab)

        # Add arm mask
        arm_mask = self._get_arm_mask(mask_info)
        masks.extend(arm_mask)

        # Add phi mask
        phi_mask = self._get_phi_mask(mask_info)
        masks.extend(phi_mask)

        # Add mask files
        mask_files = self._get_mask_files(mask_info)
        masks.extend(mask_files)
        return masks

    def get_masking_information(self, state):
        table_entries = []
        if state is not None:
            table_entries = self._generate_masking_information(state)
        return table_entries

    def display_masking_information(self, state):
        table_entries = self.get_masking_information(state)
        self._view.set_table(table_entries)

    @staticmethod
    def _display(masked_workspace):
        if masked_workspace and AnalysisDataService.doesExist(masked_workspace.name()):
            instrument_win = InstrumentViewPresenter(masked_workspace)
            instrument_win.container.show()
