# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.kernel import Logger
from ui.sans_isis.diagnostics_page import DiagnosticsPage
from ui.sans_isis.work_handler import WorkHandler
from sans.common.enums import IntegralEnum
from sans.gui_logic.gui_common import get_detector_strings_for_diagnostic_page, get_detector_from_gui_selection


class DiagnosticsPagePresenter(object):
    class ConcreteDiagnosticsPageListener(DiagnosticsPage.DiagnosticsPageListener):
        def __init__(self, presenter):
            self._presenter = presenter

        def on_browse_clicked(self):
            pass

        def on_horizontal_clicked(self):
            self._presenter.on_horizontal_clicked()

        def on_vertical_clicked(self):
            self._presenter.on_vertical_clicked()

        def on_time_clicked(self):
            self._presenter.on_time_clicked()

    class IntegralListener(WorkHandler.WorkListener):
        def __init__(self, presenter):
            super(DiagnosticsPagePresenter.IntegralListener, self).__init__()
            self._presenter = presenter

        def on_processing_finished(self, result):
            self._presenter.on_processing_finished_integral(result)

        def on_processing_error(self, error):
            self._presenter.on_processing_error_integral(error)

    def __init__(self, parent_presenter, WorkHandler, run_integral, create_state, facility):
        self._view = None
        self._facility = facility
        self._parent_presenter = parent_presenter
        self._work_handler = WorkHandler()
        self.run_integral = run_integral
        self._logger = Logger("SANS")
        self._create_state = create_state

    def set_view(self, view, instrument):
        if view:
            self._view = view

            # Set up run listener
            listener = DiagnosticsPagePresenter.ConcreteDiagnosticsPageListener(self)
            self._view.add_listener(listener)

            # Set up combo box
            self.set_instrument_settings(instrument)

    def set_instrument_settings(self, instrument=None):
        detector_list = get_detector_strings_for_diagnostic_page(instrument)
        self._view.set_detectors(detector_list)

    def on_user_file_load(self, user_file):
        self._view.user_file_name = user_file

    def on_horizontal_clicked(self):
        self._view.disable_integrals()
        input_file = self._view.run_input
        period = self._view.period
        state_model_with_view_update = self._parent_presenter._get_state_model_with_view_update()
        state = self._create_state(state_model_with_view_update, input_file, period, self._facility)
        mask = self._view.horizontal_mask
        range = self._view.horizontal_range
        listener = DiagnosticsPagePresenter.IntegralListener(self)
        detector = get_detector_from_gui_selection(self._view.detector)
        self._work_handler.process(listener, self.run_integral, 0, range, mask, IntegralEnum.Horizontal,
                                   detector, state)

    def on_vertical_clicked(self):
        self._view.disable_integrals()
        input_file = self._view.run_input
        period = self._view.period
        state_model_with_view_update = self._parent_presenter._get_state_model_with_view_update()
        state = self._create_state(state_model_with_view_update, input_file, period, self._facility)
        mask = self._view.vertical_mask
        range = self._view.vertical_range
        listener = DiagnosticsPagePresenter.IntegralListener(self)
        detector = get_detector_from_gui_selection(self._view.detector)
        self._work_handler.process(listener, self.run_integral, 0, range, mask, IntegralEnum.Vertical,
                                   detector, state)

    def on_time_clicked(self):
        self._view.disable_integrals()
        input_file = self._view.run_input
        period = self._view.period
        state_model_with_view_update = self._parent_presenter._get_state_model_with_view_update()
        state = self._create_state(state_model_with_view_update, input_file, period, self._facility)
        mask = self._view.time_mask
        range = self._view.time_range
        listener = DiagnosticsPagePresenter.IntegralListener(self)
        detector = get_detector_from_gui_selection(self._view.detector)
        self._work_handler.process(listener, self.run_integral, 0, range, mask, IntegralEnum.Time,
                                   detector, state)

    def on_processing_finished_integral(self, result):
        self._view.enable_integrals()

    def on_processing_error_integral(self, error):
        self._view.enable_integrals()
