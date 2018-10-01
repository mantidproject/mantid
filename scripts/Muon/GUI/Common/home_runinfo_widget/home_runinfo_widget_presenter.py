"""
The run information box of the home tab of Muon Analysis 2.0. This file
contains the presenter class HomeRunInfoWidgetPresenter.
"""

from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget


class HomeRunInfoWidgetPresenter(HomeTabSubWidget):
    """
    Presenter class for the MVP run information widget, part of the home tab
    of Muon Analysis 2.0.
    """
    # Total number of characters in the "log_name    : " part of the text
    NAME_COL_WIDTH = 25

    @staticmethod
    def _num_to_n_sig_figs(number, sig_figs=3, scientific_format=False):
        """
        Return a string of number rounded to n significant figures,
        with the possibility of standard/scientific form.
        """
        number_string = ("{0:.%ie}" % (sig_figs - 1)).format(number)
        if scientific_format:
            return number_string
        else:
            return str(float(number_string))

    def __init__(self, view, model):
        self._view = view
        self._model = model

    def show(self):
        """Show the widget (PyQt)."""
        self._view.show()

    def update_view_from_model(self):
        """
        Clear the view and update it entirely from the state of the model.
        """
        self._view.clear()

        run = self._model.get_run_number()
        instrument = str(self._model.get_instrument_name())
        counts = str(self._num_to_n_sig_figs(self._model.get_counts_in_MeV(), 3))
        ave_temp = self._model.get_average_temperature()
        comment = str(self._model.get_workspace_comment())
        start_time = self._model.get_start_time()
        end_time = self._model.get_end_time()

        self._view.add_text_line(self.create_text_line("Instrument", instrument))
        self._view.add_text_line(self.create_text_line("Run", run))
        self._view.add_text_line(self.create_text_line_from_log("Title", "run_title"))
        self._view.add_text_line(self.create_text_line("Comment", comment))
        self._view.add_text_line(self.create_text_line("Start", start_time))
        self._view.add_text_line(self.create_text_line("End", end_time))
        self._view.add_text_line(self.create_text_line_from_log("Good Frames", "goodfrm"))
        self._view.add_text_line(self.create_text_line("Counts (MeV)", counts))
        self._view.add_text_line(self.create_text_line(
            "Average Temperature (K)", ave_temp, "{0:.1f}"))
        self._view.add_text_line(self.create_text_line_from_log(
            "Sample Temperature (K)", "sample_temp", "{0:.1f}"))
        self._view.add_text_line(self.create_text_line_from_log(
            "Sample Magnetic Field (G)", "sample_magn_field", "{0:.1f}"))

    def create_text_line_from_log(self, name, log_name, format_string="{0}"):
        """
        Create an individual line of text for the run information box, by extracting
        a run log from the workspace with.
        :param name: The name of the log to query the workspace for.
        :param value: (string) The value of the log to appear on the right side.
        :param format_string: A string to format the value, e.g. {0:.1d} for 1 d.p.
        :return: The text line as a str.
        """
        log_value = self._model.get_log_value(log_name)
        return self.create_text_line(name, log_value, format_string)

    def create_text_line(self, name, value, format_string="{0}"):
        """
        Create an individual line of text for the run information box.
        :param name: The name of the log (appears on the left)
        :param value: (string) The value of the log to appear on the right side.
        :param format_string: A string to format the value, e.g. {0:.1d} for 1 d.p.
        :return: The text line as a str.
        """
        name_section_format = "{0:" + str(self.NAME_COL_WIDTH) + "} : "
        name_section = name_section_format.format(name)
        if value:
            text = name_section + str.format(format_string, value)
        else:
            text = name_section + "Log not found"
        return text
