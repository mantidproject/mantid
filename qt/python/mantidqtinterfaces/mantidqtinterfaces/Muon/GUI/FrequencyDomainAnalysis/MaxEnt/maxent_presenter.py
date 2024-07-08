# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import functools
import math
import re

import mantid.simpleapi as mantid

from mantidqtinterfaces.Muon.GUI.Common import thread_model
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (
    get_maxent_workspace_group_name,
    get_maxent_workspace_name,
    get_raw_data_workspace_name,
    RECONSTRUCTED_SPECTRA,
)
from mantidqt.utils.observer_pattern import GenericObserver, GenericObservable, Observable
from mantidqtinterfaces.Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from mantidqtinterfaces.Muon.GUI.Common.utilities.run_string_utils import run_list_to_string, run_string_to_list
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import run_MuonMaxent, create_empty_table

raw_data = "_raw_data"

PHASETABLE = "OutputPhaseTable"
DEADTIMES = "OutputDeadTimeTable"
SPECTRA = "ReconstructedSpectra"
PHASECONVERGENCE = "PhaseConvergenceTable"
GROUPINGTABLE = "GroupingTable"

USEGROUPS = "Groups"
USEDETECTORS = "All detectors"

optional_output_suffixes = {
    PHASETABLE: "_phase_table",
    DEADTIMES: "_dead_times",
    SPECTRA: RECONSTRUCTED_SPECTRA,
    PHASECONVERGENCE: "_phase_convergence",
}


class MaxEntPresenter(object):
    """
    This class links the MaxEnt model to the GUI
    """

    def __init__(self, view, context):
        self.view = view
        self.context = context
        self.thread = None
        self._optional_output_names = {}
        # set data
        self.getWorkspaceNames()
        self.view.set_methods([USEGROUPS, USEDETECTORS])
        # connect
        self.view.maxEntButtonSignal.connect(self.handleMaxEntButton)
        self.view.method_changed_slot(self.update_phase_table_options)
        self.view.method_changed_slot(self.notify_method_changed)
        self.view.period_changed_slot(self.notify_period_changed)
        self.view.cancelSignal.connect(self.cancel)
        self.view.run_changed_slot(self._load_periods)

        self.phase_table_observer = GenericObserver(self.update_phase_table_options)
        self.calculation_finished_notifier = GenericObservable()
        self.calculation_started_notifier = GenericObservable()
        self.new_phase_table = GenericObservable()
        self.update_phase_table_options()
        self.method_changed = Observable()
        self.period_changed = Observable()
        self.new_reconstructed_data = Observable()

    def notify_method_changed(self):
        self.method_changed.notify_subscribers(self.view.get_method)

    def notify_period_changed(self):
        self.period_changed.notify_subscribers(self.view.get_period)

    @property
    def use_groups(self):
        return self.view.get_method == USEGROUPS

    @property
    def widget(self):
        return self.view

    def _load_periods(self):
        run = run_string_to_list(self.view.get_run)
        periods = []
        if run != []:
            periods = [str(period + 1) for period in range(self.context.data_context.num_periods(run))]
        self.view.add_periods(periods)

    @property
    def get_selected_groups(self):
        selected_names = self.context.group_pair_context.selected_groups
        return [group for group in self.context.group_pair_context.groups if group.name in selected_names]

    @property
    def get_num_groups(self):
        return len(self.context.group_pair_context.selected_groups)

    def runChanged(self):
        self.getWorkspaceNames()

    def clear(self):
        self.view.addRuns([])
        self.view.update_phase_table_combo(["Construct"])

    # functions
    def getWorkspaceNames(self):
        # get runs
        run_list = [run_list_to_string(run) for run in self.context.data_context.current_runs]
        self.view.addRuns(run_list)
        # get periods
        self._load_periods()
        # get min number of points as a power of 2
        start = int(math.ceil(math.log(self.context.data_context.num_points) / math.log(2.0)))
        values = [str(2**k) for k in range(start, 21)]
        self.view.addNPoints(values)

    def cancel(self):
        if self.maxent_alg is not None:
            self.maxent_alg.cancel()

    # turn on button
    def activate(self):
        self.view.activateCalculateButton()

    # turn off button
    def deactivate(self):
        self.view.deactivateCalculateButton()

    def createThread(self):
        self.maxent_alg = mantid.AlgorithmManager.create("MuonMaxent")
        self._maxent_output_workspace_name = get_maxent_workspace_name(
            self.get_parameters_for_maxent_calculation()["InputWorkspace"], self.view.get_method
        )
        calculation_function = functools.partial(self.calculate_maxent, self.maxent_alg)
        self._maxent_calculation_model = ThreadModelWrapper(calculation_function)
        return thread_model.ThreadModel(self._maxent_calculation_model)

    # constructs the inputs for the MaxEnt algorithms
    # then executes them (see maxent_model to see the order
    # of execution
    def handleMaxEntButton(self):
        # put this on its own thread so not to freeze Mantid
        self.thread = self.createThread()
        self.thread.threadWrapperSetUp(self.deactivate, self.handleFinished, self.handle_error)
        self.calculation_started_notifier.notify_subscribers()
        self.thread.start()

    # kills the thread at end of execution
    def handleFinished(self):
        self.activate()
        self.calculation_finished_notifier.notify_subscribers(self._maxent_output_workspace_name)
        # if phase table is outputed
        if self.view.output_reconstructed_spectra and SPECTRA in self._optional_output_names.keys():
            ws = MuonWorkspaceWrapper(self._optional_output_names[SPECTRA]).workspace
            if self.use_groups:
                table = MuonWorkspaceWrapper(self._optional_output_names[GROUPINGTABLE]).workspace
                self.new_reconstructed_data.notify_subscribers({"ws": ws.name(), "table": table.name()})
            else:
                self.new_reconstructed_data.notify_subscribers({"ws": ws.name(), "table": None})

        if self.view.output_phase_table and PHASETABLE in self._optional_output_names.keys():
            name = self._optional_output_names[PHASETABLE]
            if self.use_groups:
                num_groups = self.get_num_groups
                self.context.frequency_context.add_group_phase_table(MuonWorkspaceWrapper(name), num_groups)
            else:
                self.context.phase_context.add_phase_table(MuonWorkspaceWrapper(name))
                self.new_phase_table.notify_subscribers()
            self.update_phase_table_options()
        # clear optional outputs
        self._optional_output_names = {}

    def handle_error(self, error):
        self.activate()
        self.view.warning_popup(error)

    def calculate_maxent(self, alg):
        maxent_parameters = self.get_parameters_for_maxent_calculation()
        base_name = get_maxent_workspace_name(maxent_parameters["InputWorkspace"], self.view.get_method)
        if self.use_groups and self.get_num_groups == 0:
            # this is caught as part of the calculation thread
            raise ValueError("Please select groups in the grouping tab")
        else:
            maxent_workspace = run_MuonMaxent(maxent_parameters, alg, base_name)
            self.add_maxent_workspace_to_ADS(maxent_parameters["InputWorkspace"], maxent_workspace, alg)

    def _create_group_table(self):
        tab = create_empty_table(GROUPINGTABLE)
        tab.addColumn("str", "Group")
        tab.addColumn("str", "Detectors")
        groups = self.get_selected_groups
        for group in groups:
            detectors = ""
            name = group.name
            for det in group.detectors:
                detectors += str(det) + ","
            tab.addRow([name, detectors[:-1]])
        return tab

    def get_parameters_for_maxent_calculation(self):
        inputs = {}
        run = self.view.get_run
        period = self.view.get_period
        multiperiod = True if self.view.num_periods > 1 else False
        name = get_raw_data_workspace_name(
            self.context.data_context.instrument, run, multiperiod, period=period, workspace_suffix=self.context.workspace_suffix
        )
        inputs["InputWorkspace"] = name

        if self.view.phase_table != "Construct" and self.view.phase_table != "None":
            inputs["InputPhaseTable"] = self.view.phase_table

        if self.use_groups:
            table = self._create_group_table()
            inputs["GroupTable"] = table

        dead_time_table_name = self.context.corrections_context.current_dead_time_table_name_for_run(
            self.context.data_context.instrument, [run]
        )
        if dead_time_table_name is not None:
            inputs["InputDeadTimeTable"] = dead_time_table_name

        run_float = [float(re.search("[0-9]+", name).group())]
        inputs["FirstGoodTime"] = self.context.first_good_data(run_float)

        inputs["LastGoodTime"] = self.context.last_good_data(run_float)

        inputs["Npts"] = self.view.num_points

        inputs["InnerIterations"] = self.view.inner_iterations

        inputs["OuterIterations"] = self.view.outer_iterations

        inputs["DoublePulse"] = self.view.double_pulse

        inputs["Factor"] = self.view.lagrange_multiplier

        inputs["MaxField"] = self.view.maximum_field

        inputs["DefaultLevel"] = self.view.maximum_entropy_constant

        inputs["FitDeadTime"] = self.view.fit_dead_times

        return inputs

    def update_phase_table_options(self):
        phase_table_list = []
        if self.use_groups:
            num_groups = self.get_num_groups
            phase_table_list = self.context.frequency_context.get_group_phase_tables(num_groups, self.context.data_context.instrument)
            phase_table_list.insert(0, "None")
        else:
            phase_table_list = self.context.phase_context.get_phase_table_list(self.context.data_context.instrument)
            phase_table_list.insert(0, "Construct")

        self.view.update_phase_table_combo(phase_table_list)

    def add_maxent_workspace_to_ADS(self, input_workspace, maxent_workspace, alg):
        run = re.search("[0-9]+", input_workspace).group()
        base_name = get_maxent_workspace_name(input_workspace, self.view.get_method)
        directory = get_maxent_workspace_group_name(base_name, self.context.data_context.instrument, self.context.workspace_suffix)

        muon_workspace_wrapper = MuonWorkspaceWrapper(directory + base_name)
        muon_workspace_wrapper.show()

        maxent_output_options = self.get_maxent_output_options()
        self.context.frequency_context.add_maxEnt(run, maxent_workspace)
        self.add_optional_outputs_to_ADS(alg, maxent_output_options, base_name, directory)

        # Storing this on the class so it can be sent as part of the calculation
        # finished signal.
        self._maxent_output_workspace_name = self.context._frequency_context.get_ws_name(base_name)

    def get_maxent_output_options(self):
        output_options = {}

        output_options[PHASETABLE] = self.view.output_phase_table
        output_options[DEADTIMES] = self.view.output_dead_times
        output_options[SPECTRA] = self.view.output_reconstructed_spectra
        output_options[PHASECONVERGENCE] = self.view.output_phase_convergence
        output_options[GROUPINGTABLE] = self.use_groups

        return output_options

    def add_optional_outputs_to_ADS(self, alg, output_options, base_name, directory):
        for key in output_options:
            if key == GROUPINGTABLE and output_options[key]:
                output = GROUPINGTABLE
                self.context.ads_observer.observeRename(False)
                wrapped_workspace = MuonWorkspaceWrapper(output)
                name = directory + base_name + " " + GROUPINGTABLE
                self._optional_output_names[key] = name
                wrapped_workspace.show(name)
                self.context.ads_observer.observeRename(True)
            elif output_options[key]:
                output = alg.getProperty(key).valueAsStr
                self.context.ads_observer.observeRename(False)
                wrapped_workspace = MuonWorkspaceWrapper(output)
                name = directory + base_name + "(" + str(self.get_num_groups) + " groups) " + optional_output_suffixes[key]
                self._optional_output_names[key] = name
                wrapped_workspace.show(name)
                self.context.ads_observer.observeRename(True)

    def update_view_from_model(self):
        self.getWorkspaceNames()
