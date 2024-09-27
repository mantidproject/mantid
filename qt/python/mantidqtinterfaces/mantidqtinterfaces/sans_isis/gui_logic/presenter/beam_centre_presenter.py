# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import copy
from typing import Dict

from sans_core.common.enums import DetectorType
from mantid import UsageService
from mantid.kernel import FeatureType
from mantid.kernel import Logger
from mantidqtinterfaces.sans_isis.gui_logic.models.async_workers.beam_centre_async import BeamCentreAsync
from mantidqtinterfaces.sans_isis.gui_logic.models.beam_centre_model import BeamCentreModel
from mantidqtinterfaces.sans_isis.views.beam_centre import BeamCentre


class BeamCentrePresenter(object):
    class ConcreteBeamCentreListener(BeamCentre.BeamCentreListener):
        def __init__(self, presenter):
            self._presenter = presenter

        def on_run_clicked(self):
            self._presenter.on_run_clicked()

    def __init__(self, parent_presenter, beam_centre_model=None):
        self._view = None
        self._parent_presenter = parent_presenter
        self._logger = Logger("SANS")
        self._beam_centre_model = BeamCentreModel() if not beam_centre_model else beam_centre_model
        self._worker = BeamCentreAsync(parent_presenter=self)

    def set_view(self, view):
        if view:
            self._view = view

            # Set up run listener
            listener = BeamCentrePresenter.ConcreteBeamCentreListener(self)
            self._view.add_listener(listener)

            # Set the default gui
            self._view.set_options(self._beam_centre_model)

            # Connect view signals
            self.connect_signals()

    def connect_signals(self):
        self._view.r_min_line_edit.textChanged.connect(self._validate_radius_values)
        self._view.r_max_line_edit.textChanged.connect(self._validate_radius_values)

    def on_update_instrument(self, instrument):
        self._view.on_update_instrument(instrument)

    def on_update_rows(self):
        self._beam_centre_model.reset_inst_defaults(self._parent_presenter.instrument)
        self.update_centre_positions()

    def on_update_centre_values(self, new_vals: Dict):
        self._beam_centre_model.update_centre_positions(new_vals)

    def on_processing_finished_centre_finder(self):
        # Enable button
        self._view.set_run_button_to_normal()
        # Update Centre Positions in model and GUI
        self._view.rear_pos_1 = self._round(self._beam_centre_model.rear_pos_1)
        self._view.rear_pos_2 = self._round(self._beam_centre_model.rear_pos_2)
        self._view.front_pos_1 = self._round(self._beam_centre_model.front_pos_1)
        self._view.front_pos_2 = self._round(self._beam_centre_model.front_pos_2)

    def on_processing_error_centre_finder(self, error):
        self._logger.warning("There has been an error. See more: {}".format(error))
        self._view.set_run_button_to_normal()

    def on_processing_finished(self):
        # Signal from run tab presenter
        self._view.set_run_button_to_normal()

    def on_processing_error(self, error):
        # Signal from run tab presenter
        self._view.set_run_button_to_normal()

    def on_run_clicked(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Beam Centre Finder - Run"], False)
        # Get the state information for the first row.
        state = self._parent_presenter.get_state_for_row(0)

        if not state:
            self._logger.information(
                "You can only calculate the beam centre if a user file has been loaded and there"
                "valid sample scatter entry has been provided in the selected row."
            )
            return

        # Disable the button
        self._view.set_run_button_to_processing()
        self._update_beam_model_from_view()

        # Run the task
        state_copy = copy.copy(state)
        self._worker.find_beam_centre(state_copy, self._beam_centre_model.pack_beam_centre_settings())

    def _update_beam_model_from_view(self):
        self._beam_centre_model.r_min = self._view.r_min
        self._beam_centre_model.r_max = self._view.r_max
        self._beam_centre_model.max_iterations = self._view.max_iterations
        self._beam_centre_model.tolerance = self._view.tolerance
        self._beam_centre_model.left_right = self._view.left_right
        self._beam_centre_model.verbose = self._view.verbose
        self._beam_centre_model.COM = self._view.COM
        self._beam_centre_model.up_down = self._view.up_down
        self._beam_centre_model.rear_pos_1 = self._view.rear_pos_1
        self._beam_centre_model.rear_pos_2 = self._view.rear_pos_2
        self._beam_centre_model.front_pos_1 = self._view.front_pos_1
        self._beam_centre_model.front_pos_2 = self._view.front_pos_2
        self._beam_centre_model.q_min = self._view.q_min
        self._beam_centre_model.q_max = self._view.q_max
        self._beam_centre_model.component = self._get_selected_component()

    def _get_selected_component(self):
        return DetectorType.HAB if self._view.update_front else DetectorType.LAB

    def copy_centre_positions(self, state_model):
        """
        Copies rear / front positions from an external model
        """
        self._beam_centre_model.rear_pos_1 = getattr(state_model, "rear_pos_1")
        self._beam_centre_model.rear_pos_2 = getattr(state_model, "rear_pos_2")

        self._beam_centre_model.front_pos_1 = getattr(state_model, "front_pos_1")
        self._beam_centre_model.front_pos_2 = getattr(state_model, "front_pos_2")

    def set_meters_mode_enabled(self, is_meters: bool) -> None:
        if is_meters:
            self._view.set_position_unit("m")
            return
        self._view.set_position_unit("mm")

    def update_centre_positions(self):
        rear_pos_1 = self._beam_centre_model.rear_pos_1
        rear_pos_2 = self._beam_centre_model.rear_pos_2

        front_pos_1 = self._beam_centre_model.front_pos_1
        front_pos_2 = self._beam_centre_model.front_pos_2

        self._view.rear_pos_1 = self._round(rear_pos_1)
        self._view.rear_pos_2 = self._round(rear_pos_2)

        self._view.front_pos_1 = self._round(front_pos_1)
        self._view.front_pos_2 = self._round(front_pos_2)

    def update_front_selected(self):
        self._beam_centre_model.update_front = True
        self._beam_centre_model.update_rear = False

        # front is selected, so ensure update front is enabled and checked
        self._view.enable_update_front(True)
        # Disable and deselect update rear
        self._view.enable_update_rear(False)

    def update_rear_selected(self):
        self._beam_centre_model.update_front = False
        self._beam_centre_model.update_rear = True

        # rear is selected, so ensure update rear is enabled and checked
        self._view.enable_update_rear(True)
        # Disable and deselect update front
        self._view.enable_update_front(False)

    def update_all_selected(self):
        self._beam_centre_model.update_front = True
        self._beam_centre_model.update_rear = True

        self._view.enable_update_front(True)
        self._view.enable_update_rear(True)

    def set_on_state_model(self, attribute_name, state_model):
        attribute = getattr(self._view, attribute_name)
        if attribute or isinstance(attribute, bool):
            setattr(state_model, attribute_name, attribute)

    def set_on_view(self, attribute_name, state_model):
        attribute = getattr(state_model, attribute_name)
        # We need to be careful here. We don't want to set empty strings, or None, but we want to set boolean values.
        if attribute or isinstance(attribute, bool):
            setattr(self._view, attribute_name, attribute)

    def _round(self, val):
        DECIMAL_PLACES_CENTRE_POS = 3
        try:
            val = float(val)
        except ValueError:
            return val
        return round(val, DECIMAL_PLACES_CENTRE_POS)

    def _validate_radius_values(self):
        min_value = getattr(self._view, "r_min_line_edit").text()
        max_value = getattr(self._view, "r_max_line_edit").text()

        try:
            min_value = float(min_value)
            max_value = float(max_value)
        except ValueError:
            # one of the values is empty
            pass
        else:
            if min_value == max_value == 0:
                self._view.run_button.setEnabled(False)
                return

            if min_value >= max_value:
                if self._view.run_button.isEnabled():
                    # Only post to logger once per disabling
                    self._logger.notice("Minimum radius is larger than maximum radius. " "Cannot find beam centre with current settings.")
                    self._view.run_button.setEnabled(False)
            else:
                self._view.run_button.setEnabled(True)
