# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import copy

from mantid.kernel import Logger
from ui.sans_isis.beam_centre import BeamCentre
from ui.sans_isis.work_handler import WorkHandler


class BeamCentrePresenter(object):
    class ConcreteBeamCentreListener(BeamCentre.BeamCentreListener):
        def __init__(self, presenter):
            self._presenter = presenter

        def on_run_clicked(self):
            self._presenter.on_run_clicked()

    class CentreFinderListener(WorkHandler.WorkListener):
        def __init__(self, presenter):
            super(BeamCentrePresenter.CentreFinderListener, self).__init__()
            self._presenter = presenter

        def on_processing_finished(self, result):
            self._presenter.on_processing_finished_centre_finder(result)

        def on_processing_error(self, error):
            self._presenter.on_processing_error_centre_finder(error)

    def __init__(self, parent_presenter, WorkHandler, BeamCentreModel, SANSCentreFinder):
        self._view = None
        self._parent_presenter = parent_presenter
        self._work_handler = WorkHandler()
        self._logger = Logger("SANS")
        self._beam_centre_model = BeamCentreModel(SANSCentreFinder)

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
        self._beam_centre_model.set_scaling(instrument)
        self._view.on_update_instrument(instrument)

    def on_update_rows(self):
        file_information = self._parent_presenter._table_model.get_file_information_for_row(0)
        if file_information:
            self._beam_centre_model.reset_to_defaults_for_instrument(file_information=file_information)
        self._view.set_options(self._beam_centre_model)

    def on_processing_finished_centre_finder(self, result):
        # Enable button
        self._view.set_run_button_to_normal()
        # Update Centre Positions in model and GUI
        if self._beam_centre_model.update_lab:
            self._beam_centre_model.lab_pos_1 = result['pos1']
            self._beam_centre_model.lab_pos_2 = result['pos2']
            self._view.lab_pos_1 = self._beam_centre_model.lab_pos_1 * self._beam_centre_model.scale_1
            self._view.lab_pos_2 = self._beam_centre_model.lab_pos_2 * self._beam_centre_model.scale_2
        if self._beam_centre_model.update_hab:
            self._beam_centre_model.hab_pos_1 = result['pos1']
            self._beam_centre_model.hab_pos_2 = result['pos2']
            self._view.hab_pos_1 = self._beam_centre_model.hab_pos_1 * self._beam_centre_model.scale_1
            self._view.hab_pos_2 = self._beam_centre_model.hab_pos_2 * self._beam_centre_model.scale_2

    def on_processing_error_centre_finder(self, error):
        self._logger.warning("There has been an error. See more: {}".format(error))
        self._view.set_run_button_to_normal()

    def on_processing_error(self, error):
        self._view.set_run_button_to_normal()

    def on_run_clicked(self):
        # Get the state information for the first row.
        state = self._parent_presenter.get_state_for_row(0)

        if not state:
            self._logger.information("You can only calculate the beam centre if a user file has been loaded and there"
                                     "valid sample scatter entry has been provided in the selected row.")
            return

        # Disable the button
        self._view.set_run_button_to_processing()

        #Update model
        self._update_beam_model_from_view()

        # Run the task
        listener = BeamCentrePresenter.CentreFinderListener(self)
        state_copy = copy.copy(state)

        self._work_handler.process(listener, self._beam_centre_model.find_beam_centre, 0, state_copy)

    def _update_beam_model_from_view(self):
        self._beam_centre_model.r_min = self._view.r_min
        self._beam_centre_model.r_max = self._view.r_max
        self._beam_centre_model.max_iterations = self._view.max_iterations
        self._beam_centre_model.tolerance = self._view.tolerance
        self._beam_centre_model.left_right = self._view.left_right
        self._beam_centre_model.verbose = self._view.verbose
        self._beam_centre_model.COM = self._view.COM
        self._beam_centre_model.up_down = self._view.up_down
        self._beam_centre_model.lab_pos_1 = self._view.lab_pos_1 / self._beam_centre_model.scale_1
        self._beam_centre_model.lab_pos_2 = self._view.lab_pos_2 / self._beam_centre_model.scale_2
        self._beam_centre_model.hab_pos_1 = self._view.hab_pos_1 / self._beam_centre_model.scale_1
        self._beam_centre_model.hab_pos_2 = self._view.hab_pos_2 / self._beam_centre_model.scale_2
        self._beam_centre_model.q_min = self._view.q_min
        self._beam_centre_model.q_max = self._view.q_max
        self._beam_centre_model.component = self._view.component
        self._beam_centre_model.update_hab = self._view.update_hab
        self._beam_centre_model.update_lab = self._view.update_lab

    def update_centre_positions(self, state_model):
        lab_pos_1 = getattr(state_model, 'lab_pos_1')
        lab_pos_2 = getattr(state_model, 'lab_pos_2')

        hab_pos_1 = getattr(state_model, 'hab_pos_1') if getattr(state_model, 'hab_pos_1') else lab_pos_1
        hab_pos_2 = getattr(state_model, 'hab_pos_2') if getattr(state_model, 'hab_pos_2') else lab_pos_2

        self._view.lab_pos_1 = lab_pos_1
        self._view.lab_pos_2 = lab_pos_2

        self._view.hab_pos_1 = hab_pos_1
        self._view.hab_pos_2 = hab_pos_2

    def update_hab_selected(self):
        self._beam_centre_model.update_hab = True
        self._beam_centre_model.update_lab = False

        # HAB is selected, so ensure update HAB is enabled and checked
        self._view.enable_update_hab(True)
        # Disable and deselect update LAB
        self._view.enable_update_lab(False)

    def update_lab_selected(self):
        self._beam_centre_model.update_hab = False
        self._beam_centre_model.update_lab = True

        # LAB is selected, so ensure update LAB is enabled and checked
        self._view.enable_update_lab(True)
        # Disable and deselect update HAB
        self._view.enable_update_hab(False)

    def update_all_selected(self):
        self._beam_centre_model.update_hab = True
        self._beam_centre_model.update_lab = True

        self._view.enable_update_hab(True)
        self._view.enable_update_lab(True)

    def set_on_state_model(self, attribute_name, state_model):
        attribute = getattr(self._view, attribute_name)
        if attribute or isinstance(attribute, bool):
            setattr(state_model, attribute_name, attribute)

    def set_on_view(self, attribute_name, state_model):
        attribute = getattr(state_model, attribute_name)
        if attribute or isinstance(attribute, bool):  # We need to be careful here. We don't want to set empty strings, or None, but we want to set boolean values. # noqa
            setattr(self._view, attribute_name, attribute)

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
            if min_value >= max_value:
                if self._view.run_button.isEnabled():
                    # Only post to logger once per disabling
                    self._logger.notice("Minimum radius is larger than maximum radius. "
                                        "Cannot find beam centre with current settings.")
                    self._view.run_button.setEnabled(False)
            else:
                self._view.run_button.setEnabled(True)
