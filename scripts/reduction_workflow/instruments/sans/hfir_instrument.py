# pylint: disable=invalid-name,too-many-arguments,too-many-branches
import sys
from mantid.kernel import Logger


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
        @param workspace: the pixel number and size info will be taken from
        the workspace
    """
    nx_pixels, ny_pixels, pixel_size_x, pixel_size_y = _get_pixel_info(
        workspace)

    return [x / pixel_size_x * 1000.0 + nx_pixels / 2.0 - 0.5,
            y / pixel_size_y * 1000.0 + ny_pixels / 2.0 - 0.5]


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
    nx_pixels, ny_pixels, pixel_size_x, pixel_size_y = _get_pixel_info(
        workspace)

    return [(x - nx_pixels / 2.0 + 0.5) * pixel_size_x / 1000.0,
            (y - ny_pixels / 2.0 + 0.5) * pixel_size_y / 1000.0]


def get_masked_ids(
        nx_low,
        nx_high,
        ny_low,
        ny_high,
        workspace,
        component_name=None):
    """
        Generate a list of masked IDs.
        @param nx_low: number of pixels to mask on the lower-x side of the detector
        @param nx_high: number of pixels to mask on the higher-x side of the detector
        @param ny_low: number of pixels to mask on the lower-y side of the detector
        @param ny_high: number of pixels to mask on the higher-y side of the detector
        @param workspace: the pixel number and size info will be taken from the workspace
    """

    instrument = workspace.getInstrument()
    if component_name is None or component_name == "":
        component_name = instrument.getStringParameter('detector-name')[0]

    component = instrument.getComponentByName(component_name)

    Logger("hfir_instrument").debug(
        "Masking pixels: nx_low=%s, nx_high=%s, ny_low=%s, ny_high=%s for component %s of type=%s." %
        (nx_low, nx_high, ny_low, ny_high, component_name, component.type()))

    IDs = []
    if component.type() == 'RectangularDetector':
        # left
        i = 0
        while i < nx_low * component.idstep():
            IDs.append(component.idstart() + i)
            i += 1
        # right
        i = component.maxDetectorID() - nx_high * component.idstep()
        while i < component.maxDetectorID():
            IDs.append(i)
            i += 1
        # low: 0,256,512,768,..,1,257,513
        for row in range(ny_low):
            i = row + component.idstart()
            while i < component.nelements() * component.idstep() - component.idstep() + \
                    ny_low + component.idstart():
                IDs.append(i)
                i += component.idstep()
        # high # 255, 511, 767..
        for row in range(ny_high):
            i = component.idstep() + component.idstart() - row - 1
            while i < component.nelements() * component.idstep() + component.idstart():
                IDs.append(i)
                i += component.idstep()
    elif component.type() == 'CompAssembly' or component.type() == 'ObjCompAssembly' or component.type() == 'DetectorComponent':
        # Wing detector
        # x
        total_n_tubes = component.nelements()
        for tube in range(nx_low):
            IDs.extend(list(_get_ids_for_assembly(component[tube])))
        for tube in range(total_n_tubes - nx_high, total_n_tubes):
            IDs.extend(list(_get_ids_for_assembly(component[tube])))
        # y
        for tube in range(total_n_tubes):
            for pixel in range(component[tube].nelements()):
                if pixel in range(ny_low):
                    IDs.append(component[tube][pixel].getID())
                if pixel in range(
                        component[tube].nelements() - ny_high,
                        component[tube].nelements()):
                    IDs.append(component[tube][pixel].getID())
    else:
        Logger("hfir_instrument").error(
            "get_masked_pixels not applied. Component not valid: %s of type %s." %
            (component.getName(), component.type()))
    return IDs


def get_masked_pixels(
        nx_low,
        nx_high,
        ny_low,
        ny_high,
        workspace,
        component_name=None):
    """
        Generate a list of masked pixels.
        @param nx_low: number of pixels to mask on the lower-x side of the detector
        @param nx_high: number of pixels to mask on the higher-x side of the detector
        @param ny_low: number of pixels to mask on the lower-y side of the detector
        @param ny_high: number of pixels to mask on the higher-y side of the detector
        @param workspace: the pixel number and size info will be taken from the workspace
    """
    id_list = get_masked_ids(
        nx_low,
        nx_high,
        ny_low,
        ny_high,
        workspace,
        component_name)

    nx_pixels = int(workspace.getInstrument(
    ).getNumberParameter("number-of-x-pixels")[0])
    ny_pixels = int(workspace.getInstrument(
    ).getNumberParameter("number-of-y-pixels")[0])

    pixel_list = []
    current_det_id = 3  # First ID (Need to get this from somewhere!!)
    for i in range(ny_pixels):
        for j in range(nx_pixels):
            if current_det_id in id_list:
                pixel_list.append([i, j])
    return pixel_list


def _get_ids_for_assembly(component):
    '''
    Recursive function that get a generator for all IDs for a component.
    Component must be one of these:
    'CompAssembly'
    'ObjCompAssembly'
    'DetectorComponent'
    '''
    if component.type() == 'DetectorComponent':
        yield component.getID()
    else:
        for i in range(component.nelements()):
            for j in _get_ids_for_assembly(component[i]):
                yield j


def _get_pixel_info(workspace):
    """
        Get the pixel size and number of pixels from the workspace
        @param workspace: workspace to extract the pixel information from
    """
    # # Number of detector pixels in X
    nx_pixels = int(workspace.getInstrument(
    ).getNumberParameter("number-of-x-pixels")[0])
    # # Number of detector pixels in Y
    ny_pixels = int(workspace.getInstrument(
    ).getNumberParameter("number-of-y-pixels")[0])
    # # Pixel size in mm
    pixel_size_x = workspace.getInstrument(
    ).getNumberParameter("x-pixel-size")[0]
    pixel_size_y = workspace.getInstrument(
    ).getNumberParameter("y-pixel-size")[0]

    return nx_pixels, ny_pixels, pixel_size_x, pixel_size_y


def get_detector_from_pixel(pixel_list):
    """
        Returns a list of detector IDs from a list of [x,y] pixels,
        where the pixel coordinates are in pixel units.
    """
    return [3 + p[0] + p[1] * 256 for p in pixel_list]


def get_aperture_distance(workspace):
    """
        Return the aperture distance
        @param workspace: workspace to get the aperture distance from
    """
    try:
        nguides = workspace.getRun().getProperty("number-of-guides").value
        apertures_lst = workspace.getInstrument(
        ).getStringParameter("aperture-distances")[0]
        apertures = apertures_lst.split(',')
        # Note that they are in reverse order, the first item is for 8 guides
        # and the last item is for 0 guide.
        index = 8 - nguides
        return float(apertures[index])
    except:
        raise RuntimeError(
            "Could not find the for %s\n  %s" %
            (workspace, sys.exc_info()[1]))
