# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import namedtuple
from collections.abc import Sequence

from enum import Enum
from sans_core.common.enums import DetectorType, SANSInstrument
from sans_core.common.xml_parsing import get_named_elements_from_ipf_file


detector_shape_bundle = namedtuple("detector_shape_bundle", "rectangular_shape, width, height, " "number_of_pixels_override")
geometry_bundle = namedtuple("geometry_bundle", "shape, first_low_angle_spec_number")


class DetectorOrientation(Enum):
    """
    Defines the detector orientation.
    """

    HORIZONTAL = "Horizontal"
    ROTATED = "Rotated"
    VERTICAL = "Vertical"


def get_geometry_information(ipf_path, detector_type):
    """
    This function extracts geometry information for the detector benches.

    The information requested is:
    1. Are we dealing with a rectangular detector
    2. What is the width
    3. What is the height
    4. If the detector has a hole in somewhere then the total number of pixels cannot be solely determined by height
       and width. This provides an override for the number of pixels
    :param ipf_path: The path to the Instrument Parameter File.
    :param detector_type: The type of detector for which we should get the information.
    :return: Geometry information for LAB and HAB as well as the firs
    """

    def create_detector_shape_bundle(num_columns, non_rectangle_width, num_rows, non_rectangle_height, num_pixels):
        cols_data = num_columns
        if len(cols_data) > 0:
            rectangular_shape = True
            width = int(cols_data[0])
        else:
            rectangular_shape = False
            width = int(non_rectangle_width[0])

        rows_data = num_rows
        if len(rows_data) > 0:
            height = int(rows_data[0])
        else:
            rectangular_shape = False
            height = int(non_rectangle_height[0])

        number_of_pixels = num_pixels
        if len(number_of_pixels) > 0:
            number_of_pixels_override = int(number_of_pixels[0])
        else:
            number_of_pixels_override = None

        return detector_shape_bundle(
            rectangular_shape=rectangular_shape, width=width, height=height, number_of_pixels_override=number_of_pixels_override
        )

    # Determine the prefix for the detector
    if detector_type is DetectorType.LAB:
        prefix = "low-angle-detector-"
    elif detector_type is DetectorType.HAB:
        prefix = "high-angle-detector-"
    else:
        raise RuntimeError("MaskingParser: Tyring to get information for unknown " "detector {0}".format(str(detector_type)))

    search_items_for_detectors = ["num-columns", "non-rectangle-width", "num-rows", "non-rectangle-height", "num-pixels"]
    search_items = [prefix + item for item in search_items_for_detectors]
    search_items.append("first-low-angle-spec-number")

    found_items = get_named_elements_from_ipf_file(ipf_path, search_items, int)
    # Some items might not have been found, it they are not in the dictionary, then add an empty list
    not_found_items = [item for item in search_items if item not in list(found_items.keys())]
    for not_found_item in not_found_items:
        found_items.update({not_found_item: []})

    # The old code assumed that we have an iterable value, which is not normally true, hence make it iterable
    for key, value in list(found_items.items()):
        if not isinstance(value, Sequence):
            found_items[key] = [value]

    shape = create_detector_shape_bundle(
        num_columns=found_items[prefix + "num-columns"],
        non_rectangle_width=found_items[prefix + "non-rectangle-width"],
        num_rows=found_items[prefix + "num-rows"],
        non_rectangle_height=found_items[prefix + "non-rectangle-height"],
        num_pixels=found_items[prefix + "num-pixels"],
    )

    return geometry_bundle(shape=shape, first_low_angle_spec_number=found_items["first-low-angle-spec-number"])


class SpectraBlock(object):
    """
    The SpectraBlock class maps a SANS-particular detector selection syntax to a detector selection on the actual
    instrument.
    """

    def __init__(self, ipf_path, run_number, instrument, detector_type):
        super(SpectraBlock, self).__init__()
        self._ipf_path = ipf_path
        self._run_number = run_number
        self._instrument = instrument
        self._detector_shape = None
        self._first_spectrum_number = None
        self._detector_orientation = None

        # Set the attributes above
        self._set_attributes(detector_type)

    def _set_attributes(self, detector_type):
        # Set attributes from IPF
        geometry = get_geometry_information(self._ipf_path, detector_type)
        self._detector_shape = geometry.shape
        self._first_spectrum_number = geometry.first_low_angle_spec_number

        # Extract information about the concrete instrument
        # We now have to perform instrument- and run- specific adjustments.
        if self._instrument is SANSInstrument.SANS2D:
            if self._run_number < 568:
                self._first_spectrum_number = 1
                self._detector_orientation = DetectorOrientation.VERTICAL
            elif 568 <= self._run_number < 684:
                self._first_spectrum_number = 9
                self._detector_orientation = DetectorOrientation.ROTATED
            else:
                self._first_spectrum_number = 9
                self._detector_orientation = DetectorOrientation.HORIZONTAL
        elif self._instrument is SANSInstrument.LARMOR:
            self._first_spectrum_number = 10
            self._detector_orientation = DetectorOrientation.HORIZONTAL
        elif self._instrument is SANSInstrument.LOQ:
            self._first_spectrum_number = 3
            self._detector_orientation = DetectorOrientation.HORIZONTAL
        elif self._instrument is SANSInstrument.ZOOM:
            self._first_spectrum_number = 9
            self._detector_orientation = DetectorOrientation.HORIZONTAL
        else:
            raise RuntimeError("MaskParser: Cannot handle masking request for " "instrument {0}".format(str(self._instrument)))

    def get_block(self, y_lower, x_lower, y_dim, x_dim):
        """
        Create a list of spectra for a rectangular block of size x_dim by y_dim

        :param y_lower: the x coordinate of the starting point of the lower left corner
        :param x_lower: the y coordinate of the starting point of the lower left corner
        :param y_dim: the y dimension
        :param x_dim: the x dimension
        :return: a string which contains the rectangular region of spectra
        """
        if y_dim is None:
            y_dim = self._detector_shape.height
        if x_dim is None:
            x_dim = self._detector_shape.width

        # Warn the user if there is an non-rectangular shape involved.

        # Get the spectrum list depending on the orientation of the detector
        detector_dimension = self._detector_shape.width
        base_spectrum_number = self._first_spectrum_number

        output = []
        if self._detector_orientation == DetectorOrientation.HORIZONTAL:
            start_spectrum = base_spectrum_number + y_lower * detector_dimension + x_lower
            for y in range(0, y_dim):
                output.extend((start_spectrum + (y * detector_dimension) + x for x in range(0, x_dim)))
        elif self._detector_orientation == DetectorOrientation.VERTICAL:
            start_spectrum = base_spectrum_number + x_lower * detector_dimension + y_lower
            for x in range(detector_dimension - 1, detector_dimension - x_dim - 1, -1):
                output.extend((start_spectrum + ((detector_dimension - x - 1) * detector_dimension) + y for y in range(0, y_dim)))
        elif self._detector_orientation == DetectorOrientation.ROTATED:
            # This is the horizontal one rotated so need to map the x_low and y_low to their rotated versions
            start_spectrum = base_spectrum_number + y_lower * detector_dimension + x_lower
            max_spectrum = detector_dimension * detector_dimension + base_spectrum_number - 1
            for y in range(0, y_dim):
                output.extend(
                    (max_spectrum - (start_spectrum + x + (y * detector_dimension) - base_spectrum_number) for x in range(0, x_dim))
                )
        return output
