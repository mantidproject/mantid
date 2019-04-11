# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.Common import mock_widget
from qtpy import QtWidgets
from mantid.py3compat import mock
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.Common import thread_model
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_presenter_new
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_view
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_model
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FrequencyContext
from Muon.GUI.Common.contexts.context_setup import setup_context_for_tests
from Muon.GUI.Common.muon_pair import MuonPair
from mantid.api import FileFinder


class FFTPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtWidgets.QWidget()
        setup_context_for_tests(self)

        self.data_context.instrument = 'MUSR'
        self.frequency_context = FrequencyContext(self.context)

        self.gui_context.update({'RebinType': 'None'})

        self.view = fft_view.FFTView(self.obj)
        self.model1 = fft_model.FFTModel()
        self.model = fft_model.FFTWrapper

        self.presenter = fft_presenter_new.FFTPresenter(
            self.view, self.model, self.context)

        file_path = FileFinder.findRuns('MUSR00022725.nxs')[0]
        ws, run, filename = load_utils.load_workspace_from_filename(file_path)
        self.data_context._loaded_data.remove_data(run=run)
        self.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument='MUSR')
        self.data_context.current_runs = [[22725]]

        self.context.update_current_data()
        test_pair = MuonPair('test_pair', 'top', 'bottom', alpha=0.75)
        self.group_context.add_pair(pair=test_pair)

        self.view.warning_popup = mock.MagicMock()

    def test_getWorkspaceNames_sets_workspace_and_imaginary_workspace_list_correctly(self):
        self.presenter.getWorkspaceNames()

        self.assertEquals(self.view.ws.itemText(0), 'MUSR22725_raw_data (PhaseQuad)')
        self.assertEquals(self.view.ws.itemText(1), 'MUSR22725; Pair Asym; test_pair; #1')
        self.assertEquals(self.view.ws.itemText(2), 'MUSR22725; Group; top; Asymmetry; #1')
        self.assertEquals(self.view.ws.itemText(3), 'MUSR22725; Group; bkwd; Asymmetry; #1')
        self.assertEquals(self.view.ws.itemText(4), 'MUSR22725; Group; bottom; Asymmetry; #1')
        self.assertEquals(self.view.ws.itemText(5), 'MUSR22725; Group; fwd; Asymmetry; #1')

        self.assertEquals(self.view.Im_ws.itemText(0), 'MUSR22725; Pair Asym; test_pair; #1')
        self.assertEquals(self.view.Im_ws.itemText(1), 'MUSR22725; Group; top; Asymmetry; #1')
        self.assertEquals(self.view.Im_ws.itemText(2), 'MUSR22725; Group; bkwd; Asymmetry; #1')
        self.assertEquals(self.view.Im_ws.itemText(3), 'MUSR22725; Group; bottom; Asymmetry; #1')
        self.assertEquals(self.view.Im_ws.itemText(4), 'MUSR22725; Group; fwd; Asymmetry; #1')

    def test_handle_use_raw_data_changed_when_no_rebin_set(self):
        self.view.set_raw_checkbox_state(False)

        self.assertEquals(self.view.ws.itemText(0), 'MUSR22725_raw_data (PhaseQuad)')
        self.assertEquals(self.view.ws.itemText(1), 'MUSR22725; Pair Asym; test_pair; #1')
        self.assertEquals(self.view.ws.itemText(2), 'MUSR22725; Group; top; Asymmetry; #1')
        self.assertEquals(self.view.ws.itemText(3), 'MUSR22725; Group; bkwd; Asymmetry; #1')
        self.assertEquals(self.view.ws.itemText(4), 'MUSR22725; Group; bottom; Asymmetry; #1')
        self.assertEquals(self.view.ws.itemText(5), 'MUSR22725; Group; fwd; Asymmetry; #1')

        self.assertEquals(self.view.Im_ws.itemText(0), 'MUSR22725; Pair Asym; test_pair; #1')
        self.assertEquals(self.view.Im_ws.itemText(1), 'MUSR22725; Group; top; Asymmetry; #1')
        self.assertEquals(self.view.Im_ws.itemText(2), 'MUSR22725; Group; bkwd; Asymmetry; #1')
        self.assertEquals(self.view.Im_ws.itemText(3), 'MUSR22725; Group; bottom; Asymmetry; #1')
        self.assertEquals(self.view.Im_ws.itemText(4), 'MUSR22725; Group; fwd; Asymmetry; #1')

        self.view.warning_popup.assert_called_once_with('No rebin options specified')

    def test_handle_use_raw_data_changed_when_rebin_set(self):
        self.gui_context.update({'RebinType': 'Fixed', 'RebinFixed' : 2})
        self.view.set_raw_checkbox_state(False)

        self.assertEquals(self.view.ws.itemText(0), 'MUSR22725_raw_data (PhaseQuad)')
        self.assertEquals(self.view.ws.itemText(1), 'MUSR22725; Pair Asym; test_pair; Rebin; #1')
        self.assertEquals(self.view.ws.itemText(2), 'MUSR22725; Group; top; Asymmetry; Rebin; #1')
        self.assertEquals(self.view.ws.itemText(3), 'MUSR22725; Group; bkwd; Asymmetry; Rebin; #1')
        self.assertEquals(self.view.ws.itemText(4), 'MUSR22725; Group; bottom; Asymmetry; Rebin; #1')
        self.assertEquals(self.view.ws.itemText(5), 'MUSR22725; Group; fwd; Asymmetry; Rebin; #1')

        self.assertEquals(self.view.Im_ws.itemText(0), 'MUSR22725; Pair Asym; test_pair; Rebin; #1')
        self.assertEquals(self.view.Im_ws.itemText(1), 'MUSR22725; Group; top; Asymmetry; Rebin; #1')
        self.assertEquals(self.view.Im_ws.itemText(2), 'MUSR22725; Group; bkwd; Asymmetry; Rebin; #1')
        self.assertEquals(self.view.Im_ws.itemText(3), 'MUSR22725; Group; bottom; Asymmetry; Rebin; #1')
        self.assertEquals(self.view.Im_ws.itemText(4), 'MUSR22725; Group; fwd; Asymmetry; Rebin; #1')

    def test_get_pre_inputs_with_phase_quad(self):
        self.presenter.getWorkspaceNames()

        self.assertEquals(self.presenter.get_pre_inputs(), {'ApodizationFunction': 'Lorentz', 'DecayConstant': 4.4,
                                                            'InputWorkspace': '__phaseQuad__', 'NegativePadding': 2,
                                                            'OutputWorkspace': '__ReTmp__', 'Padding': 1})

    def test_pre_inputs_without_phasequad(self):
        self.presenter.getWorkspaceNames()
        self.view.ws.setCurrentIndex(1)

        self.assertEquals(self.presenter.get_pre_inputs(), {'ApodizationFunction': 'Lorentz', 'DecayConstant': 4.4,
                                                            'InputWorkspace': u'MUSR22725; Pair Asym; test_pair; #1',
                                                            'NegativePadding': 2,
                                                            'OutputWorkspace': '__ReTmp__', 'Padding': 1})

    def test_get_imaginary_pre_inputs(self):
        self.presenter.getWorkspaceNames()
        self.assertEquals(self.presenter.get_imaginary_inputs(), {'ApodizationFunction': 'Lorentz', 'DecayConstant': 4.4,
                                                                  'InputWorkspace': u'MUSR22725; Pair Asym; test_pair; #1',
                                                                  'NegativePadding': 2,
                                                                  'OutputWorkspace': '__ImTmp__', 'Padding': 1})

    def test_get_fft_inputs_with_phase_quad(self):
        self.presenter.getWorkspaceNames()
        self.assertEquals(self.presenter.get_fft_inputs(), {'AcceptXRoundingErrors': True, 'AutoShift': True, 'Imaginary': 1,
                                                            'InputImagWorkspace': '__ReTmp__', 'InputWorkspace': '__ReTmp__',
                                                            'OutputWorkspace': u'MUSR22725_raw_data;PhaseQuad;FFT', 'Real': 0})

    def test_get_fft_inputs_without_phase_quad(self):
        self.presenter.getWorkspaceNames()
        self.view.ws.setCurrentIndex(1)
        self.assertEquals(self.presenter.get_fft_inputs(),
                          {'AcceptXRoundingErrors': True, 'AutoShift': True, 'Imaginary': 0,
                           'InputImagWorkspace': '__ImTmp__', 'InputWorkspace': '__ReTmp__',
                           'OutputWorkspace': u'MUSR22725; Pair Asym; test_pair; #1;FFT', 'Real': 0})

    def test_get_initial_input_run(self):
        self.presenter.getWorkspaceNames()

        self.assertEquals(self.presenter.get_input_run(), 'MUSR22725')
        self.view.ws.setCurrentIndex(1)
        self.assertEquals(self.presenter.get_input_run(), 'MUSR22725')

if __name__ == '__main__':
    unittest.main()