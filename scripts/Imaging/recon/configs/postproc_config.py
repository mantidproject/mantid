class PostProcConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction
    """

    def __init__(self):
        """
        Builds a default post-processing configuration with a sensible choice of parameters
        """
        self.circular_mask = 0.94
        self.cut_off_level_post = 0
        self.gaussian_filter_size = 0
        self.gaussian_filter_mode = 0

        self.median_filter_size = 0
        """
        :param median_filter_mode: Default: 'reflect', {'reflect', 'constant', 'nearest', 'mirror', 'wrap'}, optional
            The mode parameter determines how the array borders are handled, where cval is the value when
            mode is equal to 'constant'.
        """
        self.median_filter_mode = 'reflect'
        self.median_filter3d_size = 0  # TODO unused

    def __str__(self):
        return "Circular mask: {0}\n".format(self.circular_mask) \
            + "Cut-off on reconstructed volume: {0}\n".format(self.cut_off_level_post) \
            + "Gaussian filter size: {0}\n".format(self.gaussian_filter_size) \
            + "Gaussian filter mode: {0}\n".format(self.gaussian_filter_mode) \
            + "Median filter size:: {0}\n".format(self.median_filter_size) \
            + "Median filter mode: {0}\n".format(self.median_filter_mode)

    def setup_parser(self, parser):
        """
        Setup the post-processing arguments for the script
        :param parser: The parser which is set up
        """
        grp_post = parser.add_argument_group(
            'Post-processing of the reconstructed volume')

        grp_post.add_argument(
            "--circular-mask",
            required=False,
            type=float,
            default=self.circular_mask,
            help="Radius of the circular mask to apply on the reconstructed volume.\n"
                 "It is given in [0,1] relative to the size of the smaller dimension/edge "
                 "of the slices.\nEmpty or zero implies no masking.")

        grp_post.add_argument(
            "--cut-off-post",
            required=False,
            type=float,
            help="Cut off level (percentage) for reconstructed volume.\n"
            "Pixels below this percentage with respect to maximum intensity in the stack "
                 "will be set to the minimum value.")

        grp_post.add_argument(
            "--out-median-filter-size",
            required=False,
            type=float,
            default=self.median_filter_size,
            help="Apply median filter (2d) on reconstructed volume with the given window size.(post processing)"
        )

        grp_post.add_argument(
            "--out-median-filter-mode",
            type=str,
            required=False,
            default=self.median_filter_mode,
            help="Mode of median filter which determines how the array borders are handled.(post processing)\n"
                 "Default: 'reflect', available: {'reflect', 'constant', 'nearest', 'mirror', 'wrap'}.")

        grp_post.add_argument(
            "--out-gaussian-filter-size",
            required=False,
            type=float,
            default=self.median_filter_size,
            help="Apply gaussian filter (2d) on reconstructed volume with the given window size."
        )

        grp_post.add_argument(
            "--out-gaussian-filter-mode",
            type=str,
            required=False,
            default=self.median_filter_mode,
            help="Type of gaussian filter. Default: 'reflect', available: "
                 "{'reflect', 'constant', 'nearest', 'mirror', 'wrap'}"
        )

        return parser

    def update(self, args):
        self.circular_mask = args.circular_mask
        self.cut_off_level_post = args.cut_off_post

        self.gaussian_filter_size = args.out_gaussian_filter_size
        self.gaussian_filter_mode = args.out_gaussian_filter_mode

        self.median_filter_size = args.median_filter_size
        self.median_filter_mode = args.median_filter_mode
        # self.median_filter3d_size = args.median_filter3d_size
