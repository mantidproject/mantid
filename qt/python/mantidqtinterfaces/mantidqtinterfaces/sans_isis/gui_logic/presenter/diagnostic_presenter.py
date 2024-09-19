# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import Logger
from mantid import UsageService
from mantid.kernel import FeatureType
from mantidqtinterfaces.sans_isis.views.diagnostics_page import DiagnosticsPage
from SANS.sans.common.enums import IntegralEnum
from mantidqtinterfaces.sans_isis.gui_logic.gui_common import get_detector_strings_for_diagnostic_page, get_detector_from_gui_selection
from mantidqtinterfaces.sans_isis.gui_logic.models.async_workers.diagnostic_async import DiagnosticsAsync
from mantidqtinterfaces.sans_isis.gui_logic.models.diagnostics_model import DiagnosticsModel


class DiagnosticsPagePresenter:
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

    def __init__(self, parent_presenter, facility):
        self._view = None
        self._facility = facility
        self._parent_presenter = parent_presenter
        self._logger = Logger("SANS")
        self._model = DiagnosticsModel()
        self._worker = DiagnosticsAsync(parent_presenter=self)

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
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Diagnostics - Horizontal"], False)
        self._view.disable_integrals()
        input_file = self._view.run_input
        period = self._view.period
        state_model_with_view_update = self._parent_presenter.update_model_from_view()
        state = self._model.create_state(state_model_with_view_update, input_file, period, self._facility)
        mask = self._view.horizontal_mask
        range = self._view.horizontal_range
        detector = get_detector_from_gui_selection(self._view.detector)
        self._worker.run_integral(range, mask, IntegralEnum.Horizontal, detector, state)

    def on_vertical_clicked(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Diagnostics - Vertical"], False)
        self._view.disable_integrals()
        input_file = self._view.run_input
        period = self._view.period
        state_model_with_view_update = self._parent_presenter.update_model_from_view()
        state = self._model.create_state(state_model_with_view_update, input_file, period, self._facility)
        mask = self._view.vertical_mask
        range = self._view.vertical_range
        detector = get_detector_from_gui_selection(self._view.detector)
        self._worker.run_integral(range, mask, IntegralEnum.Vertical, detector, state)

    def on_time_clicked(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Diagnostics - Time"], False)
        self._view.disable_integrals()
        input_file = self._view.run_input
        period = self._view.period
        state_model_with_view_update = self._parent_presenter.update_model_from_view()
        state = self._model.create_state(state_model_with_view_update, input_file, period, self._facility)
        mask = self._view.time_mask
        range = self._view.time_range

        detector = get_detector_from_gui_selection(self._view.detector)
        self._worker.run_integral(range, mask, IntegralEnum.Time, detector, state)

    def on_processing_finished(self):
        self._view.enable_integrals()

    def on_processing_success(self, output):
        # We don't do anything with this in production, but it is replaced with a mock by unit test code
        pass
