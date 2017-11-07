from __future__ import (absolute_import, division, print_function)

from collections import namedtuple
import copy

from mantid.kernel import Logger
from mantid.api import (AnalysisDataService)
from sans.sans_batch import SANSCentreFinder
try:
    import mantidplot
except ImportError:
    pass

from ui.sans_isis.beam_centre import BeamCentre
from sans.common.enums import DetectorType
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import create_unmanaged_algorithm
from ui.sans_isis.work_handler import WorkHandler


class BeamCentrePresenter(object):
    class ConcreteBeamCentreListener(BeamCentre.BeamCentreListener):
        def __init__(self, presenter):
            super(BeamCentrePresenter.ConcreteBeamCentreListener, self).__init__()
            self._presenter = presenter

        def on_run_clicked(self):
            self._presenter.on_run_clicked()

        def on_clear_log_clicked(self):
            self._presenter.on_clear_log_clicked()

        def on_row_changed(self):
            self._presenter.on_row_changed()

        def on_update_rows(self):
            self._presenter.on_update_rows()

    class DisplayMaskListener(WorkHandler.WorkListener):
        def __init__(self, presenter):
            super(BeamCentrePresenter.DisplayMaskListener, self).__init__()
            self._presenter = presenter

        def on_processing_finished(self, result):
            self._presenter.on_processing_finished_centre_finder(result)

        def on_processing_error(self, error):
            self._presenter.on_processing_error_centre_finder(error)

    def __init__(self, parent_presenter):
        super(BeamCentrePresenter, self).__init__()
        self._view = None
        self._parent_presenter = parent_presenter
        self._work_handler = WorkHandler()
        self._logger = Logger("SANS")

    def on_row_changed(self):
        row_index = self._view.get_current_row()
        state = self.get_state(row_index)
        if state:
            self.display_centre_information(state)

    def display_centre_information(self, state):
        centre_information = self._get_centre_information(state)
        self._view.set_centre_positions(centre_information)

    def _get_centre_information(self, state):
        centre_information = {}
        if state is not None:
            centre_information = self._generate_centre_information(state)
        return centre_information

    def _generate_centre_information(self, state):
        if state is None:
            return {}
        detector_info = state.move.detectors
        centre_position = {}

        centre_position.update({'LAB1': detector_info['LAB'].sample_centre_pos1})
        centre_position.update({'LAB2': detector_info['LAB'].sample_centre_pos2})
        centre_position.update({'HAB1': detector_info['HAB'].sample_centre_pos1})
        centre_position.update({'HAB2': detector_info['HAB'].sample_centre_pos2})

        return centre_position

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
            self.on_row_changed()

    def on_processing_finished_centre_finder(self, result):
        # Enable button
        self._view.set_run_button_to_normal()

    def on_processing_error_centre_finder(self, error):
        self._logger.warning("There has been an error. See more: {}".format(error))

    def on_processing_error(self, error):
        pass

    def set_row(self, index):
        self._view.set_row(index)

    def set_view(self, view):
        if view:
            self._view = view

            # Set up row selection listener
            listener = BeamCentrePresenter.ConcreteBeamCentreListener(self)
            self._view.add_listener(listener)

            # Set the default gui
            self._set_default_gui()

    def _set_default_gui(self):
        self._view.update_rows([])

    def get_state(self, index):
        return self._parent_presenter.get_state_for_row(index)

    def on_run_clicked(self):
        # Get the state information for the selected row.
        row_index = self._view.get_current_row()
        state = self.get_state(row_index)

        if not state:
            self._logger.information("You can only calculate the beam centre if a user file has been loaded and there"
                                     "valid sample scatter entry has been provided in the selected row.")
            return

        # Disable the button
        self._view.set_run_button_to_processing()

        # Run the task
        listener = BeamCentrePresenter.CentreFinderListener(self)
        state_copy = copy.copy(state)
        self._work_handler.process(listener, find_beam_centre, state_copy)

def find_beam_centre(self, state):
    centre_finder = SANSCentreFinder()
    centre = centre_finder(state, rlow, rupp, MaxIter, xstart, ystart, tolerance, find_direction, reduction_method=True)




