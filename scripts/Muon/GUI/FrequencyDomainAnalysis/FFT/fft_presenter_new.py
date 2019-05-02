# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from Muon.GUI.Common import thread_model
from Muon.GUI.Common.utilities.algorithm_utils import run_PaddingAndApodization, run_FFT
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common.ADSHandler.workspace_naming import get_fft_workspace_name, get_fft_workspace_group_name, get_base_data_directory
import re
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper


class FFTPresenter(object):

    """
    This class links the FFT model to the GUI
    """

    def __init__(self, view, alg, load):
        self.view = view
        self.alg = alg
        self.load = load
        self.thread = None
        # set data
        self.getWorkspaceNames()
        # connect
        self.view.tableClickSignal.connect(self.tableClicked)
        self.view.buttonSignal.connect(self.handleButton)
        self.view.phaseCheckSignal.connect(self.phaseCheck)

        self.view.setup_raw_checkbox_changed(self.handle_use_raw_data_changed)

    def cancel(self):
        if self.thread is not None:
            self.thread.cancel()

    def runChanged(self):
        self.getWorkspaceNames()

    @property
    def widget(self):
        return self.view

    # turn on button
    def activate(self):
        self.view.activateButton()

    # turn off button
    def deactivate(self):
        self.view.deactivateButton()

    def getWorkspaceNames(self):
        name = self.view.workspace
        final_options = self.load.get_workspace_names_for_FFT_analysis(self.view.use_raw_data)

        self.view.addItems(final_options)
        self.view.removeRe('PhaseQuad')
        self.removePhaseFromIM(final_options)

        self.view.workspace = name

    def handle_use_raw_data_changed(self):
        if not self.view.use_raw_data and not self.load._do_rebin():
            self.view.set_raw_checkbox_state(True)
            self.view.warning_popup('No rebin options specified')
            return

        self.getWorkspaceNames()

    def removePhaseFromIM(self, final_options):
        for option in final_options:
            if "PhaseQuad" in option:
                self.view.removeIm(option)

    # functions
    def phaseCheck(self):
        self.view.phaseQuadChanged()
        # check if a phase table exists
        if mantid.AnalysisDataService.doesExist("PhaseTable"):
            self.view.setPhaseBox()

    def tableClicked(self, row, col):
        if row == self.view.getImBoxRow() and col == 1 and "PhaseQuad" not in self.view.workspace:
            self.view.changedHideUnTick(
                self.view.getImBox(),
                self.view.getImBoxRow() + 1)
        elif row == self.view.getShiftBoxRow() and col == 1:
            self.view.changed(
                self.view.getShiftBox(),
                self.view.getShiftBoxRow() + 1)

    def createThread(self):
        self._phasequad_calculation_model = ThreadModelWrapper(self.calculate_FFT)
        return thread_model.ThreadModel(self._phasequad_calculation_model)

    # constructs the inputs for the FFT algorithms
    # then executes them (see fft_model to see the order
    # of execution
    def handleButton(self):
        # put this on its own thread so not to freeze Mantid
        self.thread = self.createThread()
        self.thread.threadWrapperSetUp(self.deactivate, self.handleFinished, self.handle_error)

        self.thread.start()

    def handle_error(self, error):
        self.view.activateButton()
        self.view.warning_popup(error)

    def get_pre_inputs(self):
        pre_inputs = {}

        pre_inputs['InputWorkspace'] = self.view.workspace
        pre_inputs["ApodizationFunction"] = self.view.apodization_function
        pre_inputs["DecayConstant"] = self.view.decay_constant
        pre_inputs["NegativePadding"] = self.view.negative_padding
        pre_inputs["Padding"] = self.view.padding_value

        return pre_inputs

    def get_imaginary_inputs(self):
        pre_inputs = {}

        pre_inputs['InputWorkspace'] = self.view.imaginary_workspace
        pre_inputs["ApodizationFunction"] = self.view.apodization_function
        pre_inputs["DecayConstant"] = self.view.decay_constant
        pre_inputs["NegativePadding"] = self.view.negative_padding
        pre_inputs["Padding"] = self.view.padding_value

        return pre_inputs

    def get_fft_inputs(self, real_workspace, imaginary_workspace, imanginary=0):
        FFTInputs = {}

        FFTInputs["AcceptXRoundingErrors"] = True
        FFTInputs['Real'] = 0
        FFTInputs['InputWorkspace'] = real_workspace
        FFTInputs['Transform'] = 'Forward'
        FFTInputs['AutoShift'] = self.view.auto_shift

        if self.view.imaginary_data:
            FFTInputs['InputImagWorkspace'] = imaginary_workspace
            FFTInputs['Imaginary'] = imanginary

        return FFTInputs

    # kills the thread at end of execution
    def handleFinished(self):
        self.activate()

    def calculate_FFT(self):
        imaginary_workspace_index = 0
        real_workspace_padding_parameters = self.get_pre_inputs()
        imaginary_workspace_padding_parameters = self.get_imaginary_inputs()

        real_workspace_input = run_PaddingAndApodization(real_workspace_padding_parameters)

        if self.view.imaginary_data:
            if 'PhaseQuad' in self.view.workspace:
                imaginary_workspace_input = real_workspace_input
                imaginary_workspace_index = 1
            else:
                imaginary_workspace_input = run_PaddingAndApodization(imaginary_workspace_padding_parameters)
        else:
            imaginary_workspace_input = None

        fft_parameters = self.get_fft_inputs(real_workspace_input, imaginary_workspace_input, imaginary_workspace_index)

        frequency_domain_workspace = run_FFT(fft_parameters)

        self.add_fft_workspace_to_ADS(real_workspace_padding_parameters['InputWorkspace'], frequency_domain_workspace)

    def add_fft_workspace_to_ADS(self, input_workspace, fft_workspace):
        run = re.search('[0-9]+', input_workspace).group()
        fft_workspace_name = get_fft_workspace_name(input_workspace)
        group = get_fft_workspace_group_name(fft_workspace_name, self.load.data_context.instrument)
        directory = get_base_data_directory(self.load, run) + group

        muon_workspace_wrapper = MuonWorkspaceWrapper(fft_workspace, directory + fft_workspace_name)
        muon_workspace_wrapper.show()
