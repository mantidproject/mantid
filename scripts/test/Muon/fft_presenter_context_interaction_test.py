# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FileFinder
from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_presenter_new
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_view_new
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_model


def retrieve_combobox_info(combo_box):
    output_list = []
    for i in range(combo_box.count()):
        output_list.append(str(combo_box.itemText(i)))

    return output_list


class FFTPresenterTest(GuiTest):
    def setUp(self):
        self.context = setup_context(True)

        self.context.data_context.instrument = 'MUSR'

        self.context.gui_context.update({'RebinType': 'None'})

        self.view = fft_view_new.FFTView()
        self.model1 = fft_model.FFTModel()
        self.model = fft_model.FFTWrapper

        self.presenter = fft_presenter_new.FFTPresenter(
            self.view, self.model, self.context)

        file_path = FileFinder.findRuns('MUSR00022725.nxs')[0]
        ws, run, filename, _ = load_utils.load_workspace_from_filename(file_path)
        self.context.data_context._loaded_data.remove_data(run=run)
        self.context.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument='MUSR')
        self.context.data_context.current_runs = [[22725]]

        self.context.update_current_data()
        test_pair = MuonPair('test_pair', 'top', 'bottom', alpha=0.75)
        self.context.group_pair_context.add_pair(pair=test_pair)
        self.context.show_all_groups()
        self.context.show_all_pairs()

        self.view.warning_popup = mock.MagicMock()

    def tearDown(self):
        self.view = None

    def test_getWorkspaceNames_sets_workspace_and_imaginary_workspace_list_correctly(self):
        self.presenter.getWorkspaceNames()

        self.assertEqual(retrieve_combobox_info(self.view.ws),
                         ['MUSR22725; Group; top; Asymmetry; MA', 'MUSR22725; Group; bkwd; Asymmetry; MA',
                          'MUSR22725; Group; bottom; Asymmetry; MA', 'MUSR22725; Group; fwd; Asymmetry; MA',
                          'MUSR22725; Pair Asym; test_pair; MA'])

        self.assertEqual(retrieve_combobox_info(self.view.Im_ws), ['MUSR22725; Group; top; Asymmetry; MA',
                                                                   'MUSR22725; Group; bkwd; Asymmetry; MA',
                                                                   'MUSR22725; Group; bottom; Asymmetry; MA',
                                                                   'MUSR22725; Group; fwd; Asymmetry; MA',
                                                                   'MUSR22725; Pair Asym; test_pair; MA'])

    def test_handle_use_raw_data_changed_when_no_rebin_set(self):
        self.view.set_raw_checkbox_state(False)

        self.assertEqual(retrieve_combobox_info(self.view.ws),
                         ['MUSR22725; Group; top; Asymmetry; MA', 'MUSR22725; Group; bkwd; Asymmetry; MA',
                          'MUSR22725; Group; bottom; Asymmetry; MA', 'MUSR22725; Group; fwd; Asymmetry; MA',
                          'MUSR22725; Pair Asym; test_pair; MA'])

        self.assertEqual(retrieve_combobox_info(self.view.Im_ws), ['MUSR22725; Group; top; Asymmetry; MA',
                                                                   'MUSR22725; Group; bkwd; Asymmetry; MA',
                                                                   'MUSR22725; Group; bottom; Asymmetry; MA',
                                                                   'MUSR22725; Group; fwd; Asymmetry; MA',
                                                                   'MUSR22725; Pair Asym; test_pair; MA'])

        self.view.warning_popup.assert_called_once_with('No rebin options specified')

    def test_handle_use_raw_data_changed_when_rebin_set(self):
        self.context.gui_context.update({'RebinType': 'Fixed', 'RebinFixed': 2})
        self.context.show_all_groups()
        self.context.show_all_pairs()
        self.view.set_raw_checkbox_state(False)

        self.assertEqual(retrieve_combobox_info(self.view.ws),
                         ['MUSR22725; Group; top; Asymmetry; Rebin; MA', 'MUSR22725; Group; bkwd; Asymmetry; Rebin; MA',
                          'MUSR22725; Group; bottom; Asymmetry; Rebin; MA',
                          'MUSR22725; Group; fwd; Asymmetry; Rebin; MA','MUSR22725; Pair Asym; test_pair; Rebin; MA'])

        self.assertEqual(retrieve_combobox_info(self.view.Im_ws), ['MUSR22725; Group; top; Asymmetry; Rebin; MA',
                                                                   'MUSR22725; Group; bkwd; Asymmetry; Rebin; MA',
                                                                   'MUSR22725; Group; bottom; Asymmetry; Rebin; MA',
                                                                   'MUSR22725; Group; fwd; Asymmetry; Rebin; MA',
                                                                   'MUSR22725; Pair Asym; test_pair; Rebin; MA'])

    def test_get_pre_inputs_with_phase_quad(self):
        workspace_wrapper = mock.MagicMock()
        workspace_wrapper.workspace_name = 'MUSR22725_PhaseQuad_MUSR22725_phase_table'
        self.context.phase_context.add_phase_quad(workspace_wrapper)
        self.presenter.getWorkspaceNames()
        index = self.view.ws.findText('MUSR22725_PhaseQuad_MUSR22725_phase_table')
        self.view.ws.setCurrentIndex(index)

        self.assertEqual(self.presenter.get_pre_inputs(), {'ApodizationFunction': 'Lorentz', 'DecayConstant': 4.4,
                                                            'InputWorkspace': 'MUSR22725_PhaseQuad_MUSR22725_phase_table',
                                                            'NegativePadding': True, 'Padding': 1})

    def test_pre_inputs(self):
        self.presenter.getWorkspaceNames()
        index = self.view.ws.findText('MUSR22725; Group; top; Asymmetry; MA')
        self.view.ws.setCurrentIndex(index)

        self.assertEquals(self.presenter.get_pre_inputs(), {'ApodizationFunction': 'Lorentz', 'DecayConstant': 4.4,
                                                            'InputWorkspace': 'MUSR22725; Group; top; Asymmetry; MA',
                                                            'NegativePadding': True, 'Padding': 1})

    def test_get_imaginary_pre_inputs(self):
        self.presenter.getWorkspaceNames()
        index = self.view.Im_ws.findText('MUSR22725; Pair Asym; test_pair; MA')
        self.view.Im_ws.setCurrentIndex(index)

        self.assertEqual(self.presenter.get_imaginary_inputs(),
                          {'ApodizationFunction': 'Lorentz', 'DecayConstant': 4.4,
                           'InputWorkspace': 'MUSR22725; Pair Asym; test_pair; MA',
                           'NegativePadding': True, 'Padding': 1})

    def test_get_fft_inputs_with_phase_quad_no_imag(self):
        workspace_wrapper = mock.MagicMock()
        workspace_wrapper.workspace_name = 'MUSR22725_PhaseQuad_MUSR22725_phase_table'
        self.context.phase_context.add_phase_quad(workspace_wrapper)
        self.presenter.getWorkspaceNames()
        self.view.imaginary_data = False

        self.assertEqual(
            self.presenter.get_fft_inputs(workspace_wrapper.workspace_name, workspace_wrapper.workspace_name),
            {'AcceptXRoundingErrors': True, 'AutoShift': True,
             'InputWorkspace': workspace_wrapper.workspace_name,
             'Real': 0, 'Transform': 'Forward'})

    def test_get_fft_inputs_with_phase_quad(self):
        workspace_wrapper = mock.MagicMock()
        phase_name = 'MUSR22725_PhaseQuad_MUSR22725_phase_table'
        workspace_wrapper.workspace_name = phase_name
        self.context.phase_context.add_phase_quad(workspace_wrapper)
        self.presenter.getWorkspaceNames()
        self.assertEqual(self.presenter.get_fft_inputs(phase_name, phase_name, 1),
                          {'AcceptXRoundingErrors': True, 'AutoShift': True,
                           'InputWorkspace': phase_name, 'InputImagWorkspace': phase_name,
                           'Real': 0, 'Imaginary': 1, 'Transform': 'Forward'})

    def test_get_fft_inputs_without_phase_quad(self):
        self.presenter.getWorkspaceNames()
        self.view.ws.setCurrentIndex(1)
        self.assertEqual(self.presenter.get_fft_inputs('input_workspace', 'imaginary_input_workspace'),
                          {'AcceptXRoundingErrors': True, 'AutoShift': True, 'Imaginary': 0,
                           'InputImagWorkspace': 'imaginary_input_workspace', 'InputWorkspace': 'input_workspace',
                           'Real': 0, 'Transform': 'Forward'})

    def test_get_fft_inputs_with_no_imaginary_workspace_specified(self):
        self.presenter.getWorkspaceNames()
        self.view.imaginary_data = False

        self.assertEqual(self.presenter.get_fft_inputs('input_workspace', 'imaginary_input_workspace'),
                          {'AcceptXRoundingErrors': True, 'AutoShift': True,
                           'InputWorkspace': 'input_workspace',
                           'Real': 0, 'Transform': 'Forward'})

    @mock.patch('Muon.GUI.FrequencyDomainAnalysis.FFT.fft_presenter_new.run_PaddingAndApodization')
    @mock.patch('Muon.GUI.FrequencyDomainAnalysis.FFT.fft_presenter_new.run_FFT')
    @mock.patch('Muon.GUI.FrequencyDomainAnalysis.FFT.fft_presenter_new.convert_to_field')
    def test_calculate_FFT_calls_correct_algorithm_sequence_for_imaginary_phase_quad(self, field_mock,fft_mock, apodization_mock):
        apodization_mock_return = mock.MagicMock()
        fft_mock_return = mock.MagicMock()
        fft_mock.return_value = fft_mock_return
        field_mock_return = mock.MagicMock()
        field_mock.return_value = field_mock_return
        apodization_mock.return_value = apodization_mock_return

        self.presenter.add_fft_workspace_to_ADS = mock.MagicMock()
        self.presenter.calculate_base_name_and_group = mock.MagicMock(
            return_value=('MUSR22725_PhaseQuad_MUSR22725_phase_table',
                          'MUSR22725 PhaseTable'))
        workspace_wrapper = mock.MagicMock()
        workspace_wrapper.workspace_name = 'MUSR22725_PhaseQuad_MUSR22725_phase_table'
        self.context.phase_context.add_phase_quad(workspace_wrapper)
        self.presenter.getWorkspaceNames()
        index = self.view.ws.findText('MUSR22725_PhaseQuad_MUSR22725_phase_table')
        self.view.ws.setCurrentIndex(index)

        self.presenter.calculate_FFT()

        apodization_mock.assert_called_once_with(
            {'Padding': 1, 'ApodizationFunction': 'Lorentz', 'NegativePadding': True,
             'InputWorkspace': 'MUSR22725_PhaseQuad_MUSR22725_phase_table', 'DecayConstant': 4.4})

        fft_mock.assert_called_once_with({'Real': 0, 'InputWorkspace': apodization_mock_return, 'Transform': 'Forward',
                                          'AcceptXRoundingErrors': True, 'AutoShift': True,
                                          'InputImagWorkspace': apodization_mock_return,
                                          'Imaginary': 1})
        field_mock.assert_called_once_with(fft_mock_return)

        self.presenter.add_fft_workspace_to_ADS.assert_called_once_with('MUSR22725_PhaseQuad_MUSR22725_phase_table',
                                                                        'MUSR22725_PhaseQuad_MUSR22725_phase_table',
                                                                        field_mock_return)


if __name__ == '__main__':
    unittest.main()
