default_dict = {'first_good_time': 0.1, 'last_good_time': 15, 'forward_group': 'fwd', 'backward_group': 'bwd', 'input_workspace': '',
                'phase_quad_input_workspace': '', 'phase_table_for_phase_quad': ''}


class PhaseTableContext(object):
    def __init__(self):
        self.options_dict = default_dict.copy()
        self.phase_tables = []
        self.phase_quad = []

    def add_phase_table(self, workspace):
        self.phase_tables = [x for x in self.phase_tables if x.workspace_name != workspace.workspace_name]
        self.phase_tables.append(workspace)

    def get_phase_table_list(self, instrument):
        return [phase_table.workspace_name for phase_table in self.phase_tables if instrument in phase_table.workspace_name]

    def add_phase_quad(self, workspace):
        self.phase_quad = [x for x in self.phase_quad if x.workspace_name != workspace.workspace_name]
        self.phase_quad.append(workspace)

    def get_phase_quad(self, instrument, run):
        return [phase_quad.workspace_name for phase_quad in self.phase_quad if instrument in phase_quad.workspace_name
                and run in phase_quad.workspace_name]
