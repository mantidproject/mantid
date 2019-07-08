# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
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
        expected_directory_name = 'Muon Data/Fitting Output MA/Fitting Output_workspaces MA/'
        expected_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1; Fitted; GausOsc'
        self.model.function_name = 'GausOsc'

        name, directory = self.model.create_fitted_workspace_name(input_workspace_name, trial_function,
                                                                  'Fitting Output')

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_create_parameter_table_name(self):
        input_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1'
        trial_function = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0')
        expected_directory_name = 'Muon Data/Fitting Output MA/Fitting Output_parameter_tables MA/'
        expected_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1; Fitted Parameters; GausOsc'
        self.model.function_name = 'GausOsc'

        name, directory = self.model.create_parameter_table_name(input_workspace_name, trial_function, 'Fitting Output')

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_do_single_fit_and_return_functions_correctly(self):
        x_data = range(0, 100)
        y_data = [5 + x * x for x in x_data]
        workspace = CreateWorkspace(x_data, y_data)
        trial_function = FunctionFactory.createInitialized('name = Quadratic, A0 = 0, A1 = 0, A2 = 0')
        parameter_dict = {'Function': trial_function, 'InputWorkspace': workspace, 'Minimizer': 'Levenberg-Marquardt',
                          'StartX': 0.0, 'EndX': 100.0, 'EvaluationType': 'CentrePoint'}

        output_workspace, parameter_table, fitting_function, fit_status, fit_chi_squared = self.model.do_single_fit_and_return_workspace_parameters_and_fit_function(
            parameter_dict)

        self.assertAlmostEqual(parameter_table.row(0)['Value'], 5.0)
        self.assertAlmostEqual(parameter_table.row(1)['Value'], 0.0)
        self.assertAlmostEqual(parameter_table.row(2)['Value'], 1.0)
        self.assertEqual(fit_status, 'success')
        self.assertAlmostEqual(fit_chi_squared, 0.0)

    def test_add_workspace_to_ADS_adds_workspace_to_ads_in_correct_group_structure(self):
        workspace = CreateWorkspace([0, 0], [0, 0])
        workspace_name = 'test_workspace_name'
        workspace_directory = 'root/level one/level two/'

        self.model.add_workspace_to_ADS(workspace, workspace_name, workspace_directory)

        self.assertTrue(AnalysisDataService.doesExist(workspace_name))
        self.assertTrue(AnalysisDataService.doesExist('root'))
        self.assertTrue(AnalysisDataService.doesExist('level one'))
        self.assertTrue(AnalysisDataService.doesExist('level two'))

    def test_create_multi_domain_fitted_workspace_name(self):
        input_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1'
        trial_function = FunctionFactory.createInitialized(
            'composite = MultiDomainFunction, NumDeriv = true;name = Polynomial,'
            ' n = 0, A0 = 0,$domains = i;name = Polynomial, n = 0, A0 = 0,'
            '$domains = i;name = Polynomial, n = 0, A0 = 0,$domains = i;'
            'name = Polynomial, n = 0, A0 = 0,$domains = i')
        expected_directory_name = 'Muon Data/Fitting Output MA/Fitting Output_workspaces MA/'
        expected_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1+ ...; Fitted; Polynomial'
        self.model.function_name = 'Polynomial'

        name, directory = self.model.create_multi_domain_fitted_workspace_name(input_workspace_name, trial_function,
                                                                               'Fitting Output')

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_do_sequential_fit_correctly_delegates_to_do_single_fit(self):
        trial_function = FunctionFactory.createInitialized('name = Quadratic, A0 = 0, A1 = 0, A2 = 0')
        self.model.do_single_fit = mock.MagicMock(return_value=(trial_function, 'success', 0.56))
        x_data = range(0, 100)
        y_data = [5 + x * x for x in x_data]
        workspace = CreateWorkspace(x_data, y_data)
        parameter_dict = {'Function': trial_function, 'InputWorkspace': [workspace] * 5,
                          'Minimizer': 'Levenberg-Marquardt',
                          'StartX': [0.0] * 5, 'EndX': [100.0] * 5, 'EvaluationType': 'CentrePoint'}

        self.model.do_sequential_fit(parameter_dict)

        self.assertEqual(self.model.do_single_fit.call_count, 5)
        self.model.do_single_fit.assert_called_with(
            {'Function': mock.ANY, 'InputWorkspace': workspace, 'Minimizer': 'Levenberg-Marquardt',
             'StartX': 0.0, 'EndX': 100.0, 'EvaluationType': 'CentrePoint'})

    def test_do_simultaneous_fit_adds_single_input_workspace_to_fit_context_with_globals(self):
        trial_function = FunctionFactory.createInitialized('name = Quadratic, A0 = 0, A1 = 0, A2 = 0')
        x_data = range(0, 100)
        y_data = [5 + x * x for x in x_data]
        workspace = CreateWorkspace(x_data, y_data)
        parameter_dict = {'Function': trial_function, 'InputWorkspace': [workspace.name()],
                          'Minimizer': 'Levenberg-Marquardt',
                          'StartX': [0.0], 'EndX': [100.0], 'EvaluationType': 'CentrePoint',
                          'FitGroupName': 'SimulFit'}
        global_parameters = ['A0']
        self.model.do_simultaneous_fit(parameter_dict, global_parameters)

        fit_context = self.model.context.fitting_context
        self.assertEqual(1, len(fit_context))
        self.assertEqual(global_parameters, fit_context.fit_list[0].parameters.global_parameters)

    def test_do_simultaneous_fit_adds_multi_input_workspace_to_fit_context(self):
        # create function
        single_func = ';name=FlatBackground,$domains=i,A0=0'
        multi_func = 'composite=MultiDomainFunction,NumDeriv=1' + single_func + single_func +";"
        trial_function = FunctionFactory.createInitialized(multi_func)
        x_data = range(0, 100)
        y_data = [5 + x * x for x in x_data]
        workspace1 = CreateWorkspace(x_data, y_data)
        workspace2 = CreateWorkspace(x_data, y_data)
        parameter_dict = {'Function': trial_function, 'InputWorkspace': [workspace1.name(), workspace2.name()],
                          'Minimizer': 'Levenberg-Marquardt',
                          'StartX': [0.0]*2, 'EndX': [100.0]*2, 'EvaluationType': 'CentrePoint',
                          'FitGroupName': 'SimulFit'}
        self.model.do_simultaneous_fit(parameter_dict, global_parameters=[])

        fit_context = self.model.context.fitting_context
        self.assertEqual(1, len(fit_context))

    def test_get_function_name_returns_correctly_for_composite_functions(self):
        function_string = 'name=FlatBackground,A0=22.5129;name=Polynomial,n=0,A0=-22.5221;name=ExpDecayOsc,A=-0.172352,Lambda=0.111109,Frequency=-0.280031,Phi=-3.03983'
        function_object = FunctionFactory.createInitialized(function_string)

        name_as_string = self.model.get_function_name(function_object)

        self.assertEqual(name_as_string, 'FlatBackground,Polynomial,ExpDecayOsc')

    def test_get_function_name_returns_correctly_for_composite_functions_multi_domain_function(self):
        function_string = 'composite=MultiDomainFunction,NumDeriv=true;(composite=CompositeFunction,NumDeriv=false,' \
                          '$domains=i;name=FlatBackground,A0=-0.800317;name=Polynomial,n=0,A0=0.791112;name=ExpDecayOsc,' \
                          'A=0.172355,Lambda=0.111114,Frequency=0.280027,Phi=-0.101717);(composite=CompositeFunction,' \
                          'NumDeriv=false,$domains=i;name=FlatBackground,A0=1.8125;name=Polynomial,n=0,A0=-1.81432;' \
                          'name=ExpDecayOsc,A=0.17304,Lambda=0.102673,Frequency=0.278695,Phi=-0.0461353);' \
                          '(composite=CompositeFunction,NumDeriv=false,$domains=i;name=FlatBackground,A0=1.045;' \
                          'name=Polynomial,n=0,A0=-1.04673;name=ExpDecayOsc,A=-0.170299,Lambda=0.118256,Frequency=0.281085,Phi=-0.155812)'
        function_object = FunctionFactory.createInitialized(function_string)

        name_as_string = self.model.get_function_name(function_object)

        self.assertEqual(name_as_string, 'FlatBackground,Polynomial,ExpDecayOsc')

    def test_get_function_name_truncates_function_with_more_than_three_composite_members(self):
        function_string = 'name=ExpDecayOsc,A=-5.87503,Lambda=0.0768409,Frequency=0.0150173,Phi=-1.15833;name=GausDecay,' \
                          'A=-1.59276,Sigma=0.361339;name=ExpDecayOsc,A=-5.87503,Lambda=0.0768409,Frequency=0.0150173,' \
                          'Phi=-1.15833;name=ExpDecayMuon,A=-0.354664,Lambda=-0.15637;name=DynamicKuboToyabe,' \
                          'BinWidth=0.050000000000000003,Asym=7.0419,Delta=0.797147,Field=606.24,Nu=2.67676e-09'
        function_object = FunctionFactory.createInitialized(function_string)

        name_as_string = self.model.get_function_name(function_object)

        self.assertEqual(name_as_string, 'ExpDecayOsc,GausDecay,ExpDecayOsc,...')

    def test_get_function_name_does_truncate_for_exactly_four_members(self):
        function_string = 'name=ExpDecayOsc,A=-5.87503,Lambda=0.0768409,Frequency=0.0150173,Phi=-1.15833;name=GausDecay,' \
                          'A=-1.59276,Sigma=0.361339;name=ExpDecayOsc,A=-5.87503,Lambda=0.0768409,Frequency=0.0150173,' \
                          'Phi=-1.15833;name=ExpDecayMuon,A=-0.354664,Lambda=-0.15637'
        function_object = FunctionFactory.createInitialized(function_string)

        name_as_string = self.model.get_function_name(function_object)

        self.assertEqual(name_as_string, 'ExpDecayOsc,GausDecay,ExpDecayOsc,...')

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
