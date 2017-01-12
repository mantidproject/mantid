class FunctionalConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction
    """

    DEFAULT_TOOL = 'tomopy'
    DEFAULT_ALGORITHM = 'gridrec'

    def __init__(self):
        """
        Builds a default post-processing configuration with a sensible choice of parameters

        find_cor: Currently a boolean, TODO will be an int so that we can use different methods

        verbosity: Default 2, existing levels:
            0 - Silent, no output at all (not recommended)
            1 - Low verbosity, will output each step that is being performed
            2 - Normal verbosity, will output each step and execution time
            3 - High verbosity, will output the step name, execution time and memory usage before and after each step

        crash_on_failed_import: Default True, this option tells if the program should stop execution if an import
                                fails and a step cannot be executed:
            True - Raise an exception and stop execution immediately
            False - Note the failure to import but continue execution without applying the filter
        """
        self.debug = True
        self.debug_port = 59003
        self.input_dir = None
        self.input_dir_flat = None
        self.input_dir_dark = None

        self.readme_file_name = '0.README_reconstruction.txt'
        self.preproc_images_subdir = 'pre_processed'
        self.preproc_images_format = 'fits'
        self.output_dir = None
        self.out_slices_file_name_prefix = 'out_recon_slice'
        self.out_horiz_slices_subdir = 'out_recon_horiz_slice'

        import numpy as np

        # TODO test all data types:
        # uint16, float16, float32, float64!
        # Was getting bad data with float16 after median filter and scaling up
        # to save as PNG
        self.data_dtype = np.float32

        self.cor = None
        self.find_cor = None

        # TODO test verbosity works properly on each level
        self.verbosity = 3  # default 2
        self.crash_on_failed_import = True  # default True

        self.tool = self.DEFAULT_TOOL
        self.algorithm = self.DEFAULT_ALGORITHM
        self.num_iter = None
        self.regularization = None

    def __str__(self):
        return "Input dir:{0}\nFlat dir:{1}\nDark dir:{2}\nOutput dir:{3}\nCOR:{4}\nFind_COR:{5}\nTool:{6}\n" \
            "Algorithm:{7}\nNum iter:{8}\nRegularization:{9}".format(str(self.input_dir),
                                                                     str(self.input_dir_flat),
                                                                     str(self.input_dir_dark),
                                                                     str(self.output_dir), str(
                self.cor),
                str(self.find_cor), str(
                self.tool),
                str(self.algorithm), str(
                self.num_iter),
                str(self.regularization))
