# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import functools
import math
import re

import mantid.simpleapi as mantid

from Muon.GUI.Common import thread_model
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.ADSHandler.workspace_naming import get_maxent_workspace_group_name, get_maxent_workspace_name, \
    get_base_data_directory
from Muon.GUI.Common.observer_pattern import GenericObserver
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common.utilities.algorithm_utils import run_MuonMaxent

raw_data = "_raw_data"

optional_output_suffixes = {'OutputPhaseTable': '_phase_table', 'OutputDeadTimeTable': '_dead_times',
                            'ReconstructedSpectra': '_reconstructed_spectra', 'PhaseConvergenceTable': '_phase_convergence'}


class MaxEntPresenter(object):

    """
    This class links the MaxEnt model to the GUI
    """

    def __init__(self, view, load):
        self.view = view
        self.load = load
        self.thread = None
        # set data
        self.getWorkspaceNames()
        # connect
        self.view.maxEntButtonSignal.connect(self.handleMaxEntButton)
        self.view.cancelSignal.connect(self.cancel)

        self.phase_table_observer = GenericObserver(self.update_phase_table_options)

    @property
    def widget(self):
        return self.view

    def runChanged(self):
        self.getWorkspaceNames()

    def clear(self):
        self.view.addItems([])
        self.view.update_phase_table_combo([])

    # functions
    def getWorkspaceNames(self):
        final_options = self.load.getGroupedWorkspaceNames()

        self.view.addItems(final_options)
        start = int(
            math.ceil(math.log(self.load.data_context.num_points) / math.log(2.0)))
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

        self.thread.start()

    # kills the thread at end of execution
    def handleFinished(self):
        self.activate()

    def handle_error(self, error):
        self.activate()
        self.view.warning_popup(error)

    def calculate_maxent(self, alg):
        maxent_parameters = self.get_parameters_for_maxent_calculation()

        maxent_workspace = run_MuonMaxent(maxent_parameters, alg)

        self.add_maxent_workspace_to_ADS(maxent_parameters['InputWorkspace'], maxent_workspace, alg)

    def get_parameters_for_maxent_calculation(self):
        inputs = {}

        inputs['InputWorkspace'] = self.view.input_workspace
        run = [float(re.search('[0-9]+', inputs['InputWorkspace']).group())]

        if self.view.phase_table != 'Construct':
            inputs['InputPhaseTable'] = self.view.phase_table

        if self.load.dead_time_table(run):
            inputs['InputDeadTimeTable'] = self.load.dead_time_table(run)

        inputs['FirstGoodTime'] = self.load.first_good_data(run)

        inputs['LastGoodTime'] = self.load.last_good_data(run)

        inputs['Npts'] = self.view.num_points

        inputs['InnerIterations'] = self.view.inner_iterations

        inputs['OuterIterations'] = self.view.outer_iterations

        inputs['DoublePulse'] = self.view.double_pulse

        inputs['Factor'] = self.view.lagrange_multiplier

        inputs['MaxField'] = self.view.maximum_field

        inputs['DefaultLevel'] = self.view.maximum_entropy_constant

        inputs['FitDeadTime'] = self.view.fit_dead_times

        return inputs

    def update_phase_table_options(self):
        phase_table_list = self.load.phase_context.get_phase_table_list(self.load.data_context.instrument)
        phase_table_list.insert(0, 'Construct')

        self.view.update_phase_table_combo(phase_table_list)

    def add_maxent_workspace_to_ADS(self, input_workspace, maxent_workspace, alg):
        run = re.search('[0-9]+', input_workspace).group()
        base_name = get_maxent_workspace_name(input_workspace)
        group = get_maxent_workspace_group_name(base_name, self.load.data_context.instrument, self.load.workspace_suffix)
        directory = get_base_data_directory(self.load, run) + group

        muon_workspace_wrapper = MuonWorkspaceWrapper(maxent_workspace, directory + base_name)
        muon_workspace_wrapper.show()

        maxent_output_options = self.get_maxent_output_options()

        self.add_optional_outputs_to_ADS(alg, maxent_output_options, base_name, directory)

    def get_maxent_output_options(self):
        output_options = {}

        output_options['OutputPhaseTable'] = self.view.output_phase_table
        output_options['OutputDeadTimeTable'] = self.view.output_dead_times
        output_options['ReconstructedSpectra'] = self.view.output_reconstructed_spectra
        output_options['PhaseConvergenceTable'] = self.view.output_phase_convergence

        return output_options

    def add_optional_outputs_to_ADS(self, alg, output_options, base_name, directory):
        for key in output_options:
            if output_options[key]:
                output = alg.getProperty(key).value
                MuonWorkspaceWrapper(output, directory + base_name + optional_output_suffixes[key]).show()

    def update_view_from_model(self):
        self.getWorkspaceNames()
