# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget


class HomeRunInfoWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view, model):
        self._view = view
        self._model = model

    def show(self):
        self._view.show()

    def update_view_from_model(self):
        self._view.clear()
        run = self._model.get_run_number()
        instrument = self._model.get_instrument_name()
        counts = self._model.get_total_counts()
        self._view.add_text_line("Instrument                : " + str(instrument))
        self._view.add_text_line("Run                       : " + run)
        self._view.add_text_line(self.create_text_line("Title                    ", "run_title"))
        self._view.add_text_line("Comment                   : " + str(self._model.get_workspace_comment()))
        self._view.add_text_line(self.create_text_line("Start                    ", "run_start"))
        self._view.add_text_line(self.create_text_line("End                      ", "run_end"))
        self._view.add_text_line("Counts (MEv)              : " + str(self._model.get_counts_in_MeV(counts)))
        self._view.add_text_line(self.create_text_line("Good Frames              ", "goodfrm"))
        self._view.add_text_line("Counts per Good Frame     : " + str(self._model.get_counts_per_good_frame(counts)))
        self._view.add_text_line("Counts per Good Frame per det : " +
                                 str(self._model.get_counts_per_good_frame_per_detector(counts)))
        self._view.add_text_line("Average Temperature (K)   : "+str(self._model.get_average_temperature()))
        self._view.add_text_line(self.create_text_line("Sample Temperature (K)   ", "sample_temp"))
        self._view.add_text_line(self.create_text_line("Sample Magnetic Field (G)", "sample_magn_field"))

    def create_text_line(self, name, log_name):
        log = self._model.get_log_value(log_name)
        text = str(name) + " : " + str(log)
        return text
