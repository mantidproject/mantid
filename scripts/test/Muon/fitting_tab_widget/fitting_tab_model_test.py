import unittest
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_model import FittingTabModel


class FittingTabModelTest(unittest.TestCase):
    def setUp(self):
        self.model = FittingTabModel()

    def test_convert_function_string_into_dict(self):
        trial_function_string = 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0'
        expected_dict = {'name': 'GausOsc', 'A': '0.2','Sigma': '0.2','Frequency':'0.1','Phi':'0'}

        result = self.model.convert_function_string_into_dict(trial_function_string)

        self.assertEqual(expected_dict, result)

    def test_create_fitted_workspace_name(self):
        input_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1'
        function_name = 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0'
        expected_directory_name = 'Muon Data/Fitting Output/'
        expected_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1; Fitted; GausOsc'

        name, directory = self.model.create_fitted_workspace_name(input_workspace_name, function_name)

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

    def test_create_parameter_table_name(self):
        input_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1'
        function_name = 'name=GausOsc,A=0.2,Sigma=0.2,Frequency=0.1,Phi=0'
        expected_directory_name = 'Muon Data/Fitting Output/'
        expected_workspace_name = 'MUSR22725; Group; top; Asymmetry; #1; Fitted Parameters; GausOsc'

        name, directory = self.model.create_parameter_table_name(input_workspace_name, function_name)

        self.assertEqual(name, expected_workspace_name)
        self.assertEqual(directory, expected_directory_name)

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)