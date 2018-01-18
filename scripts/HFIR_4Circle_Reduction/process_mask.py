import math
import numpy
import re
import xml.etree.ElementTree as ET
from mantid.api import AnalysisDataService as ADS


# This module is still in development


def parse_mask(xml_name):

    # get root and 'Data' ndoe
    tree = ET.parse(xml_name)
    root = tree.getroot()
    main_mask_node = None
    for child in root:
        if child.tag == 'group':
            main_mask_node = child
            break

    # parse detector ID list string to array

    det_list_node = main_mask_node.find('detids')
    det_list_str = det_list_node.text

    det_range_list = re.split(',', det_list_str)

    for det_range in det_range_list:
        print det_range

    # int_count = 0

    # det_list_array = numpy.ndarray(shape=(2359296,), dtype='int')

    # index = 0
    # for count in det_list_count_list:
    #     if len(count) > 0:
    #         count_int = int(count)
    #         det_list_array[index] = count_int
    #         index += 1
    #     else:
    #         int_count += 1

    # print (int_count)


def get_region_of_interest(mask_ws_name):
    """
    get region of interest from mask workspace
    :param mask_ws_name:
    :return:
    """
    # check input
    assert isinstance(mask_ws_name, str), 'Mask workspace name {0} must be an integer but not a {1}' \
                                          ''.format(mask_ws_name, type(mask_ws_name))
    mask_ws = ADS.retrieve(mask_ws_name)

    # construct a 2D matrix
    size_x = size_y = int(math.sqrt(mask_ws.getNumberHistograms()))
    mask_matrix = numpy.ndarray(shape=(size_x, size_y), dtype='int')

    # mask or unmask all the matrix element according to mask workspace
    for iws in range(mask_ws.getNumberHistograms()):
        det_id = mask_ws.getDetector(iws).getID()
        pixel_2d_id = det_id / size_y, det_id % size_y
        mask_matrix[pixel_2d_id] = int(mask_ws.isMasked(iws))
    # END-FOR

    # find lower left corner
    lower_left_corner = None
    for ir in range(size_y):
        if mask_matrix[ir].min() == 0:
            ll_row = ir
            ret_value = numpy.where(mask_matrix[ir] == 0)
            ll_col = ret_value[0][0]
            lower_left_corner = ll_row, ll_col
            break
        # END-IF
    # END-FOR

    # find upper right corner
    upper_right_corner = None
    for ir in range(size_y-1, -1, -1):
        if mask_matrix[ir].min() == 0:
            ur_row = ir
            ret_value = numpy.where(mask_matrix[ir] == 0)
            ur_col = ret_value[0][-1]
            upper_right_corner = ur_row, ur_col
            break
        # END-IF
    # END-FOR

    # check before return
    if lower_left_corner is None or upper_right_corner is None:
        raise RuntimeError('It is impossible not to find either lower left corner {0} or upper right corner {1}'
                           ''.format(lower_left_corner, upper_right_corner))

    return lower_left_corner, upper_right_corner


if __name__ == '__main__':
    xml_name = 'temp_mask_635_61.xml'
    parse_mask(xml_name)
