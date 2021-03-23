# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_model
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_presenter_new
from Muon.GUI.FrequencyDomainAnalysis.FFT import fft_view_new
from mantidqt.utils.qt.testing import start_qapplication

from mantid.api import FileFinder

GROUP_LIST = ['top', 'bkwd', 'bottom', 'fwd']
EXAMPLE_PAIR = 'test_pair'
ADS_WORKSPACE_NAMES = ['MUSR22725; Group; top; Asymmetry; FD',
                       'MUSR22725; Group; bkwd; Asymmetry; FD',
                       'MUSR22725; Group; bottom; Asymmetry; FD',
                       'MUSR22725; Group; fwd; Asymmetry; FD',
                       'MUSR22725; Pair Asym; test_pair; FD']


def retrieve_combobox_info(combo_box):
    output_list = []
    for i in range(combo_box.count()):
        output_list.append(str(combo_box.itemText(i)))

    return output_list


@start_qapplication
class FFTPresenterTest(unittest.TestCase):

    def remove_from_group_selection(self, group):
        self.context.group_pair_context._selected_groups = [grp for grp in GROUP_LIST if grp != group]

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
        ws, run, filename, _, _ = load_utils.load_workspace_from_filename(file_path)
        self.context.data_context._loaded_data.remove_data(run=run)
        self.context.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument='MUSR')
        self.context.data_context.current_runs = [[22725]]

        self.context.update_current_data()
        test_pair = MuonPair(EXAMPLE_PAIR, 'top', 'bottom', alpha=0.75)
        self.context.group_pair_context.add_pair(pair=test_pair)
        self.context.show_all_groups()
        self.context.show_all_pairs()
        self.context.group_pair_context._selected_groups = GROUP_LIST
        self.context.group_pair_context._selected_pairs = [EXAMPLE_PAIR]

        self.view.warning_popup = mock.MagicMock()

    def tearDown(self):
        self.view = None

    def test_getWorkspaceNames_sets_workspace_and_imaginary_workspace_list_correctly(self):
        self.presenter.getWorkspaceNames()

        self.assertEqual(retrieve_combobox_info(self.view.ws),
                         ['MUSR22725; Group; top; Asymmetry; FD', 'MUSR22725; Group; bkwd; Asymmetry; FD',
                          'MUSR22725; Group; bottom; Asymmetry; FD', 'MUSR22725; Group; fwd; Asymmetry; FD',
                          'MUSR22725; Pair Asym; test_pair; FD'])

        self.assertEqual(retrieve_combobox_info(self.view.Im_ws), ['MUSR22725; Group; top; Asymmetry; FD',
                                                                   'MUSR22725; Group; bkwd; Asymmetry; FD',
                                                                   'MUSR22725; Group; bottom; Asymmetry; FD',
                                                                   'MUSR22725; Group; fwd; Asymmetry; FD',
                                                                   'MUSR22725; Pair Asym; test_pair; FD'])
        self.assertEqual(self.view.workspace, 'MUSR22725; Group; fwd; Asymmetry; FD')

    def test_keep_selection_for_getWorkspaceNames(self):
        # load some data and then make a selection
        self.presenter.getWorkspaceNames()
        self.view.workspace = 'MUSR22725; Group; bkwd; Asymmetry; FD'
        self.view.imaginary_workspace = 'MUSR22725; Group; bottom; Asymmetry; FD'
        # update names
        self.presenter.getWorkspaceNames()

        self.assertEqual(self.view.workspace, 'MUSR22725; Group; bkwd; Asymmetry; FD')
        self.assertEqual(self.view.imaginary_workspace, 'MUSR22725; Group; bottom; Asymmetry; FD')

    def test_selection_removed_genWorkspaceName(self):
        # load some data and then make a selection
        self.presenter.getWorkspaceNames()
        self.view.workspace = 'MUSR22725; Group; bkwd; Asymmetry; FD'
        self.view.imaginary_workspace = 'MUSR22725; Group; bottom; Asymmetry; FD'
        # remove bottom group
        self.context.group_pair_context.remove_group("bottom")
        # update names
        self.presenter.getWorkspaceNames()

        self.assertEqual(self.view.workspace, 'MUSR22725; Group; bkwd; Asymmetry; FD')
        self.assertEqual(self.view.imaginary_workspace, 'MUSR22725; Group; fwd; Asymmetry; FD')

    def test_handle_use_raw_data_changed_when_no_rebin_set(self):
        self.view.set_raw_checkbox_state(False)

        self.assertEqual(retrieve_combobox_info(self.view.ws),
                         ['MUSR22725; Group; top; Asymmetry; FD', 'MUSR22725; Group; bkwd; Asymmetry; FD',
                          'MUSR22725; Group; bottom; Asymmetry; FD', 'MUSR22725; Group; fwd; Asymmetry; FD',
                          'MUSR22725; Pair Asym; test_pair; FD'])

        self.assertEqual(retrieve_combobox_info(self.view.Im_ws), ['MUSR22725; Group; top; Asymmetry; FD',
                                                                   'MUSR22725; Group; bkwd; Asymmetry; FD',
                                                                   'MUSR22725; Group; bottom; Asymmetry; FD',
                                                                   'MUSR22725; Group; fwd; Asymmetry; FD',
                                                                   'MUSR22725; Pair Asym; test_pair; FD'])

        self.view.warning_popup.assert_called_once_with('No rebin options specified')

    def test_handle_use_raw_data_changed_when_rebin_set(self):
        self.context.gui_context.update({'RebinType': 'Fixed', 'RebinFixed': 2})
        self.context.show_all_groups()
        self.context.show_all_pairs()
        self.view.set_raw_checkbox_state(False)

        self.assertEqual(retrieve_combobox_info(self.view.ws),
                         ['MUSR22725; Group; top; Asymmetry; Rebin; FD', 'MUSR22725; Group; bkwd; Asymmetry; Rebin; FD',
                          'MUSR22725; Group; bottom; Asymmetry; Rebin; FD',
                          'MUSR22725; Group; fwd; Asymmetry; Rebin; FD', 'MUSR22725; Pair Asym; test_pair; Rebin; FD'])

        self.assertEqual(retrieve_combobox_info(self.view.Im_ws), ['MUSR22725; Group; top; Asymmetry; Rebin; FD',
                                                                   'MUSR22725; Group; bkwd; Asymmetry; Rebin; FD',
                                                                   'MUSR22725; Group; bottom; Asymmetry; Rebin; FD',
                                                                   'MUSR22725; Group; fwd; Asymmetry; Rebin; FD',
                                                                   'MUSR22725; Pair Asym; test_pair; Rebin; FD'])

    def test_pre_inputs(self):
        self.presenter.getWorkspaceNames()
        index = self.view.ws.findText('MUSR22725; Group; top; Asymmetry; FD')
        self.view.ws.setCurrentIndex(index)

        self.assertEquals(self.presenter.get_pre_inputs(), {'ApodizationFunction': 'Lorentz', 'DecayConstant': 4.4,
                                                            'InputWorkspace': 'MUSR22725; Group; top; Asymmetry; FD',
                                                            'NegativePadding': True, 'Padding': 1})

    def test_get_imaginary_pre_inputs(self):
        self.presenter.getWorkspaceNames()
        index = self.view.Im_ws.findText('MUSR22725; Pair Asym; test_pair; FD')
        self.view.Im_ws.setCurrentIndex(index)

        self.assertEqual(self.presenter.get_imaginary_inputs(),
                         {'ApodizationFunction': 'Lorentz', 'DecayConstant': 4.4,
                          'InputWorkspace': 'MUSR22725; Pair Asym; test_pair; FD',
                          'NegativePadding': True, 'Padding': 1})

    def test_get_fft_inputs(self):
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
    def test_calculate_FFT_calls_correct_algorithm_sequence_for_no_imaginary(self, field_mock, fft_mock,
                                                                             apodization_mock):
        self.view.imaginary_data = False
        name = 'MUSR22725; Group; top; Asymmetry; FD'

        field_mock_return = mock.MagicMock()
        field_mock.return_value = field_mock_return
        apodization_mock_return = mock.MagicMock()
        fft_mock_return = mock.MagicMock()
        fft_mock.return_value = fft_mock_return
        apodization_mock.return_value = apodization_mock_return
        self.presenter.add_fft_workspace_to_ADS = mock.MagicMock()

        self.presenter.getWorkspaceNames()
        index = self.view.ws.findText(name)
        self.view.ws.setCurrentIndex(index)

        self.presenter.calculate_FFT()

        apodization_mock.assert_called_once_with(
            {'Padding': 1, 'ApodizationFunction': 'Lorentz', 'NegativePadding': True,
             'InputWorkspace': name, 'DecayConstant': 4.4}, '__real')

        fft_mock.assert_called_once_with({'AcceptXRoundingErrors': True,
                                          'Real': 0,
                                          'InputWorkspace': apodization_mock_return,
                                          'AutoShift': True,
                                          'Transform': 'Forward'})

        field_mock.assert_called_once_with(fft_mock_return)
        self.presenter.add_fft_workspace_to_ADS.assert_called_once_with(name, '', field_mock_return)

    @mock.patch('Muon.GUI.FrequencyDomainAnalysis.FFT.fft_presenter_new.run_PaddingAndApodization')
    @mock.patch('Muon.GUI.FrequencyDomainAnalysis.FFT.fft_presenter_new.run_FFT')
    @mock.patch('Muon.GUI.FrequencyDomainAnalysis.FFT.fft_presenter_new.convert_to_field')
    def test_calculate_FFT_calls_correct_algorithm_sequence_with_imaginary(self, field_mock, fft_mock,
                                                                           apodization_mock):
        self.view.imaginary_data = True
        name = 'MUSR22725; Group; top; Asymmetry; FD'
        Im_name = 'MUSR22725; Pair Asym; test_pair; FD'

        field_mock_return = mock.MagicMock()
        field_mock.return_value = field_mock_return
        apodization_mock_return = mock.MagicMock()
        fft_mock_return = mock.MagicMock()
        fft_mock.return_value = fft_mock_return
        apodization_mock.return_value = apodization_mock_return
        self.presenter.add_fft_workspace_to_ADS = mock.MagicMock()

        self.presenter.getWorkspaceNames()
        index = self.view.ws.findText(name)
        self.view.ws.setCurrentIndex(index)
        index = self.view.Im_ws.findText(Im_name)
        self.view.Im_ws.setCurrentIndex(index)

        self.presenter.calculate_FFT()

        apodization_mock.has_calls(mock.call(
            {'Padding': 1, 'ApodizationFunction': 'Lorentz', 'NegativePadding': True,
             'InputWorkspace': Im_name, 'DecayConstant': 4.4}),
            mock.call(
                {'Padding': 1, 'ApodizationFunction': 'Lorentz', 'NegativePadding': True,
                 'InputWorkspace': name, 'DecayConstant': 4.4}))

        fft_mock.assert_called_once_with({
            'Real': 0,
            'InputWorkspace': apodization_mock_return,
            'Transform': 'Forward',
            'AcceptXRoundingErrors': True,
            'AutoShift': True,
            'InputImagWorkspace': apodization_mock_return,
            "Imaginary": 0})
        field_mock.assert_called_once_with(fft_mock_return)
        self.presenter.add_fft_workspace_to_ADS.assert_called_once_with(name, Im_name, field_mock_return)

    def test_selection_removed_WorkspaceName(self):
        # Remove first group from selection
        self.remove_from_group_selection('top')

        self.presenter.getWorkspaceNames()

        self.assertEqual(retrieve_combobox_info(self.view.ws), ADS_WORKSPACE_NAMES[1:])
        self.assertEqual(retrieve_combobox_info(self.view.Im_ws), ADS_WORKSPACE_NAMES[1:])

    def test_selection_added_genWorkspaceName(self):
        # Start with bottom 3 three groups selected
        self.context.group_pair_context._selected_groups = GROUP_LIST[1:]

        self.presenter.getWorkspaceNames()

        self.assertEqual(retrieve_combobox_info(self.view.ws), ADS_WORKSPACE_NAMES[1:])
        self.assertEqual(retrieve_combobox_info(self.view.Im_ws), ADS_WORKSPACE_NAMES[1:])

        # Add first group back in
        self.context.group_pair_context._selected_groups = GROUP_LIST
        self.presenter.getWorkspaceNames()

        self.assertEqual(retrieve_combobox_info(self.view.ws), ADS_WORKSPACE_NAMES)
        self.assertEqual(retrieve_combobox_info(self.view.Im_ws), ADS_WORKSPACE_NAMES)


if __name__ == '__main__':
    unittest.main()
