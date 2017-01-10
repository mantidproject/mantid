class FunctionalConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction
    """

    DEFAULT_TOOL = 'tomopy'
    DEFAULT_ALGORITHM = 'gridrec'

    def __init__(self):
        """
        Builds a default post-processing configuration with a sensible choice of parameters
        """
        self.input_dir = None
        self.input_dir_flat = None
        self.input_dir_dark = None
        self.output_dir = None
        self.cor = None
        self.find_cor = None

        self.tool = self.DEFAULT_TOOL
        self.algorithm = self.DEFAULT_ALGORITHM
        self.num_iter = None
        self.regularization = None

    def __str__(self):
        mystr = "Input dir:{0}\nFlat dir:{1}\nDark dir:{2}\nOutput dir:{3}\nCOR:{4}\nFind_COR:{5}\nTool:{6}\nAlgorithm:{7}\nNum iter:{8}\nRegularization:{9}"
        .format(
            str(self.input_dir), str(
                self.input_dir_flat), str(self.input_dir_dark),
            str(self.output_dir), str(self.cor), str(
                self.find_cor), str(self.tool),
            str(self.algorithm), str(self.num_iter), str(self.regularization))
