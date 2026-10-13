# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-arguments,too-many-branches
import sys
from mantid.geometry import ComponentType
from mantid.kernel import Logger
from plugins.algorithms.component_info_utils import resolve_component_index


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
    nx_pixels, ny_pixels, pixel_size_x, pixel_size_y = _get_pixel_info(workspace)

    return [x / pixel_size_x * 1000.0 + nx_pixels / 2.0 - 0.5, y / pixel_size_y * 1000.0 + ny_pixels / 2.0 - 0.5]


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

    return [(x - nx_pixels / 2.0 + 0.5) * pixel_size_x / 1000.0, (y - ny_pixels / 2.0 + 0.5) * pixel_size_y / 1000.0]


def get_masked_ids(nx_low, nx_high, ny_low, ny_high, workspace, component_name=None):
    """
    Generate a list of masked IDs.
    @param nx_low: number of pixels to mask on the lower-x side of the detector
    @param nx_high: number of pixels to mask on the higher-x side of the detector
    @param ny_low: number of pixels to mask on the lower-y side of the detector
    @param ny_high: number of pixels to mask on the higher-y side of the detector
    @param workspace: the pixel number and size info will be taken from the workspace
    """

    component_info = workspace.componentInfo()
    if component_name is None or component_name == "":
        component_name = component_info.getStringParameter("detector-name")[0]

    component_index = resolve_component_index(component_name, component_info)
    component_type = component_info.componentType(component_index)

    Logger("hfir_instrument").debug(
        "Masking pixels: nx_low=%s, nx_high=%s, ny_low=%s, ny_high=%s for component %s of type=%s."
        % (nx_low, nx_high, ny_low, ny_high, component_name, component_type)
    )

    IDs = []
    if component_type == ComponentType.Rectangular:
        bank_index = component_index
        id_start = component_info.pixelGridIdStart(bank_index)
        id_step = component_info.pixelGridIdStep(bank_index)
        max_detector_id = component_info.pixelGridMaxDetectorID(bank_index)
        npixels_x = component_info.pixelGridNX(bank_index)
        # left
        i = 0
        while i < nx_low * id_step:
            IDs.append(id_start + i)
            i += 1
        # right
        i = max_detector_id - nx_high * id_step
        while i < max_detector_id:
            IDs.append(i)
            i += 1
        # low: 0,256,512,768,..,1,257,513
        for row in range(ny_low):
            i = row + id_start
            while i < npixels_x * id_step - id_step + ny_low + id_start:
                IDs.append(i)
                i += id_step
        # high # 255, 511, 767..
        for row in range(ny_high):
            i = id_step + id_start - row - 1
            while i < npixels_x * id_step + id_start:
                IDs.append(i)
                i += id_step
    elif _is_assembly_or_detector(component_info, component_index):
        # Wing detector
        # x
        detector_info = workspace.detectorInfo()
        total_n_tubes = len(component_info.children(component_index))
        for tube in range(nx_low):
            IDs.extend(list(_get_ids_for_assembly(component_info, detector_info, _child_index(component_info, component_index, tube))))
        for tube in range(total_n_tubes - nx_high, total_n_tubes):
            IDs.extend(list(_get_ids_for_assembly(component_info, detector_info, _child_index(component_info, component_index, tube))))
        # y
        for tube in range(total_n_tubes):
            tube_index = _child_index(component_info, component_index, tube)
            n_pixels = len(component_info.children(tube_index))
            for pixel in range(n_pixels):
                if pixel in range(ny_low):
                    IDs.append(_detector_id(component_info, detector_info, _child_index(component_info, tube_index, pixel)))
                if pixel in range(n_pixels - ny_high, n_pixels):
                    IDs.append(_detector_id(component_info, detector_info, _child_index(component_info, tube_index, pixel)))
    else:
        Logger("hfir_instrument").error(
            "get_masked_pixels not applied. Component not valid: %s of type %s." % (component_info.name(component_index), component_type)
        )
    return IDs


def get_masked_pixels(nx_low, nx_high, ny_low, ny_high, workspace, component_name=None):
    """
    Generate a list of masked pixels.
    @param nx_low: number of pixels to mask on the lower-x side of the detector
    @param nx_high: number of pixels to mask on the higher-x side of the detector
    @param ny_low: number of pixels to mask on the lower-y side of the detector
    @param ny_high: number of pixels to mask on the higher-y side of the detector
    @param workspace: the pixel number and size info will be taken from the workspace
    """
    id_list = get_masked_ids(nx_low, nx_high, ny_low, ny_high, workspace, component_name)

    component_info = workspace.componentInfo()
    nx_pixels = int(component_info.getNumberParameter("number-of-x-pixels")[0])
    ny_pixels = int(component_info.getNumberParameter("number-of-y-pixels")[0])

    pixel_list = []
    current_det_id = 3  # First ID (Need to get this from somewhere!!)
    for i in range(ny_pixels):
        for j in range(nx_pixels):
            if current_det_id in id_list:
                pixel_list.append([i, j])
    return pixel_list


def _is_assembly_or_detector(component_info, component_index):
    """
    True if the component is a generic (unstructured) assembly or a detector, i.e.
    what the legacy Instrument API reported as one of these types:
    'CompAssembly'
    'ObjCompAssembly'
    'DetectorComponent'
    The instrument root is itself an unstructured assembly, but it was never one of these types.
    """
    assembly_types = (ComponentType.Unstructured, ComponentType.OutlineComposite, ComponentType.Detector)
    return component_index != component_info.root() and component_info.componentType(component_index) in assembly_types


def _child_index(component_info, parent_index, position):
    """
    Component index of the child at the given position within its parent.
    Raises RuntimeError for a position out of range, including negative positions,
    as the legacy ICompAssembly item access did.
    """
    children = component_info.children(parent_index)
    if position < 0 or position >= len(children):
        raise RuntimeError(f"Child {position} of component {component_info.name(parent_index)} is out of range")
    return int(children[position])


def _detector_id(component_info, detector_info, component_index):
    """
    Detector ID of a component that must be a detector.
    Raises AttributeError otherwise, as the legacy call of getID() on a non-detector component did.
    """
    if not component_info.isDetector(component_index):
        raise AttributeError(f"Component {component_info.name(component_index)} is not a detector and has no detector ID")
    return detector_info.detid(component_index)


def _get_ids_for_assembly(component_info, detector_info, component_index):
    """
    Recursive function that get a generator for all IDs for a component.
    Component must be one of these:
    'CompAssembly'
    'ObjCompAssembly'
    'DetectorComponent'
    """
    if component_info.isDetector(component_index):
        yield detector_info.detid(component_index)
    elif component_info.componentType(component_index) in (ComponentType.Generic, ComponentType.Infinite):
        # A leaf that is not a detector, which the legacy recursion could not descend into
        raise AttributeError(f"Component {component_info.name(component_index)} is neither a detector nor an assembly")
    else:
        for child_index in component_info.children(component_index):
            for j in _get_ids_for_assembly(component_info, detector_info, int(child_index)):
                yield j


def _get_pixel_info(workspace):
    """
    Get the pixel size and number of pixels from the workspace
    @param workspace: workspace to extract the pixel information from
    """
    component_info = workspace.componentInfo()
    # # Number of detector pixels in X
    nx_pixels = int(component_info.getNumberParameter("number-of-x-pixels")[0])
    # # Number of detector pixels in Y
    ny_pixels = int(component_info.getNumberParameter("number-of-y-pixels")[0])
    # # Pixel size in mm
    pixel_size_x = component_info.getNumberParameter("x-pixel-size")[0]
    pixel_size_y = component_info.getNumberParameter("y-pixel-size")[0]

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
        component_info = workspace.componentInfo()
        apertures_lst = component_info.getStringParameter("aperture-distances")[0]
        apertures = apertures_lst.split(",")
        # Note that they are in reverse order, the first item is for 8 guides
        # and the last item is for 0 guide.
        index = 8 - nguides
        return float(apertures[index])
    except:
        raise RuntimeError("Could not find the for %s\n  %s" % (workspace, sys.exc_info()[1]))
