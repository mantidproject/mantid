default_dict = {'first_good_time' : 0.1, 'last_good_time': 15, 'forward_group': 'fwd', 'backward_group': 'bwd', 'input_workspace': None}

class PhaseTableContext(object):
    def __init__(self):
        self.options_dict = default_dict
        self.phase_tables = []

