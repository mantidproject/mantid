# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os

from mantid.kernel import ConfigService
from enum import Enum
from sans.common.enums import BinningType
from sans.common.file_information import SANSFileInformationFactory
from sans.gui_logic.gui_common import SANSGuiPropertiesHandler
from sans.gui_logic.models.RunSelectionModel import RunSelectionModel
from sans.gui_logic.models.SummationSettingsModel import SummationSettingsModel
from sans.gui_logic.models.run_finder import SummableRunFinder
from sans.gui_logic.presenter.RunSelectorPresenter import RunSelectorPresenter
from sans.gui_logic.presenter.summation_settings_presenter import SummationSettingsPresenter

DEFAULT_BIN_SETTINGS = "5.5,45.5,50.0, 50.0,1000.0, 500.0,1500.0, 750.0,99750.0, 255.0,100005.0"


class AddRunsFilenameManager(object):
    def __init__(self, inst):
        if isinstance(inst, str):
            self.instrument_string = inst
        else:
            assert isinstance(inst, Enum)
            self.instrument_string = inst.value

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
    def __init__(self, sum_runs_model, view, parent_view):
        self._view = view
        self._parent_view = parent_view
        self._sum_runs_model = sum_runs_model
        self._use_generated_file_name = True
        self._view.disable_output_file_name_edit()

        self._init_sub_presenters(view)

        self.save_directory = ""
        self._connect_to_view(view)

        self.gui_properties_handler = SANSGuiPropertiesHandler({"add_runs_output_directory": (self.set_output_directory, str)})

    def _init_sub_presenters(self, view):
        self._run_selector_presenter = self._init_run_selector_presenter(view.run_selector_view(), self._handle_selection_changed, view)

        self._summation_settings_presenter = self._init_run_summations_settings_presenter(
            view.summation_settings_view(), view, ConfigService.Instance().getString("default.instrument")
        )

    @staticmethod
    def _init_run_selector_presenter(run_selector_view, on_selection_change, parent_view):
        title = "Runs To Sum"
        run_finder = SummableRunFinder(SANSFileInformationFactory())
        run_selection = RunSelectionModel(on_selection_change)
        return RunSelectorPresenter(title, run_selection, run_finder, run_selector_view, parent_view)

    @staticmethod
    def _init_run_summations_settings_presenter(summation_settings_view, parent_view, instrument_str):
        if instrument_str != "LOQ":
            binning_type = BinningType.SAVE_AS_EVENT_DATA
        else:
            binning_type = BinningType.CUSTOM
        summation_settings = SummationSettingsModel(binning_type)
        summation_settings.bin_settings = DEFAULT_BIN_SETTINGS
        return SummationSettingsPresenter(summation_settings, summation_settings_view, parent_view)

    def _get_filename_manager(self):
        # Separate call so AddRunsFilesnameManager can be mocked out.
        return AddRunsFilenameManager(self._parent_view.instrument)

    def _init_views(self, view, parent_view):
        self._view = view
        self._parent = parent_view

    def _connect_to_view(self, view):
        view.sum.connect(self._handle_sum)
        view.customOutFileChanged.connect(self._handle_custom_outfile_check_changed)
        view.saveDirectoryClicked.connect(self._handle_output_directory_changed)

    def _make_base_file_name_from_selection(self, run_selection):
        filename_manager = self._get_filename_manager()

        if not run_selection.has_any_runs():
            return ""

        names = [run.display_name() for run in run_selection]
        return filename_manager.make_filename(names)

    def _sum_base_file_name(self, run_selection):
        if self._use_generated_file_name:
            return self._make_base_file_name_from_selection(run_selection)
        else:
            return self._view.out_file_name()

    def _refresh_view(self, run_selection):
        self._update_output_filename(run_selection)
        self._update_histogram_binning()
        if run_selection.has_any_runs():
            self._view.enable_sum()
        else:
            self._view.disable_sum()

    def _update_output_filename(self, run_selection):
        self._generated_output_file_name = self._make_base_file_name_from_selection(run_selection)
        if self._use_generated_file_name:
            self._view.set_out_file_name(self._generated_output_file_name)

    def _update_histogram_binning(self):
        self._view.enable_summation_settings()

    def _handle_selection_changed(self, run_selection):
        self._refresh_view(run_selection)

    def _handle_custom_outfile_check_changed(self, enabled):
        if enabled:
            self._use_generated_file_name = False
            self._view.clear_output_file_name_edit()
            self._view.enable_output_file_name_edit()
            return
        self._use_generated_file_name = True
        self._view.disable_output_file_name_edit()
        self._handle_selection_changed(self._run_selector_presenter.run_selection())

    @staticmethod
    def _output_directory_is_not_empty(settings):
        return settings.save_directory

    def _handle_output_directory_changed(self):
        directory = self._view.display_save_directory_box("Save sum runs", self.save_directory)
        directory = os.path.join(directory, "")  # Add an OS specific trailing slash if it doesn't already exist
        self.handle_new_save_directory(directory)

    def handle_new_save_directory(self, directory):
        """
        This method is called when a new save directory is selected on the add runs page, but is also called
        in the run_tab_presenter when a new default save directory is selected through Manage Directories.
        :param directory: A string. The new path to the save directory
        """
        self.set_output_directory(directory)
        self.gui_properties_handler.set_setting("add_runs_output_directory", directory)

    def _handle_sum(self):
        run_selection = self._run_selector_presenter.run_selection()
        settings = self._summation_settings_presenter.settings()
        settings.save_directory = self.save_directory

        if self._output_directory_is_not_empty(settings):
            self._view.disable_sum()
            self._sum_runs_model(run_selection, settings, self._sum_base_file_name(run_selection))
        else:
            self._view.no_save_directory()

    def set_output_directory(self, directory):
        if not directory:
            directory = ConfigService.Instance().getString("defaultsave.directory")
        self.save_directory = directory
        self._view.set_out_file_directory(directory)
        return directory
