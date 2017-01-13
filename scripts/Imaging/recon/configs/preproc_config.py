# pylint: disable=too-many-instance-attributes
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
        self.normalize_air_region = None
        self.crop_before_normalize = None
        self.median_filter_size = 3
        """
        :param median_filter_mode: Default: 'reflect', {'reflect', 'constant', 'nearest', 'mirror', 'wrap'}, optional
            The mode parameter determines how the array borders are handled, where cval is the value when
            mode is equal to 'constant'.
        """
        self.median_filter_mode = 'reflect'
        self.stripe_removal_method = 'wavelet-fourier'

        # Rotation 90 degrees clockwise (positive) or counterclockwise (negative)
        # Example: -1 => (-90 degrees == 90 degrees counterclockwise)
        self.rotation = False

        # clip normalisation values
        self.clip_min = 0
        self.clip_max = 1.5

        # list with coordinates of the region for normalization / "air" / not
        # blocked by any object
        # self.normalize_proton_charge = False

        self.cut_off_level_pre = 0  # TODO unused
        self.mcp_corrections = True
        self.scale_down = 0

        self.line_projection = True  # TODO unused

    def __str__(self):
        return "Region of interest (crop coordinates): {0}\n".format(self.region_of_interest) \
               + "Normalize by air region: {0}\n".format(self.normalize_air_region) \
               + "Cut-off on normalized images: {0}\n".format(self.cut_off_level_pre) \
               + "Corrections for MCP detector: {0}\n".format(self.mcp_corrections) \
               + "Scale down factor for images: {0}\n".format(self.scale_down) \
               + "Median filter width: {0}\n".format(self.median_filter_size) \
               + "Rotation: {0}\n".format(self.rotation) \
               + "Line projection (line integral/log re-scale): {0}\n".format(self.line_projection) \
               + "Sinogram stripes removal: {0}".format(self.stripe_removal_method) \
               + "Clip min value: {0}".format(self.clip_min) \
               + "Clip max value: {0}".format(self.clip_max)

    def setup_parser(self, parser):
        """
        Setup the pre-processing arguments for the script
        :param parser: The parser which is set up
        """

        grp_pre = parser.add_argument_group(
            'Pre-processing of input raw images/projections')

        grp_pre.add_argument(
            "--region-of-interest",
            required=False,
            type=str,
            help="Region of interest (crop original "
                 "images to these coordinates, given as comma separated values: x1,y1,x2,y2. If not "
                 "given, the whole images are used.")

        grp_pre.add_argument(
            "--air-region",
            required=False,
            type=str,
            help="Air region /region for normalization. "
                 "If not provided, the normalization against beam intensity fluctuations in this "
                 "region will not be performed. list with coordinates of the region for normalization "
                 "by 'air', must not be blocked by any object.")

        grp_pre.add_argument(
            "--crop-before-normalize",
            required=False,
            action='store_true',
            help="Default: True, If True crop before normalizing by flat/dark or air region. \
                    Cropping first could improve performance and reduce memory usage, as this allows \
                    the algorithms to work on smaller data, as they will be cropped.")

        grp_pre.add_argument(
            "--median-filter-size",
            type=int,
            required=False,
            default=self.median_filter_size,
            help="Size/width of the median filter (pre-processing")

        grp_pre.add_argument(
            "--median-filter-mode",
            type=str,
            required=False,
            default=self.median_filter_mode,
            help="Type of median filter. Default: 'reflect', available: "
                 "{'reflect', 'constant', 'nearest', 'mirror', 'wrap'}"
        )

        grp_pre.add_argument(
            "--remove-stripes",
            default='wf',
            required=False,
            type=str,
            help="Methods supported: 'wf' (Wavelet-Fourier)")

        grp_pre.add_argument(
            "-r"
            "--rotation",
            required=False,
            type=int,
            help="Rotate images by 90 degrees a number of "
                 "times. The rotation is clockwise unless a negative number is given which indicates "
                 "rotation counterclockwise")

        grp_pre.add_argument(
            "--clip-min",
            required=False,
            type=float,
            default=self.clip_min,
            help="Clip values after normalisations to remove out of bounds pixel values. "
            "Default for min is 0, default for max is 1.5"
        )

        grp_pre.add_argument(
            "--clip-min",
            required=False,
            type=float,
            default=self.clip_max,
            help="Clip values after normalisations to remove out of bounds pixel values. "
            "Default for min is 0, default for max is 1.5"
        )

        grp_pre.add_argument(
            "--cut-off-pre",
            required=False,
            type=float,
            help="Cut off level (percentage) for pre processed "
                 "volume. pixels below this percentage with respect to maximum intensity in the stack "
                 "will be set to the minimum value.")

        grp_pre.add_argument(
            "--scale-down",
            required=False,
            type=int,
            help="Scale down factor, to reduce the size of "
                 "the images for faster (lower-resolution) reconstruction. For example a factor of 2 "
                 "reduces 1kx1k images to 512x512 images (combining blocks of 2x2 pixels into a single "
                 "pixel. The output pixels are calculated as the average of the input pixel blocks."
        )

        grp_pre.add_argument(
            "-m"
            "--mcp-corrections",
            required=False,
            action='store_true',
            help="Perform corrections specific to images taken with the MCP detector"
        )

        return parser

    def update(self, args):
        self.region_of_interest = args.region_of_interest
        self.normalize_air_region = args.air_region
        self.crop_before_normalize = args.crop_before_normalize
        self.median_filter_size = args.median_filter_size
        self.median_filter_mode = args.median_filter_mode
        self.stripe_removal_method = args.remove_stripes

        self.rotation = args.rotation

        self.clip_min = args.clip_min
        self.clip_max = args.clip_max

        self.cut_off_level_pre = args.cut_off_pre
        self.mcp_corrections = args.mcp_corrections
        self.scale_down = args.scale_down

        # self.line_projection = args.line_projection
