# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.utilities.algorithm_utils import run_Fit, run_simultaneous_Fit
import mantid
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.ADSHandler.workspace_naming import get_fit_workspace_base_directory
from mantid.simpleapi import RenameWorkspace


class FittingTabModel(object):
    def __init__(self):
        pass

    def do_single_fit(self, parameter_dict):
        output_workspace, fitting_parameters_table, function_string = self.do_single_fit_and_return_workspace_parameters_and_fit_function(
            parameter_dict)

        workspace_name, directory = self.create_fitted_workspace_name(parameter_dict['InputWorkspace'],
                                                                      parameter_dict['Function'])
        table_name, directory = self.create_parameter_table_name(parameter_dict['InputWorkspace'],
                                                                 parameter_dict['Function'])

        self.add_workspace_to_ADS(output_workspace, workspace_name, directory)
        self.add_workspace_to_ADS(fitting_parameters_table, table_name, directory)

    def do_single_fit_and_return_workspace_parameters_and_fit_function(self, parameters_dict):
        alg = mantid.AlgorithmManager.create("Fit")
        return run_Fit(parameters_dict, alg)

    def add_workspace_to_ADS(self, workspace, name, directory):
        workspace_wrapper = MuonWorkspaceWrapper(workspace, directory + name)
        workspace_wrapper.show()

    def create_fitted_workspace_name(self, input_workspace_name, function_name):
        directory = get_fit_workspace_base_directory()
        name = input_workspace_name + '; Fitted; ' + self.convert_function_string_into_dict(function_name)['name']

        return name, directory

    def create_multi_domain_fitted_workspace_name(self, input_workspace, function):
        directory = get_fit_workspace_base_directory()
        name = input_workspace + '+ ...; Fitted; ' + self.convert_function_string_into_dict(function)['name']

        return name, directory

    def create_parameter_table_name(self, input_workspace_name, function_name):
        directory = get_fit_workspace_base_directory()
        name = input_workspace_name + '; Fitted Parameters; ' + self.convert_function_string_into_dict(function_name)[
            'name']
        return name, directory

    def convert_function_string_into_dict(self, function_string):
        value_list = function_string.replace(' ', '').replace(';', ',').split(',')
        value_dict = {item.split('=')[0]: item.split('=')[1] for item in value_list}
        return value_dict

    def do_simultaneous_fit(self, parameter_dict):
        output_workspace, fitting_parameters_table, function_string = self.do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(parameter_dict)

        workspace_name, directory = self.create_multi_domain_fitted_workspace_name(parameter_dict['InputWorkspace'][0],
                                                                      parameter_dict['Function'])
        table_name, directory = self.create_parameter_table_name(parameter_dict['InputWorkspace'][0] + '+ ...',
                                                                 parameter_dict['Function'])

        self.add_workspace_to_ADS(output_workspace, workspace_name, directory)
        self.rename_members_of_fitted_workspace_group(output_workspace, parameter_dict['InputWorkspace'], parameter_dict['Function'])
        self.add_workspace_to_ADS(fitting_parameters_table, table_name, directory)

    def do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(self, parameters_dict):
        alg = mantid.AlgorithmManager.create("Fit")
        return run_simultaneous_Fit(parameters_dict, alg)

    def rename_members_of_fitted_workspace_group(self, group_workspace, inputworkspace_list, function):
        for index, workspace_name in enumerate(group_workspace.getNames()):
            new_name, _ = self.create_fitted_workspace_name(inputworkspace_list[index], function)
            new_name += '; Simultaneous'
            RenameWorkspace(InputWorkspace=workspace_name, OutputWorkspace=new_name)
