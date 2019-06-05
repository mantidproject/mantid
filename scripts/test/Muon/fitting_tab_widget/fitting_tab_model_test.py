import unittest
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_model import FittingTabModel
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantid.api import FunctionFactory, AnalysisDataService
from mantid.simpleapi import CreateWorkspace
from mantid.py3compat import mock


class FittingTabModelTest(unittest.TestCase):
    def setUp(self):
        self.model = FittingTabModel(setup_context())

    def test_convert_function_string_into_dict(self):
        trial_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')

        name = self.model.get_function_name(trial_function)

        self.assertEqual(name, 'GausOsc')

    def test_create_fitted_workspace_name(self):
        input_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1'
        trial_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        expected_directory_name = 'Muon Data/Fitting Output/'
        expected_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1; Fitted; GausOsc'

        name, directory = self.model.create_fitted_workspace_name(input_workspace_name, trial_function)

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_create_parameter_table_name(self):
        input_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1'
        trial_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        expected_directory_name = 'Muon Data/Fitting Output/'
        expected_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1; Fitted Parameters; GausOsc'

        name, directory = self.model.create_parameter_table_name(input_workspace_name, trial_function)

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_do_single_fit_and_return_functions_correctly(self):
        x_data = range(0,100)
        y_data = [5 + x * x for x in x_data]
        workspace = CreateWorkspace(x_data, y_data)
        trial_function = FunctionFactory.createInitialized('name = Quadratic, A0 = 0, A1 = 0, A2 = 0')
        parameter_dict = {'Function': trial_function, 'InputWorkspace': workspace, 'Minimizer': 'Levenberg-Marquardt',
                          'StartX': 0.0, 'EndX': 100.0, 'EvaluationType': 'CentrePoint'}

        output_workspace, parameter_table, fitting_function, fit_status, fit_chi_squared = self.model.do_single_fit_and_return_workspace_parameters_and_fit_function(parameter_dict)

        self.assertAlmostEqual(parameter_table.row(0)['Value'], 5.0)
        self.assertAlmostEqual(parameter_table.row(1)['Value'], 0.0)
        self.assertAlmostEqual(parameter_table.row(2)['Value'], 1.0)
        self.assertEqual(fit_status, 'success')
        self.assertAlmostEqual(fit_chi_squared, 0.0)

    def test_add_workspace_to_ADS_adds_workspace_to_ads_in_correct_group_structure(self):
        workspace = CreateWorkspace([0,0], [0,0])
        workspace_name = 'test_workspace_name'
        workspace_directory = 'root/level one/level two/'

        self.model.add_workspace_to_ADS(workspace, workspace_name, workspace_directory)

        self.assertTrue(AnalysisDataService.doesExist(workspace_name))
        self.assertTrue(AnalysisDataService.doesExist('root'))
        self.assertTrue(AnalysisDataService.doesExist('level one'))
        self.assertTrue(AnalysisDataService.doesExist('level two'))

    def test_create_multi_domain_fitted_workspace_name(self):
        input_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1'
        trial_function = FunctionFactory.createInitialized('composite = MultiDomainFunction, NumDeriv = true;name = Polynomial,'
                                                           ' n = 0, A0 = 0,$domains = i;name = Polynomial, n = 0, A0 = 0,'
                                                           '$domains = i;name = Polynomial, n = 0, A0 = 0,$domains = i;'
                                                           'name = Polynomial, n = 0, A0 = 0,$domains = i')
        expected_directory_name = 'Muon Data/Fitting Output/'
        expected_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1+ ...; Fitted; Polynomial'

        name, directory = self.model.create_multi_domain_fitted_workspace_name(input_workspace_name, trial_function)

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_do_sequential_fit_correctly_delegates_to_do_single_fit(self):
        trial_function = FunctionFactory.createInitialized('name = Quadratic, A0 = 0, A1 = 0, A2 = 0')
        self.model.do_single_fit = mock.MagicMock(return_value=(trial_function, 'success', 0.56))
        x_data = range(0, 100)
        y_data = [5 + x * x for x in x_data]
        workspace = CreateWorkspace(x_data, y_data)
        parameter_dict = {'Function': trial_function, 'InputWorkspace': [workspace] * 5, 'Minimizer': 'Levenberg-Marquardt',
                          'StartX': [0.0] * 5, 'EndX': [100.0] * 5, 'EvaluationType': 'CentrePoint'}

        self.model.do_sequential_fit(parameter_dict)

        self.assertEqual(self.model.do_single_fit.call_count, 5)
        self.model.do_single_fit.assert_called_with({'Function': mock.ANY, 'InputWorkspace': workspace, 'Minimizer': 'Levenberg-Marquardt',
                          'StartX': 0.0, 'EndX': 100.0, 'EvaluationType': 'CentrePoint'})


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
