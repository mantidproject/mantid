# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as mantid

from Muon.GUI.Common import thread_model
from Muon.GUI.Common.utilities.algorithm_utils import run_PaddingAndApodization, run_FFT, convert_to_field, \
    extract_single_spec
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common.ADSHandler.workspace_naming import get_fft_workspace_name, get_fft_workspace_group_name, \
    get_group_or_pair_from_name
import re
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from mantidqt.utils.observer_pattern import GenericObservable
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FREQUENCY_EXTENSIONS


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
        self.calculation_finished_notifier = GenericObservable()
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
        # get current values
        original_Re_name = self.view.workspace
        original_Im_name = self.view.imaginary_workspace
        final_options = self.load.get_workspace_names_for_FFT_analysis(self.view.use_raw_data)

        # update view
        self.view.addItems(final_options)

        # make intelligent guess of what user wants
        current_group_pair = self.load.group_pair_context[self.load.group_pair_context.selected]
        Re_name_to_use = None
        Im_name_to_use = None
        default_name = None
        # will need to check this exists before using it
        if current_group_pair:
            default_name = current_group_pair.get_asymmetry_workspace_names(
                    self.load.data_context.current_runs)
        # if the original selection is available we should use it
        if original_Re_name in final_options:
            Re_name_to_use = original_Re_name
        elif default_name:
            Re_name_to_use = default_name[0]
        self.view.workspace = Re_name_to_use
        if original_Im_name in final_options:
            Im_name_to_use = original_Im_name
        elif default_name:
            Im_name_to_use = default_name[0]
        self.view.imaginary_workspace=Im_name_to_use
        return

    def handle_use_raw_data_changed(self):
        if not self.view.use_raw_data and not self.load._do_rebin():
            self.view.set_raw_checkbox_state(True)
            self.view.warning_popup('No rebin options specified')
            return

        self.getWorkspaceNames()

    def tableClicked(self, row, col):
        if row == self.view.getImBoxRow() and col == 1:
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
        pre_inputs = self._get_generic_apodiazation_and_padding_inputs()
        pre_inputs['InputWorkspace'] = self.view.workspace

        return pre_inputs

    def get_imaginary_inputs(self):
        pre_inputs = self._get_generic_apodiazation_and_padding_inputs()

        pre_inputs['InputWorkspace'] = self.view.imaginary_workspace

        return pre_inputs

    def _get_generic_apodiazation_and_padding_inputs(self):
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
        self.calculation_finished_notifier.notify_subscribers(self._output_workspace_name)

    def calculate_FFT(self):
        imaginary_workspace_index = 0
        real_workspace_padding_parameters = self.get_pre_inputs()
        imaginary_workspace_padding_parameters = self.get_imaginary_inputs()

        real_workspace_input = run_PaddingAndApodization(real_workspace_padding_parameters, '__real')

        if self.view.imaginary_data:
            imaginary_workspace_input = run_PaddingAndApodization(imaginary_workspace_padding_parameters, '__Imag')
        else:
            imaginary_workspace_input = None
            imaginary_workspace_padding_parameters['InputWorkspace'] = ""

        fft_parameters = self.get_fft_inputs(real_workspace_input, imaginary_workspace_input, imaginary_workspace_index)

        frequency_domain_workspace = convert_to_field(run_FFT(fft_parameters))
        self.add_fft_workspace_to_ADS(real_workspace_padding_parameters['InputWorkspace'],
                                      imaginary_workspace_padding_parameters['InputWorkspace'],
                                      frequency_domain_workspace)

    def add_fft_workspace_to_ADS(self, input_workspace, imaginary_input_workspace, fft_workspace_label):
        run = re.search('[0-9]+', input_workspace).group()
        fft_workspace = mantid.AnalysisDataService.retrieve(fft_workspace_label)
        Im_run = ""
        if imaginary_input_workspace != "":
            Im_run = re.search('[0-9]+', imaginary_input_workspace).group()
        fft_workspace_name = get_fft_workspace_name(input_workspace, imaginary_input_workspace)
        directory = get_fft_workspace_group_name(fft_workspace_name, self.load.data_context.instrument,
                                                 self.load.workspace_suffix)
        Re = get_group_or_pair_from_name(input_workspace)
        Im = get_group_or_pair_from_name(imaginary_input_workspace)
        shift = 3 if fft_workspace.getNumberHistograms() == 6 else 0
        spectra = {"_" + FREQUENCY_EXTENSIONS["RE"]: 0 + shift, "_" + FREQUENCY_EXTENSIONS["IM"]: 1 + shift,
                   "_" + FREQUENCY_EXTENSIONS["MOD"]: 2 + shift}

        for spec_type in list(spectra.keys()):
            extracted_ws = extract_single_spec(fft_workspace, spectra[spec_type], fft_workspace_name + spec_type)

            self.load._frequency_context.add_FFT(fft_workspace_name + spec_type, run, Re, Im_run, Im)

            muon_workspace_wrapper = MuonWorkspaceWrapper(extracted_ws)
            muon_workspace_wrapper.show(directory + fft_workspace_name + spec_type)

        # This is a small hack to get the output name to a location where it can be part of the calculation finished
        # signal.
        self._output_workspace_name = fft_workspace_name + '_mod'

    def update_view_from_model(self):
        self.getWorkspaceNames()
