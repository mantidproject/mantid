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

        self.stripe_removal_wf = None
        self.stripe_removal_ti = None
        self.stripe_removal_sf = None

        # Rotation 90 degrees clockwise (positive) or counterclockwise (negative)
        # Example: -1 => (-90 degrees == 90 degrees counterclockwise)
        self.rotation = False

        # clip normalisation values
        self.clip_min = 0.0
        self.clip_max = 1.5

        self.cut_off = None
        # list with coordinates of the region for normalisation / "air" / not
        # blocked by any object
        # self.normalise_proton_charge = False

        self.outliers_threshold = None
        self.outliers_radius = None
        self.mcp_corrections = True
        self.rebin = None
        self.rebin_mode = 'bilinear'

        self.minus_log = False
        # self.line_projection = True  # TODO unused

    def __str__(self):
        return "Region of interest (crop coordinates): {0}\n".format(self.region_of_interest) \
               + "Normalise by air region: {0}\n".format(self.normalise_air_region) \
               + "Median filter kernel size: {0}\n".format(self.median_size) \
               + "Median filter edges mode: {0}\n".format(self.median_mode) \
               + "Gaussian filter kernel size: {0}\n".format(self.gaussian_size) \
               + "Gaussian filter edges mode: {0}\n".format(self.gaussian_mode) \
               + "Gaussian filter order: {0}\n".format(self.gaussian_order) \
               + "Sinogram stripe removal using wavelett-fourier: {0}\n".format(self.stripe_removal_wf) \
               + "Sinogram stripe removal using Titarenko approach: {0}\n".format(self.stripe_removal_ti) \
               + "Sinogram stripe removal using smoothing-filter: {0}\n".format(self.stripe_removal_sf) \
               + "Rotation: {0}\n".format(self.rotation) \
               + "Clip min value: {0}\n".format(self.clip_min) \
               + "Clip max value: {0}\n".format(self.clip_max) \
               + "Cut off level: {0}\n".format(self.cut_off) \
               + "Outliers threshold: {0}\n".format(self.outliers_threshold) \
               + "Outliers mode: {0}\n".format(self.outliers_radius) \
               + "Corrections for MCP detector: {0}\n".format(self.mcp_corrections) \
               + "Rebin down factor for images: {0}\n".format(self.rebin) \
               + "Rebin mode: {0}\n".format(self.rebin_mode) \
               + "Minus log on images: {0}".format(self.minus_log)

    def setup_parser(self, parser):
        """
        Setup the pre-processing arguments for the script
        :param parser: The parser which is set up
        """

        grp_pre = parser.add_argument_group(
            'Pre-processing of input raw images/projections')

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

        return parser

    def update(self, args):
        """
        SPECIAL CASES ARE HANDLED IN:
        recon_config.ReconstructionConfig.handle_special_arguments
        """
        if args.region_of_interest:
            if len(args.region_of_interest) < 4:
                raise ValueError(
                    "Not enough arguments provided for the Region of Interest! Expecting 4, but found {0}: {1}"
                    .format(
                        len(args.region_of_interest), args.region_of_interest))

            self.region_of_interest = [
                int(val) for val in args.region_of_interest
            ]

        if args.air_region:
            if len(args.air_region) < 4:
                raise ValueError(
                    "Not enough arguments provided for the Region of Interest! Expecting 4, but found {0}: {1}"
                    .format(len(args.air_region), args.air_region))

            self.normalise_air_region = [int(val) for val in args.air_region]

        self.median_size = args.pre_median_size
        self.median_mode = args.pre_median_mode

        self.gaussian_size = args.pre_gaussian_size
        self.gaussian_mode = args.pre_gaussian_mode
        self.gaussian_order = args.pre_gaussian_order

        self.stripe_removal_wf = args.pre_stripe_removal_wf
        self.stripe_removal_ti = args.pre_stripe_removal_ti
        self.stripe_removal_sf = args.pre_stripe_removal_sf

        self.rotation = args.rotation

        self.clip_min = args.clip_min
        self.clip_max = args.clip_max

        self.cut_off = args.cut_off

        self.outliers_threshold = args.pre_outliers
        self.outliers_radius = args.pre_outliers_radius
        # self.mcp_corrections = args.mcp_corrections
        self.rebin = args.rebin
        self.rebin_mode = args.rebin_mode

        self.minus_log = args.pre_minus_log

        # self.line_projection = args.line_projection
