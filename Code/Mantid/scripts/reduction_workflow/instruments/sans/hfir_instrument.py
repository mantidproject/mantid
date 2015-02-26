#pylint: disable=invalid-name
import sys

def get_default_beam_center(workspace=None):
    """
        Returns the default beam center position, or the pixel location
        of real-space coordinates (0,0).
    """
    # If a workspace is not provided, we'll figure out the position later
    if workspace is None:
        return [None, None]
    return get_pixel_from_coordinate(0, 0, workspace)

def get_pixel_from_coordinate(x, y, workspace):
    """
        Returns the pixel coordinates corresponding to the
        given real-space position.

        This assumes that the center of the detector is aligned
        with the beam. An additional offset may need to be applied

        @param x: real-space x coordinate [m]
        @param y: real-space y coordinate [m]
        @param workspace: the pixel number and size info will be taken from the workspace
    """
    nx_pixels, ny_pixels, pixel_size_x, pixel_size_y = _get_pixel_info(workspace)

    return [x/pixel_size_x*1000.0 + nx_pixels/2.0-0.5,
            y/pixel_size_y*1000.0 + ny_pixels/2.0-0.5]

def get_coordinate_from_pixel(x, y, workspace):
    """
        Returns the real-space coordinates corresponding to the
        given pixel coordinates [m].

        This assumes that the center of the detector is aligned
        with the beam. An additional offset may need to be applied

        @param x: pixel x coordinate
        @param y: pixel y coordinate
        @param workspace: the pixel number and size info will be taken from the workspace
    """
    nx_pixels, ny_pixels, pixel_size_x, pixel_size_y = _get_pixel_info(workspace)

    return [(x-nx_pixels/2.0+0.5) * pixel_size_x/1000.0,
            (y-ny_pixels/2.0+0.5) * pixel_size_y/1000.0]

def get_masked_pixels(nx_low, nx_high, ny_low, ny_high, workspace):
    """
        Generate a list of masked pixels.
        @param nx_low: number of pixels to mask on the lower-x side of the detector
        @param nx_high: number of pixels to mask on the higher-x side of the detector
        @param ny_low: number of pixels to mask on the lower-y side of the detector
        @param ny_high: number of pixels to mask on the higher-y side of the detector
        @param workspace: the pixel number and size info will be taken from the workspace
    """
    nx_pixels, ny_pixels, dummy_pixel_size_x, dummy_pixel_size_y = _get_pixel_info(workspace)
    if nx_low<0 or nx_high<0 or ny_low<0 or ny_high<0:
        raise RuntimeError, "Pixel edges should be greater than zero"

    masked_x = range(0, nx_low)
    masked_x.extend(range(nx_pixels-nx_high, nx_pixels))

    masked_y = range(0, ny_low)
    masked_y.extend(range(ny_pixels-ny_high, ny_pixels))

    masked_pts = []
    for y in masked_y:
        masked_pts.extend([ [y,x] for x in range(nx_pixels) ])
    for x in masked_x:
        masked_pts.extend([ [y,x] for y in range(ny_low, ny_pixels-ny_high) ])

    return masked_pts

def _get_pixel_info(workspace):
    """
        Get the pixel size and number of pixels from the workspace
        @param workspace: workspace to extract the pixel information from
    """
    ## Number of detector pixels in X
    nx_pixels = int(workspace.getInstrument().getNumberParameter("number-of-x-pixels")[0])
    ## Number of detector pixels in Y
    ny_pixels = int(workspace.getInstrument().getNumberParameter("number-of-y-pixels")[0])
    ## Pixel size in mm
    pixel_size_x = workspace.getInstrument().getNumberParameter("x-pixel-size")[0]
    pixel_size_y = workspace.getInstrument().getNumberParameter("y-pixel-size")[0]

    return nx_pixels, ny_pixels, pixel_size_x, pixel_size_y

def get_detector_from_pixel(pixel_list):
    """
        Returns a list of detector IDs from a list of [x,y] pixels,
        where the pixel coordinates are in pixel units.
    """
    return [ 1000000 + p[0]*1000 + p[1] for p in pixel_list ]

def get_aperture_distance(workspace):
    """
        Return the aperture distance
        @param workspace: workspace to get the aperture distance from
    """
    try:
        nguides = workspace.getRun().getProperty("number-of-guides").value
        apertures_lst = workspace.getInstrument().getStringParameter("aperture-distances")[0]
        apertures = apertures_lst.split(',')
        # Note that they are in reverse order, the first item is for 8 guides
        # and the last item is for 0 guide.
        index = 8-nguides
        return float(apertures[index])
    except:
        raise RuntimeError, "Could not find the for %s\n  %s" % (workspace, sys.exc_value)
