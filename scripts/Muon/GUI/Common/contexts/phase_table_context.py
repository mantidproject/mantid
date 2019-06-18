# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

default_dict = {'first_good_time': 0.1, 'last_good_time': 15, 'forward_group': 'fwd', 'backward_group': 'bwd', 'input_workspace': '',
                'phase_quad_input_workspace': '', 'phase_table_for_phase_quad': ''}


class PhaseTableContext(object):
    def __init__(self):
        self.options_dict = default_dict.copy()
        self.phase_tables = []
        self.phase_quad = []

    def add_phase_table(self, workspace):
        self.phase_tables = [phase_table for phase_table in self.phase_tables if phase_table.workspace_name != workspace.workspace_name]
        self.phase_tables.append(workspace)

    def get_phase_table_list(self, instrument):
        return [phase_table.workspace_name for phase_table in self.phase_tables if instrument in phase_table.workspace_name]

    def add_phase_quad(self, workspace):
        self.phase_quad = [item for item in self.phase_quad if item.workspace_name != workspace.workspace_name]
        self.phase_quad.append(workspace)

    def get_phase_quad(self, instrument, run):
        return [phase_quad.workspace_name for phase_quad in self.phase_quad if instrument in phase_quad.workspace_name
                and run in phase_quad.workspace_name]

    def remove_workspace_by_name(self, workspace_name):
        self.phase_tables = [item for item in self.phase_tables if item!=workspace_name]
        self.phase_quad = [item for item in self.phase_tables if item!=workspace_name]
