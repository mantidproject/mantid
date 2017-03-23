class FunctionalConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction
    """

    def __init__(self):
        """
        Builds a default post-processing configuration with a sensible choice of parameters

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
        self.input_path = None
        self.input_path_flat = None
        self.input_path_dark = None
        self.in_format = 'tiff'

        self.output_path = None
        self.out_format = 'tiff'
        self.out_slices_prefix = 'recon_slice'
        self.out_horiz_slices_prefix = 'recon_horiz'
        self.out_horiz_slices_subdir = 'horiz_slices'
        self.save_horiz_slices = False

        # Processing options
        self.save_preproc = False
        self.only_preproc = False
        self.only_postproc = False
        self.reuse_preproc = False
        self.preproc_subdir = 'pre_processed'
        self.swap_axes = False

        import numpy as np

        self.data_dtype = np.float32

        self.cors = None
        self.cor_slices = [0]

        self.verbosity = 3

        self.overwrite_all = False

        self.debug = True
        self.debug_port = None

        # Reconstruction options
        self.tool = 'tomopy'
        self.algorithm = 'gridrec'
        self.num_iter = 5
        self.max_angle = 360.0

        import multiprocessing
        # get max cores on the system as default
        self.cores = multiprocessing.cpu_count()
        # how to spread the image load per worker
        self.chunksize = None
        self.parallel_load = False

        # imopr
        self.imopr = None

        # aggregate
        self.aggregate = None
        self.aggregate_angles = None
        self.aggregate_single_folder_output = None

        # convert
        self.convert = False
        self.convert_prefix = 'converted_images'

        # start the GUI
        self.gui = False

    def __str__(self):
        return "Input directory: {0}\n".format(str(self.input_path)) \
               + "Flat directory: {0}\n".format(str(self.input_path_flat)) \
               + "Dark directory: {0}\n".format(str(self.input_path_dark)) \
               + "Input image format: {0}\n".format(str(self.in_format)) \
               + "Output directory: {0}\n".format(str(self.output_path)) \
               + "Output image format: {0}\n".format(str(self.out_format)) \
               + "Output slices file name prefix: {0}\n".format(str(self.out_slices_prefix)) \
               + "Output horizontal slices file name prefix: {0}\n".format(str(self.out_horiz_slices_prefix)) \
               + "Output horizontal slices subdir: {0}\n".format(str(self.out_horiz_slices_subdir)) \
               + "Save horizontal slices: {0}\n".format(str(self.save_horiz_slices)) \
               + "Save preprocessed images: {0}\n".format(str(self.save_preproc)) \
               + "Do only pre processing and exit: {0}\n".format(str(self.only_preproc)) \
               + "Do only post processing and exit: {0}\n".format(str(self.only_postproc)) \
               + "Reuse preprocessing images: {0}\n".format(str(self.reuse_preproc)) \
               + "Pre processing images subdir: {0}\n".format(str(self.preproc_subdir)) \
               + "Radiograms: {0}\n".format(str(self.swap_axes)) \
               + "Data type: {0}\n".format(str(self.data_dtype)) \
               + "Provided center of rotation: {0}\n".format(str(self.cors)) \
               + "Slice IDs for CORs: {0}\n".format(str(self.cor_slices)) \
               + "Verbosity: {0}\n".format(str(self.verbosity)) \
               + "Overwrite files in output directory: {0}\n".format(str(self.overwrite_all)) \
               + "Debug: {0}\n".format(str(self.debug)) \
               + "Debug port: {0}\n".format(str(self.debug_port)) \
               + "Tool: {0}\n".format(str(self.tool)) \
               + "Algorithm: {0}\n".format(str(self.algorithm)) \
               + "Number of iterations: {0}\n".format(str(self.num_iter)) \
               + "Maximum angle: {0}\n".format(str(self.max_angle)) \
               + "Cores: {0}\n".format(str(self.cores)) \
               + "Chunk per worker: {0}\n".format(str(self.chunksize)) \
               + "Load data in parallel: {0}\n".format(str(self.parallel_load)) \
               + "Image operator mode: {0}\n".format(str(self.imopr)) \
               + "Aggregate mode: {0}\n".format(str(self.aggregate)) \
               + "Aggregate angles: {0}\n".format(str(self.aggregate_angles)) \
               + "Aggregate single folder output: {0}\n".format(str(self.aggregate_single_folder_output)) \
               + "Convert images mode: {0}\n".format(str(self.convert)) \
               + "Prefix for the output converted images: {0}\n".format(str(self.convert_prefix)) \
               + "Running the GUI: {0}\n".format(str(self.gui))

    def setup_parser(self, parser):
        """
        Setup the functional arguments for the script
        :param parser: The parser which is set up
        """
        grp_func = parser.add_argument_group('Functionality options')

        grp_func.add_argument(
            "-i",
            "--input-path",
            required=True,
            type=str,
            help="Input directory",
            default=self.input_path)

        grp_func.add_argument(
            "-F",
            "--input-path-flat",
            required=False,
            default=self.input_path_flat,
            type=str,
            help="Input directory for flat images")

        grp_func.add_argument(
            "-D",
            "--input-path-dark",
            required=False,
            default=self.input_path_dark,
            type=str,
            help="Input directory for flat images")

        from imgdata import loader
        grp_func.add_argument(
            "--in-format",
            required=False,
            default=self.out_format,
            type=str,
            choices=loader.supported_formats(),
            help="Format/file extension expected for the input images.")

        grp_func.add_argument(
            "-o",
            "--output-path",
            required=False,
            default=self.output_path,
            type=str,
            help="Where to write the output slice images (reconstructed volume)."
        )

        from imgdata.saver import Saver
        grp_func.add_argument(
            "--out-format",
            required=False,
            default=self.out_format,
            type=str,
            choices=Saver.supported_formats(),
            help="Format/file extension expected for the input images.")

        grp_func.add_argument(
            "--out-slices-prefix",
            required=False,
            default=self.out_slices_prefix,
            type=str,
            help="The prefix for the reconstructed slices files.")

        grp_func.add_argument(
            "--out-horiz-slices-subdir",
            required=False,
            default=self.out_horiz_slices_subdir,
            type=str,
            help="The subdirectory for the reconstructed horizontal slices.")

        grp_func.add_argument(
            "--out-horiz-slices-prefix",
            required=False,
            default=self.out_horiz_slices_prefix,
            type=str,
            help="The prefix for the reconstructed horizontal slices files.")

        grp_func.add_argument(
            "-s",
            "--save-preproc",
            required=False,
            action='store_true',
            help="Save out the pre-processed images.")

        grp_func.add_argument(
            "--only-preproc",
            required=False,
            action='store_true',
            help="Complete pre-processing of images and exit.")

        grp_func.add_argument(
            "--reuse-preproc",
            required=False,
            action='store_true',
            help="The images loaded have already been pre-processed. All pre-processing steps will be skipped."
        )

        grp_func.add_argument(
            "--only-postproc",
            required=False,
            action='store_true',
            help="The images have already been reconstructed. All pre-processing and reconstruciton steps will be skipped."
        )

        grp_func.add_argument(
            "--save-horiz-slices",
            required=False,
            action='store_true',
            help="Save out the horizontal reconstructed files.")

        grp_func.add_argument(
            "-p",
            "--preproc-subdir",
            required=False,
            type=str,
            default=self.preproc_subdir,
            help="The subdirectory for the pre-processed images.\nDefault output-path/pre_processed/."
        )

        grp_func.add_argument(
            "--swap-axes",
            required=False,
            action='store_true',
            default=self.swap_axes,
            help="NOT RECOMMENDED: This means an additional conversion will be done inside Tomopy, which will double the memory usage temporarily."
            "\nPre-processed images will be saved as swap_axes if --save-preproc is specified."
            "\nIf --reuse-preproc is specified, then the images that will be loaded will be expected to be swap_axes."
        )

        grp_func.add_argument(
            "--data-dtype",
            required=False,
            default='float32',
            type=str,
            help="The data type in which the data will be processed.\nSupported: float32, float64"
        )

        grp_func.add_argument(
            "-v",
            "--verbosity",
            type=int,
            default=self.verbosity,
            help="0 - Silent, no text output at all, except results (not recommended)\n"
            "1 - Low verbosity, will output text on step name\n"
            "2 - Normal verbosity, will output step name and execution time\n"
            "3 - High verbosity, will output step name, execution time and memory usage before and after each step\n"
            "Default: 2 - Normal verbosity.")

        grp_func.add_argument(
            "-w",
            "--overwrite-all",
            required=False,
            action='store_true',
            default=self.overwrite_all,
            help="Overwrite all conflicting files found in the output directory."
        )
        grp_func.add_argument(
            "--cores",
            required=False,
            type=int,
            default=self.cores,
            help="Number of CPU cores that will be used for reconstruction.")

        grp_func.add_argument(
            "--chunksize",
            required=False,
            type=int,
            default=self.chunksize,
            help="How to spread the load on each worker.")

        grp_func.add_argument(
            "--parallel-load",
            required=False,
            action='store_true',
            default=self.parallel_load,
            help="Load the data with multiple reader processes. This CAN MAKE THE LOADING slower on a single local Hard Disk Drive."
        )

        grp_run_modes = parser.add_argument_group('Run Modes')

        grp_run_modes.add_argument(
            "--convert",
            required=False,
            action='store_true',
            default=self.convert,
            help='Convert images to a different format.')

        grp_run_modes.add_argument(
            "--convert-prefix",
            required=False,
            type=str,
            default=self.convert_prefix,
            help='Prefix for saved out files from conversion.')

        from imopr import imopr
        grp_run_modes.add_argument(
            "--imopr",
            nargs='*',
            required=False,
            type=str,
            default=self.imopr,
            help='Image operator currently supports the following operators: '
            + str(imopr.get_available_operators()))

        grp_run_modes.add_argument(
            "--aggregate",
            nargs='*',
            required=False,
            type=str,
            default=self.aggregate,
            help='Aggregate the selected image energy levels. The expected input is '
            '--aggregate <start> <end> <method:{sum, avg}>... to select indices.\n\
                  There must always be an even lenght of indices: --aggregate 0 100 101 201 300 400 sum'
        )

        grp_run_modes.add_argument(
            "--aggregate-angles",
            nargs='*',
            required=False,
            type=str,
            default=self.aggregate_angles,
            help="Select which angles to be aggregated with --aggregate.\n"
            "This can be used to spread out the load on multiple nodes.\n"
            "Sample command: --aggregate-angles 0 10, will select only angles 0 - 10 inclusive."
        )

        grp_run_modes.add_argument(
            "--aggregate-single-folder-output",
            action='store_true',
            required=False,
            default=self.aggregate_single_folder_output,
            help='The output will be images with increasing number in a single folder.'
        )

        grp_run_modes.add_argument(
            "--debug",
            required=False,
            action='store_true',
            help='Run debug to specified port, if no port is specified, it will default to 59003'
        )

        grp_run_modes.add_argument(
            "--debug-port",
            required=False,
            type=int,
            default=self.debug_port,
            help='Port on which a debugger is listening.')

        grp_run_modes.add_argument(
            "--gui",
            required=False,
            action='store_true',
            default=self.gui,
            help='Start up the GUI.')

        grp_recon = parser.add_argument_group('Reconstruction options')

        supported_tools = ['tomopy', 'astra']
        grp_recon.add_argument(
            "-t",
            "--tool",
            required=False,
            type=str,
            default=self.tool,
            choices=supported_tools,
            help="Tomographic reconstruction tool to use.\nAvailable: {0}".
            format(supported_tools))

        from tools.tomopy_tool import TomoPyTool
        from tools.astra_tool import AstraTool

        tomo_algs = TomoPyTool.tool_supported_methods()
        astra_algs = AstraTool.tool_supported_methods()
        grp_recon.add_argument(
            "-a",
            "--algorithm",
            required=False,
            type=str,
            default=self.algorithm,
            help="Reconstruction algorithm (tool dependent).\nAvailable:\nTomoPy: {0}\nAstra:{1}".
            format(tomo_algs, astra_algs))

        grp_recon.add_argument(
            "-n",
            "--num-iter",
            required=False,
            type=int,
            default=self.num_iter,
            help="Number of iterations(only valid for iterative methods: {'art', 'bart', 'mlem', 'osem', "
            "'ospml_hybrid', 'ospml_quad', pml_hybrid', 'pml_quad', 'sirt'}.")

        grp_recon.add_argument(
            "--max-angle",
            required=False,
            type=float,
            default=self.max_angle,
            help="Maximum angle of the last projection.\n"
            "Assuming first angle=0, and uniform angle increment for every projection"
        )

        grp_recon.add_argument(
            "-c",
            "--cors",
            required=False,
            nargs='*',
            type=str,  # this is string but will be later converted to floats in self.update()
            default=self.cors,
            help="Provide the CORs for the selected slices with --cor-slices.\n"
            "If no slices are provided a SINGLE COR is expected, that will be used for the whole stack.\n"
            "If slices are provided, the number of CORs provided with this option MUST BE THE SAME as the slices."
        )
        grp_recon.add_argument(
            "--cor-slices",
            required=False,
            nargs='*',
            type=str,  # this is string but will be later converted to ints in self.update()
            default=self.cor_slices,
            help="Specify the Slice IDs to which the centers of rotation from --cors correspond.\n"
            "The number of slices passed here MUST be the same as the number of CORs provided.\n"
            "The slice IDs MUST be ints. If no slice IDs are provided, then only 1 COR is expected and will be used for the whole stack."
        )

        return parser

    def update(self, args):
        """
        Should be called after the parser has had a chance to
        parse the real arguments from the user.

        SPECIAL CASES ARE HANDLED IN:
        recon_config.ReconstructionConfig.handle_special_arguments
        """
        self.input_path = args.input_path
        self.input_path_flat = args.input_path_flat
        self.input_path_dark = args.input_path_dark
        self.in_format = args.in_format

        self.output_path = args.output_path
        self.out_format = args.out_format
        self.out_slices_prefix = args.out_slices_prefix
        self.out_horiz_slices_prefix = args.out_horiz_slices_prefix
        self.out_horiz_slices_subdir = args.out_horiz_slices_subdir
        self.save_horiz_slices = args.save_horiz_slices

        self.save_preproc = args.save_preproc
        self.only_preproc = args.only_preproc
        self.reuse_preproc = args.reuse_preproc
        self.only_postproc = args.only_postproc
        self.preproc_subdir = args.preproc_subdir
        self.swap_axes = args.swap_axes

        import numpy as np

        if args.data_dtype == 'float32':
            self.data_dtype = np.float32
        elif args.data_dtype == 'float64':
            self.data_dtype = np.float64

        # float16, uint16 data types produce exceptions
        # > float 16 - scipy median filter does not support it
        # > uint16 -  division is wrong, so all values become 0 and 1
        # could convert to float16, but then we'd have to go up to
        # float32 for the median filter anyway

        self.debug = args.debug
        self.debug_port = args.debug_port

        if args.cors:
            self.cors = [float(cor) for cor in args.cors]
        if args.cor_slices:
            self.cor_slices = [int(slice_id) for slice_id in args.cor_slices]

        self.verbosity = args.verbosity
        self.overwrite_all = args.overwrite_all

        # grab tools options
        self.tool = args.tool
        self.algorithm = args.algorithm
        self.num_iter = args.num_iter
        self.max_angle = args.max_angle
        self.cores = args.cores
        self.chunksize = args.chunksize
        self.parallel_load = args.parallel_load
        self.convert = args.convert
        self.convert_prefix = args.convert_prefix
        self.imopr = args.imopr
        self.aggregate = args.aggregate
        self.aggregate_angles = args.aggregate_angles
        self.aggregate_single_folder_output = args.aggregate_single_folder_output
        self.gui = args.gui
