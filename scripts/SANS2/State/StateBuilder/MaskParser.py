from collections import namedtuple, Sequence
from mantid.kernel import logger
from SANS2.Common.XMLParsing import get_named_elements_from_ipf_file
from SANS2.Common.SANSFileInformation import (get_instrument_paths_for_sans_file, SANSFileInformationFactory)
from SANS2.Common.SANSEnumerations import (SANSInstrument, DetectorOrientation, DetectorType)


detector_shape_bundle = namedtuple("detector_shape_bundle", 'rectangular_shape, width, height, '
                                                            'number_of_pixels_override')
geometry_bundle = namedtuple("geometry_bundle", 'shape, first_low_angle_spec_number')


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

        return detector_shape_bundle(rectangular_shape=rectangular_shape, width=width, height=height,
                                     number_of_pixels_override=number_of_pixels_override)

    # Determine the prefix for the detector
    if detector_type is DetectorType.Hab:
        prefix = "low-angle-detector-"
    elif detector_type is DetectorType.Lab:
        prefix = "high-angle-detector-"
    else:
        raise RuntimeError("MaskingParser: Tyring to get information for unknown "
                           "detector {0}".format(str(detector_type)))

    search_items_for_detectors = ["num-columns",
                                  "non-rectangle-width", "num-rows",
                                  "non-rectangle-height", "num-pixels"]
    search_items = [prefix + item for item in search_items_for_detectors]
    search_items.append("first-low-angle-spec-number")

    found_items = get_named_elements_from_ipf_file(ipf_path, search_items, int)
    # Some items might not have been found, it they are not in the dictionary, then add an empty list
    not_found_items = [item for item in search_items if item not in found_items.keys()]
    for not_found_item in not_found_items:
        found_items.update({not_found_item: []})

    # The old code assumed that we have an iterable value, which is not normally true, hence make it iterable
    for key, value in found_items.items():
        if not isinstance(value, Sequence):
            found_items[key] = [value]

    shape = create_detector_shape_bundle(num_columns=found_items[prefix + "num-columns"],
                                         non_rectangle_width=found_items[prefix + "non-rectangle-width"],
                                         num_rows=found_items[prefix + "num-rows"],
                                         non_rectangle_height=found_items[prefix + "non-rectangle-height"],
                                         num_pixels=found_items[prefix + "num-pixels"])

    return geometry_bundle(shape=shape,
                           first_low_angle_spec_number=found_items["first-low-angle-spec-number"])


class SpectrumBlock(object):
    def __init__(self, data_info, detector_type):
        super(SpectrumBlock, self).__init__()
        self._data_info = data_info
        self._detector_shape = None
        self._first_spectrum_number = None
        self._detector_orientation = None

        # Set the attributes above
        self._set_attributes(data_info, detector_type)

    def _set_attributes(self, data_info, detector_type):
        # Set attributes from IPF
        file_name = data_info.sample_scatter
        _, ipf_path = get_instrument_paths_for_sans_file(file_name)

        geometry = get_geometry_information(ipf_path, detector_type)
        self._detector_shape = geometry.shape
        self._first_spectrum_number = geometry.first_low_angle_spec_number

        # Extract information about the concrete instrument
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(file_name)
        run_number = file_information.get_run_number()
        instrument = file_information.get_instrument()

        # We now have to perform instrument- and run- specific adjustments.
        if instrument is SANSInstrument.SANS2D:
            if run_number < 568:
                self._first_spectrum_number = 1
                self._detector_orientation = DetectorOrientation.Vertical
            elif 568 <= run_number < 684:
                self._first_spectrum_number = 9
                self._detector_orientation = DetectorOrientation.Rotated
            else:
                self._first_spectrum_number = 9
                self._detector_orientation = DetectorOrientation.Horizontal
        elif instrument is SANSInstrument.LARMOR:
            self._first_spectrum_number = 10
            self._detector_orientation = DetectorOrientation.Horizontal
        elif instrument is SANSInstrument.LOQ:
            self._first_spectrum_number = 3
            self._detector_orientation = DetectorOrientation.Horizontal
        else:
            raise RuntimeError("MaskParser: Cannot handle masking request for instrument {0}".format(str(instrument)))

    def get_spectrum_block_string(self, y_lower, x_lower, y_dim, x_dim):
        """
        Create a list of spectra for a rectangular block of size x_dim by y_dim

        :param y_lower: the x coordiante of the starting point of the lower left corner
        :param x_lower: the y coordinate of the starting point of the lower left corner
        :param y_dim: the y dimension
        :param x_dim: the x dimension
        :return: a string which contains the rectangular region of spectra
        """
        # TODO: Check for efficiency
        if y_dim == 'all':
            y_dim = self._detector_shape.height
        if x_dim == 'all':
            x_dim = self._detector_shape.width

        # Warn the user if there is an non-rectangular shape involved.

        # Get the spectrum list depending on the orientation of the detector
        detector_dimension = self._detector_shape.width
        base_spectrum_number = self._first_spectrum_number

        output = ''
        if self._detector_orientation == DetectorOrientation.Horizontal:
            start_spectrum = base_spectrum_number + y_lower * detector_dimension + x_lower
            for y in range(0, y_dim):
                for x in range(0, x_dim):
                    output += str(start_spectrum + x + (y * detector_dimension)) + ','
        elif self._detector_orientation == DetectorOrientation.Vertical:
            start_spectrum = base_spectrum_number + x_lower * detector_dimension + y_lower
            for x in range(detector_dimension - 1, detector_dimension - x_dim - 1, -1):
                for y in range(0, y_dim):
                    output += str(start_spectrum + y + ((detector_dimension - x - 1) * detector_dimension)) + ','
        elif self._detector_orientation == DetectorOrientation.Rotated:
            # This is the horizontal one rotated so need to map the x_low and y_low to their rotated versions
            start_spectrum = base_spectrum_number + y_lower * detector_dimension + x_lower
            max_spectrum = detector_dimension * detector_dimension + base_spectrum_number - 1
            for y in range(0, y_dim):
                for x in range(0, x_dim):
                    std_i = start_spectrum + x + (y * detector_dimension)
                    output += str(max_spectrum - (std_i - base_spectrum_number)) + ','
        return output.rstrip(",")


def mask_error_handler(mask_command):
    logger.notice("MaskParser: Cannot handle command: {0}".format(mask_command))


def is_no_command(mask_command):
    return len(mask_command) == 0


# --------------------------------
# Big piece parsing
# -------------------------------
def is_block(mask_command):
    return "+" in mask_command


def handle_block(mask_command, spectrum_block):
    """
    This command defines a block on the detector.

    The form is, for example, H0>H3+V0>V3. This indicates that all spectra in the square with the
    corners (0,0), (0,3), (3,0), (3,3) are selected
    """
    def get_lower_and_upper(piece):
        if '>' in piece:
            sub_pieces = piece.split('>')
            lower = int(sub_pieces[0].lstrip('hv'))
            upper = int(sub_pieces[1].lstrip('hv'))
        else:
            lower = int(piece.lstrip('hv'))
            upper = lower
        return lower, upper

    # Get the spectra specifiers
    elements = mask_command.split('+')
    lower_1, upper_1 = get_lower_and_upper(elements[0])
    lower_2, upper_2 = get_lower_and_upper(elements[1])

    # Determine which way the command is, i.e. hA>HB+vCvD or vCvD+hA>HB
    if 'h' in elements[0] and 'v' in elements[1]:
        y_dim = abs(upper_1 - lower_1) + 1
        x_dim = abs(upper_2 - lower_2) + 1
        spectrum_list_string = spectrum_block.get_spectrum_block_string(lower_1, lower_2, y_dim, x_dim)
    elif 'v' in elements[0] and 'h' in elements[1]:
        x_dim = abs(upper_1 - lower_1) + 1
        y_dim = abs(upper_2 - lower_2) + 1
        spectrum_list_string = spectrum_block.get_spectrum_block_string(lower_2, lower_1, y_dim, x_dim)
    else:
        mask_error_handler(mask_command)
        spectrum_list_string = ''
    return spectrum_list_string


# --------------------------------
# Range parsing
# -------------------------------
def is_range(mask_command):
    return '>' in mask_command


def handle_range(mask_command, spectrum_block):
    elements = mask_command.split('>')
    lower = int(elements[0].lstrip('hvs'))
    upper = int(elements[1].lstrip('hvs'))
    if 'h' in elements[0]:
        number_of_strips = abs(upper - lower) + 1
        spectrum_list_string = spectrum_block.get_spectrum_block_string(lower, 0, number_of_strips, 'all') + ','
    elif 'v' in elements[0]:
        number_of_strips = abs(upper - lower) + 1
        spectrum_list_string = spectrum_block.get_spectrum_block_string(0, lower, 'all', number_of_strips) + ','
    else:
        spectrum_list_string = ""
        for i in range(lower, upper + 1):
            spectrum_list_string += str(i) + ','
    return spectrum_list_string


# --------------------------------
# Horizontal parsing
# -------------------------------
def is_horizontal_strip(mask_command):
    return 'h' in mask_command


def handle_horizontal_strip(mask_command, spectrum_block):
    element = int(mask_command.lstrip('h'))
    return spectrum_block.get_spectrum_block_string(element, 0, 1, 'all') + ','


# --------------------------------
# Vertical parsing
# -------------------------------
def is_vertical_strip(mask_command):
    return 'v' in mask_command


def handle_vertical_strip(mask_command, spectrum_block):
    element = int(mask_command.lstrip('v'))
    return spectrum_block.get_spectrum_block_string(0, element, 'all', 1) + ','


# --------------------------------
# Single spectrum parsing
# -------------------------------
def is_single_spectrum(mask_command):
    return 's' in mask_command


def handle_single_spectrum(mask_command):
    return mask_command.lstrip('s') + ','


# --------------------------------
# Not supported parsing
# -------------------------------
def is_not_supported(mask_command):
    return len(mask_command.split()) == 4


def handle_not_supported(mask_command):
    logger.notice("MaskParser: The command {0} is currently not supported.".format(mask_command))


# --------------------------------------
#  Parse a masking string
# --------------------------------------
def parse_masking_string(masking_string, spectrum_block=None):
    """
    Converts a masking string to a spectra list

    @param masking_string Is a comma separated list of mask commands for masking spectra
                      using the e.g. the h, s and v commands
    :return: a comma separated list with spectra which were encoded in the masking string
    """
    if not masking_string:
        return []

    masking_list = masking_string.split(',')

    spectrum_list_string = ""
    # Go through each command and translate it into spectra numbers
    # There are several types of commands:
    # 1. Blocks: They are rectangles on the detector specified by H0>H10+V0+V12
    # 2. Ranges:
    # 3. Horizontal strip:
    # 4. Vertical strip:
    # 5. Single spectrum
    # 6. Empty command:
    # 7. Not-supported command:
    for mask_command in masking_list:
        mask_command = mask_command.lower()
        if is_block(mask_command):
            to_mask = handle_block(mask_command, spectrum_block)
        elif is_range(mask_command):
            to_mask = handle_range(mask_command, spectrum_block)
        elif is_horizontal_strip(mask_command):
            to_mask = handle_horizontal_strip(mask_command, spectrum_block)
        elif is_vertical_strip(mask_command):
            to_mask = handle_vertical_strip(mask_command, spectrum_block)
        elif is_single_spectrum(mask_command):
            to_mask = handle_single_spectrum(mask_command)
        elif is_no_command(mask_command):
            to_mask = ""
        elif is_not_supported(mask_command):
            handle_not_supported(mask_command)
            to_mask = ""
        else:
            mask_error_handler(mask_command)
            to_mask = ""
        # Add the to_mask
        if to_mask:
            spectrum_list_string += to_mask

    # Remove any trailing comma
    if spectrum_list_string.endswith(','):
        spectrum_list_string = spectrum_list_string[0:len(spectrum_list_string) - 1]
    return spectrum_list_string
