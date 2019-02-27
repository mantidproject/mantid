# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import os

from mantid.kernel import ConfigService
from sans.common.enums import SANSInstrument
from sans.gui_logic.gui_common import GENERIC_SETTINGS, load_property, SANSGuiPropertiesHandler, set_setting
from sans.gui_logic.models.run_selection import has_any_event_data


class AddRunsFilenameManager(object):
    def __init__(self, inst):
        if isinstance(inst, str):
            self.instrument_string = inst
        else:
            self.instrument_string = SANSInstrument.to_string(inst)

    def make_filename(self, runs):
        if runs:
            full_run_name = self._all_runs_are_ints(runs)
            if full_run_name:
                # If we have a full name defined, use it
                return full_run_name + "-add"
            max_run = self._select_max_run(runs)
            zeroes = self._get_leading_zeroes(max_run)
            return self.instrument_string + zeroes + max_run + "-add"
        return ""

    @staticmethod
    def _all_runs_are_ints(runs):
        # May not just pass in runs to sum runs, but full name
        # e.g. LOQ74044. If this is present, this name should be taken
        for run in runs:
            try:
                int(run)
            except ValueError:
                return run
        return None

    @staticmethod
    def _select_max_run(list_of_runs):
        return str(max(map(int, list_of_runs)))

    def _get_leading_zeroes(self, run_number):
        run_number_int = int(run_number)
        total_digits_for_inst = ConfigService.getInstrument(self.instrument_string).zeroPadding(run_number_int)
        zeros_to_add = total_digits_for_inst - len(run_number)
        return zeros_to_add * "0"


class AddRunsPagePresenter(object):
    def __init__(self,
                 sum_runs,
                 make_run_selector_presenter,
                 make_run_summation_presenter,
                 view,
                 parent_view):

        self._view = view
        self._parent_view = parent_view
        self._sum_runs = sum_runs
        self._use_generated_file_name = True
        self._run_selector_presenter = \
            make_run_selector_presenter(view.run_selector_view(),
                                        self._handle_selection_changed, view)
        self._summation_settings_presenter = \
            make_run_summation_presenter(view.summation_settings_view(),
                                         view, ConfigService.Instance().getString("default.instrument"))

        self.save_directory = ""
        self._connect_to_view(view)

        self.gui_properties_handler = SANSGuiPropertiesHandler({"add_runs_output_directory": (self.set_output_directory,
                                                                                            str)})

    def _get_filename_manager(self):
        # Separate call so AddRunsFilesnameManager can be mocked out.
        return AddRunsFilenameManager(self._parent_view.instrument)

    def _init_views(self, view, parent_view):
        self._view = view
        self._parent = parent_view

    def _connect_to_view(self, view):
        view.sum.connect(self._handle_sum)
        view.outFileChanged.connect(self._handle_out_file_changed)
        view.saveDirectoryClicked.connect(self._handle_output_directory_changed)

    def _make_base_file_name_from_selection(self, run_selection):
        filename_manager = self._get_filename_manager()
        names = [run.display_name() for run in run_selection]

        return filename_manager.make_filename(names)

    def _sum_base_file_name(self, run_selection):
        if self._use_generated_file_name:
            return self._generated_output_file_name
        else:
            return self._view.out_file_name()

    def _refresh_view(self, run_selection):
        self._update_output_filename(run_selection)
        self._update_histogram_binning(run_selection)
        if run_selection.has_any_runs():
            self._view.enable_sum()
        else:
            self._view.disable_sum()

    def _update_output_filename(self, run_selection):
        self._generated_output_file_name = self._make_base_file_name_from_selection(run_selection)
        if self._use_generated_file_name:
            self._view.set_out_file_name(self._generated_output_file_name)

    def _update_histogram_binning(self, run_selection):
        self._view.enable_summation_settings()

    def _handle_selection_changed(self, run_selection):
        self._refresh_view(run_selection)

    def _handle_out_file_changed(self):
        self._use_generated_file_name = False

    @staticmethod
    def _output_directory_is_not_empty(settings):
        return settings.save_directory != ''

    def _handle_output_directory_changed(self):
        directory = self._view.display_save_directory_box("Save sum runs", self.save_directory)
        directory = os.path.join(directory, '')  # Add an OS specific trailing slash if it doesn't already exist
        self.gui_properties_handler.update_default("add_runs_output_directory", directory)
        self.set_output_directory(directory)

    def _handle_sum(self):
        run_selection = self._run_selector_presenter.run_selection()
        settings = self._summation_settings_presenter.settings()
        settings.save_directory = self.save_directory

        if self._output_directory_is_not_empty(settings):
            self._view.disable_sum()
            self._sum_runs(run_selection,
                           settings,
                           self._sum_base_file_name(run_selection))
        else:
            self._view.no_save_directory()

    def set_output_directory(self, directory):
        if not directory:
            directory = ConfigService.Instance().getString("defaultsave.directory")
        self.save_directory = directory
        self._view.set_out_file_directory(directory)
