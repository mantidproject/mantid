from __future__ import (absolute_import, division, print_function)

import copy

from mantid.kernel import Logger
from ui.sans_isis.diagnostics_page import DiagnosticsPage
from ui.sans_isis.work_handler import WorkHandler
from sans.common.enums import IntegralEnum, DetectorType

class DiagnosticsPagePresenter(object):
    class ConcreteDiagnosticsPageListener(DiagnosticsPage.DiagnosticsPageListener):
        def __init__(self, presenter):
            self._presenter = presenter

        def on_browse_clicked(self):
            pass

        def on_det1_horizontal_clicked(self):
            self._presenter.on_det1_horizontal_clicked()

        def on_det1_vertical_clicked(self):
            self._presenter.on_det1_vertical_clicked()

        def on_det1_time_clicked(self):
            self._presenter.on_det1_time_clicked()

        def on_det2_horizontal_clicked(self):
            self._presenter.on_det2_horizontal_clicked()

        def on_det2_vertical_clicked(self):
            self._presenter.on_det2_vertical_clicked()

        def on_det2_time_clicked(self):
            self._presenter.on_det2_time_clicked()

    class IntegralListener(WorkHandler.WorkListener):
        def __init__(self, presenter):
            super(DiagnosticsPagePresenter.IntegralListener, self).__init__()
            self._presenter = presenter

        def on_processing_finished(self, result):
            self._presenter.on_processing_finished_integral(result)

        def on_processing_error(self, error):
            self._presenter.on_processing_error_integral(error)

    def __init__(self, parent_presenter, WorkHandler, run_integral):
        super(DiagnosticsPagePresenter, self).__init__()
        self._view = None
        self._parent_presenter = parent_presenter
        self._work_handler = WorkHandler()
        self.run_integral = run_integral
        self._logger = Logger("SANS")

    def set_view(self, view):
        if view:
            self._view = view

            # Set up run listener
            listener = DiagnosticsPagePresenter.ConcreteDiagnosticsPageListener(self)
            self._view.add_listener(listener)

    def on_det1_horizontal_clicked(self):
        file = self._view.run_input
        period = self._view.period
        mask = self._view.det1_horizontal_mask
        range = self._view.det1_horizontal_range
        listener = DiagnosticsPagePresenter.IntegralListener(self)
        self._work_handler.process(listener, self.run_integral, file, period, range, mask, IntegralEnum.Horizontal,
                                   DetectorType.LAB)

    def on_det2_horizontal_clicked(self):
        file = self._view.run_input
        period = self._view.period
        mask = self._view.det2_horizontal_mask
        range = self._view.det2_horizontal_range
        listener = DiagnosticsPagePresenter.IntegralListener(self)
        self._work_handler.process(listener, self.run_integral, file, period, range, mask, IntegralEnum.Horizontal,
                                   DetectorType.HAB)

    def on_det1_vertical_clicked(self):
        file = self._view.run_input
        period = self._view.period
        mask = self._view.det1_vertical_mask
        range = self._view.det1_vertical_range
        listener = DiagnosticsPagePresenter.IntegralListener(self)
        self._work_handler.process(listener, self.run_integral, file, period, range, mask, IntegralEnum.Vertical,
                                   DetectorType.LAB)

    def on_det2_vertical_clicked(self):
        file = self._view.run_input
        period = self._view.period
        mask = self._view.det2_vertical_mask
        range = self._view.det2_vertical_range
        listener = DiagnosticsPagePresenter.IntegralListener(self)
        self._work_handler.process(listener, self.run_integral, file, period, range, mask, IntegralEnum.Vertical,
                                   DetectorType.HAB)

    def on_det1_time_clicked(self):
        file = self._view.run_input
        period = self._view.period
        mask = self._view.det1_time_mask
        range = self._view.det1_time_range
        listener = DiagnosticsPagePresenter.IntegralListener(self)
        self._work_handler.process(listener, self.run_integral, file, period, range, mask, IntegralEnum.Time,
                                   DetectorType.LAB)

    def on_det2_time_clicked(self):
        file = self._view.run_input
        period = self._view.period
        mask = self._view.det2_time_mask
        range = self._view.det2_time_range
        listener = DiagnosticsPagePresenter.IntegralListener(self)
        self._work_handler.process(listener, self.run_integral, file, period, range, mask, IntegralEnum.Time,
                                   DetectorType.HAB)

    def on_processing_finished_integral(self, result):
        pass

    def on_processing_error_integral(self, error):
        pass
