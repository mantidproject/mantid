class PreProcConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction.

    Options like the stripe removal, MCP correction, or even the median filter would
    better be handled as plugins. For the time being we just have a panel with a fixed
    set of options to enable/disable/configure
    """

    def __init__(self):
        # defaults that look sensible for the MCP detector:
        # median_filter=3, rotate=-1, crop=[0,  252, 0, 512], MCP correction:
        # on

        # all coords outside the ROI will be cropped
        self.region_of_interest = None
        self.normalise_air_region = None
        self.crop_before_normalise = None
        self.median_size = None
        """
        :param median_mode: Default: 'reflect', {'reflect', 'constant', 'nearest', 'mirror', 'wrap'}, optional
            The mode parameter determines how the array borders are handled, where cval is the value when
            mode is equal to 'constant'.
        """
        self.median_mode = 'reflect'

        self.gaussian_size = None
        self.gaussian_mode = 'reflect'
        self.gaussian_order = 0

        self.stripe_removal_method = 'wavelet-fourier'

        # Rotation 90 degrees clockwise (positive) or counterclockwise (negative)
        # Example: -1 => (-90 degrees == 90 degrees counterclockwise)
        self.rotation = False

        # clip normalisation values
        self.clip_min = 0.0
        self.clip_max = 1.5

        # list with coordinates of the region for normalisation / "air" / not
        # blocked by any object
        # self.normalise_proton_charge = False

        self.outliers_threshold = None
        self.outliers_mode = None
        self.mcp_corrections = True
        self.rebin = None
        self.rebin_mode = 'bilinear'

        self.line_projection = True  # TODO unused

    def __str__(self):
        return "Region of interest (crop coordinates): {0}\n".format(self.region_of_interest) \
               + "Normalise by air region: {0}\n".format(self.normalise_air_region) \
               + "Cut-off on normalised images: {0}\n".format(self.outliers_threshold) \
               + "Corrections for MCP detector: {0}\n".format(self.mcp_corrections) \
               + "rebin down factor for images: {0}\n".format(self.rebin) \
               + "Median filter width: {0}\n".format(self.median_size) \
               + "Rotation: {0}\n".format(self.rotation) \
               + "Line projection (line integral/log re-rebin): {0}\n".format(self.line_projection) \
               + "Sinogram stripes removal: {0}\n".format(self.stripe_removal_method) \
               + "Clip min value: {0}\n".format(self.clip_min) \
               + "Clip max value: {0}\n".format(self.clip_max)

    def setup_parser(self, parser):
        """
        Setup the pre-processing arguments for the script
        :param parser: The parser which is set up
        """

        grp_pre = parser.add_argument_group(
            'Pre-processing of input raw images/projections')

        grp_pre.add_argument(
            "-R",
            "--region-of-interest",
            nargs='*',
            required=False,
            type=str,
            help="Crop original images using these coordinates, after rotating the images.\n"
                 "If not given, the whole images are used.\n"
                 "Example: --region-of-interest='[150,234,23,22]'.")

        grp_pre.add_argument(
            "-A",
            "--air-region",
            required=False,
            nargs='*',
            type=str,
            help="Air region /region for normalisation.\n"
                 "For best results it should avoid being blocked by any object.\n"
                 "Example: --air-region='[150,234,23,22]'")

        grp_pre.add_argument(
            "--crop-before-normalise",
            required=False,
            action='store_true',
            help="Crop before doing any normalisations on the images.\n"
                 "This improves performance and reduces memory usage, as"
                 "the algorithms will work on smaller data.")

        grp_pre.add_argument(
            "--pre-median-size",
            type=int,
            required=False,
            default=self.median_size,
            help="Size / width of the median filter(pre - processing)."
        )

        median_modes = ['reflect', 'constant', 'nearest', 'mirror', 'wrap']
        grp_pre.add_argument(
            "--pre-median-mode",
            type=str,
            required=False,
            default=self.median_mode,
            choices=median_modes,
            help="Default: %(default)s\n"
                 "Mode of median filter which determines how the array borders are handled."

        )

        grp_pre.add_argument(
            "--remove-stripes",
            default='wf',
            required=False,
            type=str,
            help="Methods supported: 'wf' (Wavelet-Fourier).")

        grp_pre.add_argument(
            "-r",
            "--rotation",
            required=False,
            type=int,
            help="Rotate images by 90 degrees a number of times.\n"
                 "The rotation is clockwise unless a negative number is given which indicates "
                 "rotation counterclockwise.")

        grp_pre.add_argument(
            "--clip-min",
            required=False,
            type=float,
            default=self.clip_min,
            help="Default: %(default)s\n"
            "Clip values after normalisations to remove out of bounds pixel values."
        )

        grp_pre.add_argument(
            "--clip-max",
            required=False,
            type=float,
            default=self.clip_max,
            help="Default: %(default)s\n"
            "Clip values after normalisations to remove out of bounds pixel values."
        )

        grp_pre.add_argument(
            "--pre-outliers",
            required=False,
            type=float,
            help="Outliers threshold for pre-processed images.\n"
                 "Pixels below this threshold with respect to maximum intensity in the stack "
                 "will be set to the minimum value.")

        outliers_mode = ['dark', 'bright', 'both']
        grp_pre.add_argument(
            "--pre-outliers-mode",
            required=False,
            type=str,
            choices=outliers_mode,
            help="Which pixels to clip, only dark ones, bright ones or both.")

        grp_pre.add_argument(
            "--rebin",
            required=False,
            type=float,
            help="Rebin factor by which the images will be rebinned. This could be any positive float number.\n"
            "If not specified no scaling will be done."
        )

        rebin_modes = ['nearest', 'lanczos', 'bilinear', 'bicubic', 'cubic']
        grp_pre.add_argument(
            "--rebin-mode",
            required=False,
            type=str,
            default=self.rebin_mode,
            choices=rebin_modes,
            help="Default: %(default)s\n"
            "Specify which interpolation mode will be used for the scaling of the image."
        )

        grp_pre.add_argument(
            "-m",
            "--mcp-corrections",
            required=False,
            action='store_true',
            help="Perform corrections specific to images taken with the MCP detector.")

        grp_pre.add_argument(
            "--pre-gaussian-size",
            required=False,
            type=float,
            default=self.gaussian_size,
            help="Apply gaussian filter (2d) on reconstructed volume with the given window size."
        )

        grp_pre.add_argument(
            "--pre-gaussian-mode",
            type=str,
            required=False,
            default=self.gaussian_mode,
            choices=median_modes,
            help="Default: %(default)s\nMode of gaussian filter which determines how the array borders are handled.(pre processing).")

        grp_pre.add_argument(
            "--pre-gaussian-order",
            required=False,
            type=int,
            default=self.gaussian_order,
            help="Default: %(default)d\nThe order of the filter along each axis is given as a sequence of integers, \n"
            "or as a single number. An order of 0 corresponds to convolution with a Gaussian kernel.\n"
            "An order of 1, 2, or 3 corresponds to convolution with the first, second or third derivatives of a Gaussian.\n"
            "Higher order derivatives are not implemented.")

        return parser

    def update(self, args):

        if args.region_of_interest:
            if len(args.region_of_interest) < 4:
                raise ValueError(
                    "Not enough arguments provided for the Region of Interest! Expecting 4, but found {0}: {1}"
                    .format(len(args.region_of_interest), args.region_of_interest))

            self.region_of_interest = [int(val)
                                       for val in args.region_of_interest]

        if args.air_region:
            if len(args.air_region) < 4:
                raise ValueError(
                    "Not enough arguments provided for the Region of Interest! Expecting 4, but found {0}: {1}"
                    .format(len(args.air_region), args.air_region))

            self.normalise_air_region = [int(val) for val in args.air_region]

        self.crop_before_normalise = args.crop_before_normalise
        self.median_size = args.pre_median_size
        self.median_mode = args.pre_median_mode

        self.gaussian_size = args.pre_gaussian_size
        self.gaussian_mode = args.pre_gaussian_mode
        self.gaussian_order = args.pre_gaussian_order

        self.stripe_removal_method = args.remove_stripes

        self.rotation = args.rotation

        self.clip_min = args.clip_min
        self.clip_max = args.clip_max

        self.outliers_threshold = args.pre_outliers
        self.outliers_mode = args.pre_outliers_mode
        self.mcp_corrections = args.mcp_corrections
        self.rebin = args.rebin
        self.rebin_mode = args.rebin_mode

        # self.line_projection = args.line_projection
