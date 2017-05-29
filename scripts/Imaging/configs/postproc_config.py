class PostProcConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction
    """

    def __init__(self):
        """
        Builds a default post-processing configuration with a sensible choice of parameters
        """
        self.circular_mask = None
        self.circular_mask_val = None
        self.outliers_threshold = None
        self.outliers_mode = None
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

    def __str__(self):
        return "Circular mask: {0}\n".format(self.circular_mask) \
            + "Circular mask value: {0}\n".format(self.circular_mask_val) \
            + "Outliers threshold on reconstructed volume: {0}\n".format(self.outliers_threshold) \
            + "Outliers mode on reconstructed volume: {0}\n".format(self.outliers_mode) \
            + "Gaussian filter size: {0}\n".format(self.gaussian_size) \
            + "Gaussian filter mode: {0}\n".format(self.gaussian_mode) \
            + "Gaussian filter order: {0}\n".format(self.gaussian_order) \
            + "Median filter size:: {0}\n".format(self.median_size) \
            + "Median filter mode: {0}".format(self.median_mode)

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
            "--circular-mask-val",
            required=False,
            type=float,
            default=self.circular_mask_val,
            help="The value that the pixels in the mask will be set to.")

        grp_post.add_argument(
            "--post-outliers",
            required=False,
            type=float,
            help="Outliers threshold for reconstructed volume.\n"
            "Pixels below and/or above (depending on mode) this threshold will be clipped."
        )

        from filters.outliers import modes as outliers_modes
        grp_post.add_argument(
            "--post-outliers-mode",
            required=False,
            type=str,
            choices=outliers_modes(),
            help="Which pixels to clip, only dark ones, bright ones or both.")

        grp_post.add_argument(
            "--post-median-size",
            required=False,
            type=float,
            default=self.median_size,
            help="Apply median filter (2d) on reconstructed volume with the given window size.(post processing)"
        )

        from filters.median_filter import modes as median_modes
        grp_post.add_argument(
            "--post-median-mode",
            type=str,
            required=False,
            default=self.median_mode,
            choices=median_modes(),
            help="Default: %(default)s\n"
            "Mode of median filter which determines how the array borders are handled.(post processing)"
        )

        grp_post.add_argument(
            "--post-gaussian-size",
            required=False,
            type=float,
            default=self.gaussian_size,
            help="Apply gaussian filter (2d) on reconstructed volume with the given window size."
        )

        from filters.gaussian import modes as gaussian_modes
        grp_post.add_argument(
            "--post-gaussian-mode",
            type=str,
            required=False,
            default=self.gaussian_mode,
            choices=gaussian_modes(),
            help="Default: %(default)s\nMode of gaussian filter which determines how the array borders are handled.(post processing)."
        )

        grp_post.add_argument(
            "--post-gaussian-order",
            required=False,
            type=int,
            default=self.gaussian_order,
            help="Default: %(default)d\nThe order of the filter along each axis is given as a sequence of integers, \n"
            "or as a single number. An order of 0 corresponds to convolution with a Gaussian kernel.\n"
            "An order of 1, 2, or 3 corresponds to convolution with the first, second or third derivatives of a Gaussian.\n"
            "Higher order derivatives are not implemented.")

        return parser

    def update(self, args):
        self.circular_mask = args.circular_mask
        self.circular_mask_val = args.circular_mask_val

        self.outliers_threshold = args.post_outliers
        self.outliers_mode = args.post_outliers_mode

        self.gaussian_size = args.post_gaussian_size
        self.gaussian_mode = args.post_gaussian_mode
        self.gaussian_order = args.post_gaussian_order

        self.median_size = args.post_median_size
        self.median_mode = args.post_median_mode
