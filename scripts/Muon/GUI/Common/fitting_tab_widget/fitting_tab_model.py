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
from mantid.api import FunctionFactory


class FittingTabModel(object):
    def __init__(self, context):
        self.context = context

    def do_single_fit(self, parameter_dict):
        output_workspace, fitting_parameters_table, function_string, output_status, output_chi_squared =\
            self.do_single_fit_and_return_workspace_parameters_and_fit_function(parameter_dict)

        workspace_name, directory = self.create_fitted_workspace_name(
            parameter_dict['InputWorkspace'], parameter_dict['Function'])
        table_name, directory = self.create_parameter_table_name(
            parameter_dict['InputWorkspace'], parameter_dict['Function'])

        self.add_workspace_to_ADS(output_workspace, workspace_name, directory)
        wrapped_parameter_workspace = self.add_workspace_to_ADS(
            fitting_parameters_table, table_name, directory)
        self.add_fit_to_context(wrapped_parameter_workspace,
                                parameter_dict['Function'],
                                parameter_dict['InputWorkspace'])

        return function_string, output_status, output_chi_squared

    def do_single_fit_and_return_workspace_parameters_and_fit_function(
            self, parameters_dict):
        alg = mantid.AlgorithmManager.create("Fit")
        return run_Fit(parameters_dict, alg)

    def add_workspace_to_ADS(self, workspace, name, directory):
        workspace_wrapper = MuonWorkspaceWrapper(workspace, directory + name)
        workspace_wrapper.show()
        return workspace_wrapper

    def create_fitted_workspace_name(self, input_workspace_name,
                                     function_name):
        directory = get_fit_workspace_base_directory()
        name = input_workspace_name + '; Fitted; ' + self.get_function_name(
            function_name)

        return name, directory

    def create_multi_domain_fitted_workspace_name(self, input_workspace,
                                                  function):
        directory = get_fit_workspace_base_directory()
        name = input_workspace + '+ ...; Fitted; ' + self.get_function_name(
            function)

        return name, directory

    def create_parameter_table_name(self, input_workspace_name, function_name):
        directory = get_fit_workspace_base_directory()
        name = input_workspace_name + '; Fitted Parameters; ' + self.get_function_name(
            function_name)
        return name, directory

    def do_simultaneous_fit(self, parameter_dict):
        output_workspace, fitting_parameters_table, function_string, output_status, output_chi_squared =\
            self.do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(parameter_dict)

        workspace_name, directory = self.create_multi_domain_fitted_workspace_name(
            parameter_dict['InputWorkspace'][0], parameter_dict['Function'])
        table_name, directory = self.create_parameter_table_name(
            parameter_dict['InputWorkspace'][0] + '+ ...',
            parameter_dict['Function'])

        self.add_workspace_to_ADS(output_workspace, workspace_name, directory)
        if len(parameter_dict['InputWorkspace']) > 1:
            self.rename_members_of_fitted_workspace_group(
                output_workspace, parameter_dict['InputWorkspace'],
                parameter_dict['Function'])
        wrapped_parameter_workspace = self.add_workspace_to_ADS(
            fitting_parameters_table, table_name, directory)
        self.add_fit_to_context(wrapped_parameter_workspace,
                                parameter_dict['Function'],
                                parameter_dict['InputWorkspace'])

        return function_string, output_status, output_chi_squared

    def do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(
            self, parameters_dict):
        alg = mantid.AlgorithmManager.create("Fit")
        return run_simultaneous_Fit(parameters_dict, alg)

    def rename_members_of_fitted_workspace_group(self, group_workspace,
                                                 inputworkspace_list,
                                                 function):
        for index, workspace_name in enumerate(group_workspace.getNames()):
            new_name, _ = self.create_fitted_workspace_name(
                inputworkspace_list[index], function)
            new_name += '; Simultaneous'
            RenameWorkspace(InputWorkspace=workspace_name,
                            OutputWorkspace=new_name)

    def do_sequential_fit(self, parameter_dict):
        function_string_list = []
        output_status_list = []
        output_chi_squared_list = []
        function_string = parameter_dict['Function']
        for input_workspace, startX, endX in zip(
                parameter_dict['InputWorkspace'], parameter_dict['StartX'],
                parameter_dict['EndX']):
            sub_parameter_dict = parameter_dict.copy()
            sub_parameter_dict['InputWorkspace'] = input_workspace
            sub_parameter_dict['StartX'] = startX
            sub_parameter_dict['EndX'] = endX
            sub_parameter_dict['Function'] = function_string

            function_string, output_status, output_chi_squared = self.do_single_fit(
                sub_parameter_dict)
            # This is required so that a new function object is created that is not overwritten in subsequent iterations
            new_function = FunctionFactory.createInitialized(
                str(function_string))
            function_string_list.append(new_function)
            output_status_list.append(output_status)
            output_chi_squared_list.append(output_chi_squared)

        return function_string_list, output_status_list, output_chi_squared_list

    def get_function_name(self, function):
        if function.getNumberDomains() > 1:
            return function.getFunction(0).name()
        else:
            return function.name()

    def add_fit_to_context(self, parameter_workspace, function,
                           input_workspace):
        self.context.fitting_context.add_fit_from_values(
            parameter_workspace, self.get_function_name(function),
            input_workspace)
