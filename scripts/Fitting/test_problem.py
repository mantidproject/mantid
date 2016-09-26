class FittingTestProblem:
    """
    Definition of a fitting test problem, normally loaded from a problem definition file.
    """
    def __init__(self):
        self.name = None
        # If there is an online/documentation link describing this problem
        self.linked_name = None
        self.equation = None
        self.start_x = None
        self.end_x = None
        self.starting_values = None
        self.data_pattern_in = None
        self.data_pattern_out = None
        self.data_pattern_obs_errors = None
        self.ref_residual_sum_sq = None
