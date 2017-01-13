class FunctionalConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction
    """

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

        self.readme_file_name = '0.README_reconstruction.txt'

        # Functionality options
        self.input_dir = None
        self.input_dir_flat = None
        self.input_dir_dark = None
        self.in_img_format = 'fits'

        self.preproc_subdir = 'pre_processed'
        self.preproc_format = 'fits'
        self.preproc_as_stack = True

        self.output_dir = None
        self.out_img_format = 'fits'
        self.out_slices_file_name_prefix = 'out_recon_slice'
        self.out_horiz_slices_subdir = 'out_recon_horiz_slice'

        import numpy as np
        # TODO more tests with float16/float64/uint16
        self.data_dtype = np.float32

        self.cor = None
        self.find_cor = False

        # TODO test verbosity works properly on each level
        self.verbosity = 3  # default 2

        # TODO unused (add exception handling funcitons in helper.py)
        # default True
        self.crash_on_failed_import = True

        # Reconstruction options
        self.debug = True
        self.debug_port = 59003

        self.tool = 'tomopy'
        self.algorithm = 'gridrec'
        self.num_iter = 5
        self.max_angle = 360.0

    def __str__(self):
        return "Input dir: {0}".format(str(self.input_dir)) \
               + "Flat dir: {0}".format(str(self.input_dir_flat)) \
               + "Dark dir: {0}".format(str(self.input_dir_dark)) \
               + "In image format: {0}".format(str(self.in_img_format)) \
               + "Pre processing images subdir: {0}".format(str(self.preproc_subdir)) \
               + "Pre processing images format: {0}".format(str(self.preproc_format)) \
               + "Pre processing images as stack: {0}".format(str(self.preproc_as_stack)) \
               + "Output dir: {0}".format(str(self.output_dir)) \
               + "Output image format: {0}".format(str(self.out_img_format)) \
               + "Output slices file name prefix: {0}".format(str(self.out_slices_file_name_prefix)) \
               + "Output horizontal slices subdir: {0}".format(str(self.out_horiz_slices_subdir)) \
               + "Debug: {0}".format(str(self.debug)) \
               + "Debug port: {0}".format(str(self.debug_port)) \
               + "Data dtype: {0}".format(str(self.data_dtype)) \
               + "Argument COR: {0}".format(str(self.cor)) \
               + "Find COR Run: {0}".format(str(self.find_cor)) \
               + "Verbosity: {0}".format(str(self.verbosity)) \
               + "Crash on failed import: {0}".format(str(self.crash_on_failed_import)) \
               + "Tool: {0}".format(str(self.tool)) \
               + "Algorithm: {0}".format(str(self.algorithm)) \
               + "Number of iterations: {0}".format(str(self.num_iter)) \
               + "Maximum angle: {0}".format(str(self.max_angle))

    def setup_parser(self, parser):
        """
                Setup the functional arguments for the script
                :param parser: The parser which is set up
                """
        grp_func = parser.add_argument_group('Functionality options')

        grp_func.add_argument(
            "-i", "--input-path", required=True, type=str, help="Input directory", default=self.input_dir)

        grp_func.add_argument(
            "-iflat",
            "--input-path-flat",
            required=False,
            default=self.input_dir_flat,
            type=str,
            help="Input directory for flat images")

        grp_func.add_argument(
            "-idark",
            "--input-path-dark",
            required=False,
            default=self.input_dir_dark,
            type=str,
            help="Input directory for flat images")

        img_formats = ['tiff', 'fits', 'tif', 'fit', 'png']
        grp_func.add_argument(
            "--in-img-format",
            required=False,
            default='fits',
            type=str,
            help="Format/file extension expected for the input images. Supported: {0}".
            format(img_formats))

        grp_func.add_argument(
            "--preproc-subdir",
            required=False,
            type=str,
            default=self.preproc_subdir,
            help="Default output-path/pre_processed/. The subdirectory for the pre-processed images."
        )

        grp_func.add_argument(
            "--preproc-format",
            required=False,
            type=str,
            default=self.preproc_format,
            help="Format/file extension expected for the output of the pre processed images. Supported: {0}".
            format(img_formats)
        )

        grp_func.add_argument(
            "--preproc-as-stack",
            required=False,
            action='store_true',
            help="Format/file extension expected for the output of the pre processed images. Supported: {0}".
            format(img_formats)
        )

        grp_func.add_argument(
            "-o",
            "--output-path",
            required=True,
            default=self.output_dir,
            type=str,
            help="Where to write the output slice images (reconstructed volume)")

        grp_func.add_argument(
            "--out-img-format",
            required=False,
            default='tiff',
            type=str,
            help="Format/file extension expected for the input images. Supported: {0}".
            format(img_formats))

        grp_func.add_argument(
            "--out-slices-file-name-prefix",
            required=False,
            default=self.out_slices_file_name_prefix,
            type=str,
            help="The prefix for the reconstructed slices files.")

        grp_func.add_argument(
            "--out-horiz-slices-subdir",
            required=False,
            default=self.out_horiz_slices_subdir,
            type=str,
            help="The subdirectory for the reconstructed horizontal slices")

        grp_func.add_argument(
            "--data-dtype",
            required=False,
            default='float32',
            type=str,
            help="The data type in which the data will be processed. Available: float32")

        grp_func.add_argument(
            "-c",
            "--cor",
            required=False,
            type=float,
            default=self.cor,
            help="Provide a pre-calculated centre of rotation. If one is not provided it will be automatically "
                 "calculated "
        )

        grp_func.add_argument(
            "-f",
            "--find-cor",
            action='store_true',
            required=False,
            help="Find the center of rotation (in pixels). rotation around y axis is assumed"
        )

        grp_func.add_argument(
            "-v",
            "--verbosity",
            type=int,
            default=self.verbosity,
            help="Verbosity level. Default: 2."
                 "0 - Silent, no text output at all, except results (not recommended)"
                 "1 - Low verbosity, will output text on each step that is being performed"
                 "2 - Normal verbosity, will output text on each step, including execution time"
                 "3 - High verbosity, will output text on each step, including the name, execution time and memory "
                 "usage before and after each step")

        grp_func.add_argument(
            "--crash-on-failed-import",
            required=False,
            action='store_true',
            default=self.crash_on_failed_import,
            help="Default True, this option tells if the program should stop execution if an import fails and a step "
                 "cannot be executed: True - Raise an exception and stop execution immediately False - Note the "
                 "failure to import but continue execution without applying the filter, WARNING this could corrupt "
                 "the data")

        grp_recon = parser.add_argument_group('Reconstruction options')

        grp_recon.add_argument(
            "-d",
            "--debug",
            required=False,
            action='store_true',
            help='Run debug to specified port, if no port is specified, it will default to 59003')

        grp_recon.add_argument(
            "-p",
            "--debug-port",
            required=False,
            type=int,
            default=self.debug_port,
            help='Port on which a debugger is listening, if no port is specified, it will default to 59003')

        grp_recon.add_argument(
            "-t",
            "--tool",
            required=False,
            type=str,
            default=self.tool,
            help="Tomographic reconstruction tool to use")

        grp_recon.add_argument(
            "-a",
            "--algorithm",
            required=False,
            type=str,
            default=self.algorithm,
            help="Reconstruction algorithm (tool dependent)")

        grp_recon.add_argument(
            "-n",
            "--num-iter",
            required=False,
            type=int,
            default=self.num_iter,
            help="Number of iterations (only valid for iterative methods "
                 "(example: SIRT, ART, etc.).")

        grp_recon.add_argument(
            "--max-angle",
            required=False,
            type=float,
            default=self.max_angle,
            help="Maximum angle (of the last projection), assuming first angle=0, and "
                 "uniform angle increment for every projection (note: this "
                 "is overridden by the angles found in the input FITS headers)")

        return parser

    def update(self, args):
        """
        Should be called after the parser has had a chance to
        parse the real arguments from the user
        """
        self.input_dir = args.input_path
        self.input_dir_flat = args.input_path_flat
        self.input_dir_dark = args.input_path_dark
        self.in_img_format = args.in_img_format

        self.preproc_subdir = args.preproc_subdir
        self.preproc_format = args.preproc_format
        self.preproc_as_stack = args.preproc_as_stack

        self.output_dir = args.output_path
        self.out_img_format = args.out_img_format
        self.out_slices_file_name_prefix = args.out_slices_file_name_prefix
        self.out_horiz_slices_subdir = args.out_horiz_slices_subdir

        import numpy as np

        if args.data_dtype is 'uint16':
            self.data_dtype = np.uint16
        elif args.data_dtype is 'float16':
            self.data_dtype = np.float16
        elif args.data_dtype is 'float32':
            self.data_dtype = np.float32
        elif args.data_dtype is 'float64':
            self.data_dtype = np.float64

        self.debug = args.debug
        self.debug_port = args.debug_port

        if args.cor:
            self.cor = int(args.cor)

        self.find_cor = args.find_cor

        self.verbosity = args.verbosity

        # grab tools options
        self.tool = args.tool
        self.algorithm = args.algorithm
        self.num_iter = args.num_iter
        self.max_angle = args.max_angle
