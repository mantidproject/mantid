class PostProcConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction
    """

    def __init__(self):
        """
        Builds a default post-processing configuration with a sensible choice of parameters
        """
        self.circular_mask = None
        self.cut_off_level_post = 0
        self.gaussian_size = None
        self.gaussian_mode = 'reflect'
        self.gaussian_order = 0

        self.median_size = None
        """
        :param median_mode: Default: 'reflect', {'reflect', 'constant', 'nearest', 'mirror', 'wrap'}, optional
            The mode parameter determines how the array borders are handled, where cval is the value when
            mode is equal to 'constant'.
        """
        self.median_mode = 'reflect'
        self.median_filter3d_size = 0  # TODO unused

    def __str__(self):
        return "Circular mask: {0}\n".format(self.circular_mask) \
            + "Cut-off on reconstructed volume: {0}\n".format(self.cut_off_level_post) \
            + "Gaussian filter size: {0}\n".format(self.gaussian_size) \
            + "Gaussian filter mode: {0}\n".format(self.gaussian_mode) \
            + "Median filter size:: {0}\n".format(self.median_size) \
            + "Median filter mode: {0}\n".format(self.median_mode)

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
            "--out-median-size",
            required=False,
            type=float,
            default=self.median_size,
            help="Apply median filter (2d) on reconstructed volume with the given window size.(post processing)"
        )

        median_modes = ['reflect', 'constant', 'nearest', 'mirror', 'wrap']
        grp_post.add_argument(
            "--out-median-mode",
            type=str,
            required=False,
            default=self.median_mode,
            choices=median_modes,
            help="Default: %(default)s\n"
                 "Mode of median filter which determines how the array borders are handled.(post processing)"
        )

        grp_post.add_argument(
            "--out-gaussian-size",
            required=False,
            type=float,
            default=self.gaussian_size,
            help="Apply gaussian filter (2d) on reconstructed volume with the given window size."
        )

        grp_post.add_argument(
            "--out-gaussian-mode",
            type=str,
            required=False,
            default=self.gaussian_mode,
            choices=median_modes,
            help="Default: %(default)s\nMode of gaussian filter which determines how the array borders are handled.(post processing).")

        grp_post.add_argument(
            "--out-gaussian-order",
            required=False,
            type=int,
            default=self.gaussian_order,
            help="Default: %(default)d\nThe order of the filter along each axis is given as a sequence of integers, \n"
            "or as a single number. An order of 0 corresponds to convolution with a Gaussian kernel.\n"
            "An order of 1, 2, or 3 corresponds to convolution with the first, second or third derivatives of a Gaussian.\n"
            "Higher order derivatives are not implemented."
        )

        return parser

    def update(self, args):
        self.circular_mask = args.circular_mask
        self.cut_off_level_post = args.cut_off_post

        self.gaussian_size = args.out_gaussian_size
        self.gaussian_mode = args.out_gaussian_mode
        self.gaussian_order = args.out_gaussian_order

        self.median_size = args.out_median_size
        self.median_mode = args.out_median_mode
        # self.median_filter3d_size = args.median_filter3d_size
