# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from Muon.GUI.Common.utilities.algorithm_utils import run_Fit, run_simultaneous_Fit
import mantid
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.ADSHandler.workspace_naming import get_fit_workspace_directory
from mantid.simpleapi import RenameWorkspace, CopyLogs


class FittingTabModel(object):
    def __init__(self, context):
        self.context = context
        self.function_name = ''

    def do_single_fit(self, parameter_dict):
        fit_group_name = parameter_dict.pop('FitGroupName')
        output_workspace, fitting_parameters_table, function_object, output_status, output_chi_squared = \
            self.do_single_fit_and_return_workspace_parameters_and_fit_function(parameter_dict)

        workspace_name, workspace_directory = self.create_fitted_workspace_name(parameter_dict['InputWorkspace'],
                                                                                parameter_dict['Function'],
                                                                                fit_group_name)
        table_name, table_directory = self.create_parameter_table_name(parameter_dict['InputWorkspace'],
                                                                       parameter_dict['Function'], fit_group_name)

        self.add_workspace_to_ADS(output_workspace, workspace_name, workspace_directory)
        wrapped_parameter_workspace = self.add_workspace_to_ADS(fitting_parameters_table, table_name, table_directory)
        self.add_fit_to_context(wrapped_parameter_workspace,
                                parameter_dict['Function'],
                                parameter_dict['InputWorkspace'], [workspace_name])

        return function_object.clone(), output_status, output_chi_squared

    def do_single_fit_and_return_workspace_parameters_and_fit_function(
            self, parameters_dict):
        alg = mantid.AlgorithmManager.create("Fit")
        output_workspace, output_parameters, function_object, output_status, output_chi = run_Fit(parameters_dict, alg)
        CopyLogs(InputWorkspace=parameters_dict['InputWorkspace'], OutputWorkspace=output_workspace, StoreInADS=False)
        return output_workspace, output_parameters, function_object, output_status, output_chi

    def add_workspace_to_ADS(self, workspace, name, directory):
        workspace_wrapper = MuonWorkspaceWrapper(workspace, directory + name)
        workspace_wrapper.show()
        return workspace_wrapper

    def create_fitted_workspace_name(self, input_workspace_name, function_name, group_name):
        directory = get_fit_workspace_directory(group_name, '_workspaces', self.context.data_context.base_directory,
                                                self.context.workspace_suffix)
        name = input_workspace_name + '; Fitted; ' + self.function_name

        return name, directory

    def create_multi_domain_fitted_workspace_name(self, input_workspace, function, group_name):
        directory = get_fit_workspace_directory(group_name, '_workspaces', self.context.data_context.base_directory,
                                                self.context.workspace_suffix)
        name = input_workspace + '+ ...; Fitted; ' + self.function_name

        return name, directory

    def create_parameter_table_name(self, input_workspace_name, function_name, group_name):
        directory = get_fit_workspace_directory(group_name, '_parameter_tables', self.context.data_context.base_directory,
                                                self.context.workspace_suffix)
        name = input_workspace_name + '; Fitted Parameters; ' + self.function_name

        return name, directory

    def do_simultaneous_fit(self, parameter_dict, global_parameters):
        fit_group_name = parameter_dict.pop('FitGroupName')
        output_workspace, fitting_parameters_table, function_object, output_status, output_chi_squared = \
            self.do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(parameter_dict)

        workspace_name, workspace_directory = self.create_multi_domain_fitted_workspace_name(
            parameter_dict['InputWorkspace'][0],
            parameter_dict['Function'], fit_group_name)
        table_name, table_directory = self.create_parameter_table_name(parameter_dict['InputWorkspace'][0] + '+ ...',
                                                                       parameter_dict['Function'], fit_group_name)

        self.add_workspace_to_ADS(output_workspace, workspace_name, workspace_directory)
        if len(parameter_dict['InputWorkspace']) > 1:
            workspace_name = self.rename_members_of_fitted_workspace_group(output_workspace, parameter_dict['InputWorkspace'],
                                                                           parameter_dict['Function'],
                                                                           fit_group_name)
        wrapped_parameter_workspace = self.add_workspace_to_ADS(fitting_parameters_table, table_name,
                                                                table_directory)
        self.add_fit_to_context(wrapped_parameter_workspace,
                                parameter_dict['Function'],
                                parameter_dict['InputWorkspace'],
                                workspace_name,
                                global_parameters)

        return function_object, output_status, output_chi_squared

    def do_simultaneous_fit_and_return_workspace_parameters_and_fit_function(
            self, parameters_dict):
        alg = mantid.AlgorithmManager.create("Fit")

        output_workspace, output_parameters, function_object, output_status, output_chi = run_simultaneous_Fit(parameters_dict, alg)
        if len(parameters_dict['InputWorkspace']) > 1:
            for input_workspace, output in zip(parameters_dict['InputWorkspace'], output_workspace.getNames()):
                CopyLogs(InputWorkspace=input_workspace, OutputWorkspace=output, StoreInADS=False)
        else:
            CopyLogs(InputWorkspace=parameters_dict['InputWorkspace'][0], OutputWorkspace=output_workspace, StoreInADS=False)
        return output_workspace, output_parameters, function_object, output_status, output_chi

    def rename_members_of_fitted_workspace_group(self, group_workspace, inputworkspace_list, function, group_name):
        output_workspace_list = []
        for index, workspace_name in enumerate(group_workspace.getNames()):
            new_name, _ = self.create_fitted_workspace_name(inputworkspace_list[index], function, group_name)

            new_name += '; Simultaneous'
            output_workspace_list.append(new_name)
            RenameWorkspace(InputWorkspace=workspace_name,
                            OutputWorkspace=new_name)

        return output_workspace_list

    def do_sequential_fit(self, parameter_dict):
        function_object_list = []
        output_status_list = []
        output_chi_squared_list = []
        function_object = parameter_dict['Function']
        for input_workspace, startX, endX in zip(parameter_dict['InputWorkspace'], parameter_dict['StartX'],
                                                 parameter_dict['EndX']):
            sub_parameter_dict = parameter_dict.copy()
            sub_parameter_dict['InputWorkspace'] = input_workspace
            sub_parameter_dict['StartX'] = startX
            sub_parameter_dict['EndX'] = endX
            sub_parameter_dict['Function'] = function_object

            function_object, output_status, output_chi_squared = self.do_single_fit(sub_parameter_dict)
            # This is required so that a new function object is created that is not overwritten in subsequent iterations
            new_function = function_object.clone()
            function_object_list.append(new_function)

            output_status_list.append(output_status)
            output_chi_squared_list.append(output_chi_squared)

        return function_object_list, output_status_list, output_chi_squared_list

    def get_function_name(self, function):
        if function is None:
            return ''

        if function.getNumberDomains() > 1:
            function_temp = function.getFunction(0)
        else:
            function_temp = function

        try:
            function_string_list = []
            for i in range(function_temp.nFunctions()):
                function_string_list.append(function_temp.getFunction(i).name())
            if len(function_string_list) > 3:
                function_string_list = function_string_list[:3]
                function_string_list.append('...')
            function_string = ','.join(function_string_list)
            return function_string
        except AttributeError:
            return function_temp.name()

    def add_fit_to_context(self, parameter_workspace, function,
                           input_workspace, output_workspace_name, global_parameters=None):
        self.context.fitting_context.add_fit_from_values(
            parameter_workspace, self.function_name,
            input_workspace, output_workspace_name, global_parameters)
