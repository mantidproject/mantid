class PostProcConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction
    """

    def __init__(self):
        """
        Builds a default post-processing configuration with a sensible choice of parameters
        """
        self.output_dir = None
        self.circular_mask = 0.94
        self.cut_off_level = 0
        self.gaussian_filter_par = 0
        self.median_filter_size = 0
        self.median_filter3d_size = 0

    def __str__(self):
        import os
        mystr = "Output path (relative): {0}\n".format(self.output_dir)
        if self.output_dir:
            mystr += "Output path (absolute): {0}\n".format(
                os.path.abspath(self.output_dir))
        else:
            mystr += "Output path (absolute): {0}\n".format(
                'cannot find because the input '
                'path has not been set')
        mystr += "Circular mask: {0}\n".format(self.circular_mask)
        mystr += "Cut-off on reconstructed volume: {0}\n".format(
            self.cut_off_level)
        mystr += "Gaussian filter: {0}\n".format(self.gaussian_filter_par)
        mystr += "Median filter size:: {0}\n".format(self.median_filter_size)
        mystr += "Median filter (3d) size:: {0}\n".format(
            self.median_filter3d_size)

        return mystr
