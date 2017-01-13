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

        self.input_dir = None
        self.input_dir_flat = None
        self.input_dir_dark = None
        self.in_img_format = 'fits'

        self.preproc_images_subdir = 'pre_processed'
        self.preproc_images_format = 'fits'
        self.preproc_images_as_stack = True

        self.output_dir = None
        self.out_img_format = 'fits'
        self.out_slices_file_name_prefix = 'out_recon_slice'
        self.out_horiz_slices_subdir = 'out_recon_horiz_slice'

        self.debug = True
        self.debug_port = 59003

        import numpy as np
        # TODO more tests with float16/float64/uint16
        self.data_dtype = np.float32

        self.cor = None
        self.find_cor = None

        # TODO test verbosity works properly on each level
        self.verbosity = 3  # default 2
        self.crash_on_failed_import = True  # default True

        self.tool = 'tomopy'
        self.algorithm = 'gridrec'
        self.num_iter = None
        self.regularization = None
        self.max_angle = 360.0

    def __str__(self):
        return\
            "Input dir: {0}\n" \
            "Flat dir: {1}\n" \
            "Dark dir: {2}\n" \
            "In image format: {3}" \
            "Pre processing images subdir: {4}" \
            "Pre processing images format: {5}" \
            "Pre processing images as stack: {6}" \
            "Output dir: {7}\n" \
            "Output image format: {8}" \
            "Output slices file name prefix: {9}" \
            "Output horizontal slices subdir: {10}" \
            "Debug: {11}" \
            "Debug port: {12}" \
            "Data dtype: {13}" \
            "Argument COR: {14}" \
            "Find COR Run: {15}" \
            "Verbosity: {16}" \
            "Crash on failed import: {17}" \
            "Tool: {18}" \
            "Algorithm: {19}" \
            "Number of iterations: {20}" \
            "Maximum angle: {21}"\
            .format(
                str(self.input_dir),
                str(self.input_dir_flat),
                str(self.input_dir_dark),
                str(self.in_img_format),
                str(self.preproc_images_subdir),
                str(self.preproc_images_format),
                str(self.preproc_images_as_stack),
                str(self.output_dir),
                str(self.out_img_format),
                str(self.out_slices_file_name_prefix),
                str(self.out_horiz_slices_subdir),
                str(self.debug),
                str(self.debug_port),
                str(self.data_dtype),
                str(self.cor),
                str(self.find_cor),
                str(self.verbosity),
                str(self.crash_on_failed_import),
                str(self.tool),
                str(self.algorithm),
                str(self.num_iter),
                str(self.regularization),
                str(self.max_angle)
            )

    def setup_parser(self, parser):
        """
                Setup the functional arguments for the script
                :param parser: The parser which is set up
                """
        grp_req = parser.add_argument_group(
            'Arguments for the functionality of the script')

        grp_req.add_argument(
            "-i", "--input-path", required=True, type=str, help="Input directory", default=self.input_dir)

        grp_req.add_argument(
            "-iflat"
            "--input-path-flat",
            required=False,
            default=self.input_dir_flat,
            type=str,
            help="Input directory for flat images")

        grp_req.add_argument(
            "-idark"
            "--input-path-dark",
            required=False,
            default=self.input_dir_dark,
            type=str,
            help="Input directory for flat images")

        grp_req.add_argument(
            "-o",
            "--output-path",
            required=True,
            default=self.output_dir,
            type=str,
            help="Where to write the output slice images (reconstructed volume)")

        img_formats = ['tiff', 'fits', 'tif', 'fit', 'png']
        grp_pre.add_argument(
            "--in-img-format",
            required=False,
            default='fits',
            type=str,
            help="Format/file extension expected for the input images. Supported: {0}".
            format(img_formats))

        grp_pre.add_argument(
            "--out-img-format",
            required=False,
            default='tiff',
            type=str,
            help="Format/file extension expected for the input images. Supported: {0}".
            format(img_formats))

        grp_req.add_argument(
            "-c",
            "--cor",
            required=False,
            type=float,
            default=self.cor,
            help="Provide a pre-calculated centre of rotation. If one is not provided it will be automatically "
                 "calculated "
        )

        grp_req.add_argument(
            "-f",
            "--find-cor",
            action='store_true',
            required=False,
            help="Find the center of rotation (in pixels). rotation around y axis is assumed"
        )

        grp_req.add_argument(
            "-v",
            "--verbosity",
            action="count",
            default=self.verbosity,
            help="Verbosity level. Default: 2."
                 "0 - Silent, no text output at all (not recommended)"
                 "1 - Low verbosity, will output text on each step that is being performed"
                 "2 - Normal verbosity, will output text on each step, including execution time"
                 "3 - High verbosity, will output text on each step, including the name, execution time and memory "
                 "usage before and after each step")

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

        grp_recon.add_argument(
            "--crash-on-failed-import",
            required=False,
            action='store_true',
            default=self.crash_on_failed_import,
            help="Default True, this option tells if the program should stop execution if an import fails and a step "
                 "cannot be executed: True - Raise an exception and stop execution immediately False - Note the "
                 "failure to import but continue execution without applying the filter, WARNING this could corrupt "
                 "the data")

        return parser
