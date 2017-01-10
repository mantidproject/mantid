#pylint: disable=too-many-instance-attributes
class PreProcConfig(object):
    """
    All pre-processing options required to run a tomographic reconstruction.

    Options like the stripe removal, MCP correction, or even the median filter would
    better be handled as plugins. For the time being we just have a panel with a fixed
    set of options to enable/disable/configure
    """

    DEF_NUM_ITER = 5

    def __init__(self):
        # defaults that look sensible for the MCP detector:
        # median_filter=3, rotate=-1, crop=[0,  252, 0, 512], MCP correction: on
        self.input_dir = None
        self.input_dir_flat = None
        self.input_dir_dark = None
        self.in_img_format = 'fits'
        self.out_img_format = 'fits'
        self.max_angle = 360

        # Rotation 90 degrees clockwise (positive) or counterclockwise (negative)
        # Example: -1 => (-90 degrees == 90 degrees counterclockwise)
        self.rotation = -1
        self.normalize_flat_dark = True
        # list with coordinates of the region for normalization / "air" / not blocked by any object
        self.normalize_air_region = None
        self.normalize_proton_charge = False

        # all coords outside the ROI will be cropped
        self.region_of_interest = None
        self.cut_off_level = 0
        self.mcp_corrections = True
        self.scale_down = 0
        self.median_filter_size = 3
        self.line_projection = True
        self.stripe_removal_method = 'wavelet-fourier'

        # center of rotation
        self.cor = None
        self.save_preproc_imgs = True

    def __str__(self):
        import os
        mystr = "Input path (relative): {0}\n".format(self.input_dir)
        if self.input_dir:
            mystr += "Input path (absolute): {0}\n".format(
                os.path.abspath(self.input_dir))
        else:
            mystr += "Input path (absolute): {0}\n".format(
                'cannot find because the input '
                'path has not been set')
        mystr += "Input path for flat (open beam) images (relative): {0}\n".format(
            self.input_dir_flat)
        mystr += "Input path for dark images (relative): {0}\n".format(
            self.input_dir_dark)
        mystr += "Input image format: {0}\n".format(self.in_img_format)
        mystr += "Output image format: {0}\n".format(self.out_img_format)
        mystr += "Maximum angle:: {0}\n".format(self.max_angle)
        mystr += "Center of rotation: {0}\n".format(self.cor)
        mystr += "Region of interest (crop coordinates): {0}\n".format(
            self.region_of_interest)
        mystr += "Normalize by flat/dark images: {0}\n".format(
            self.normalize_flat_dark)
        mystr += "Normalize by air region: {0}\n".format(
            self.normalize_air_region)
        mystr += "Normalize by proton charge: {0}\n".format(
            self.normalize_proton_charge)
        mystr += "Cut-off on normalized images: {0}\n".format(
            self.cut_off_level)
        mystr += "Corrections for MCP detector: {0}\n".format(
            self.mcp_corrections)
        mystr += "Scale down factor for images: {0}\n".format(self.scale_down)
        mystr += "Median filter width: {0}\n".format(self.median_filter_size)
        mystr += "Rotation: {0}\n".format(self.rotation)
        mystr += "Line projection (line integral/log re-scale): {0}\n".format(
            1)
        mystr += "Sinogram stripes removal: {0}".format(
            self.stripe_removal_method)

        return mystr
