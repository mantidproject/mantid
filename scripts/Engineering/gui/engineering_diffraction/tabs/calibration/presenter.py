# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from mantidqt.utils.asynchronous import AsyncTask
from mantid.simpleapi import logger


class CalibrationPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.view.set_on_calibrate_clicked(self.on_calibrate_clicked)
        self.worker = None

    def on_calibrate_clicked(self):
        vanadium_no = self.view.get_vanadium_filename()
        calib_no = self.view.get_calib_filename()
        plot_output = self.view.get_plot_output()
        self.start_calibration_worker(vanadium_no, calib_no, plot_output)

    def start_calibration_worker(self, vanadium_no, calib_no, plot_output):
        self.worker = AsyncTask(self.model.create_new_calibration, (vanadium_no, calib_no),
                                {"plot_output": plot_output},
                                error_cb=self._on_error)
        self.worker.start()

    @staticmethod
    def _on_error(failure_info):
        logger.warning(str(failure_info))
