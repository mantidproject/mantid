# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import math
import numpy
import re
import xml.etree.ElementTree as ET
from mantid.api import AnalysisDataService as ADS


# This module is still in development
def parse_mask(xml_name):
    """
    parse mask XML file
    :param xml_name:
    :return:
    """
    # get root and 'Data' ndoe
    tree = ET.parse(xml_name)
    root = tree.getroot()
    main_mask_node = None
    for child in root:
        if child.tag == "group":
            main_mask_node = child
            break

    # parse detector ID list string to array

    det_list_node = main_mask_node.find("detids")
    det_list_str = det_list_node.text

    det_range_list = re.split(",", det_list_str)

    for det_range in det_range_list:
        print(det_range)

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
    assert isinstance(mask_ws_name, str), "Mask workspace name {0} must be an integer but not a {1}" "".format(
        mask_ws_name, type(mask_ws_name)
    )
    mask_ws = ADS.retrieve(mask_ws_name)

    # construct a 2D matrix
    size_x = size_y = int(math.sqrt(mask_ws.getNumberHistograms()))
    mask_matrix = numpy.ndarray(shape=(size_x, size_y), dtype="int")

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
    for ir in range(size_y - 1, -1, -1):
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
        raise RuntimeError(
            "It is impossible not to find either lower left corner {0} or upper right corner {1}" "".format(
                lower_left_corner, upper_right_corner
            )
        )

    return lower_left_corner, upper_right_corner


class RegionOfInterest(object):
    """
    a class to manage region of interest used in application including mask workspace and ROI information
    """

    def __init__(self, roi_name):
        """
        initialization with unchangeable ROI name
        :param roi_name:
        """
        assert isinstance(roi_name, str), "ROI name must be a string"

        self._roiName = roi_name
        self._maskWorkspaceName = None
        self._lowerLeftCorner = None
        self._upperRightCorner = None

        return

    @property
    def mask_workspace(self):
        """
        get name of mask workspace
        :return:
        """
        return self._maskWorkspaceName

    @property
    def lower_left_corner(self):
        """
        get lower left corner position
        :return: 2-tuple
        """
        if self._lowerLeftCorner is None:
            raise RuntimeError("lower left not set")

        return self._lowerLeftCorner

    @property
    def upper_right_corner(self):
        """
        get upper right corner position
        :return: 2-tuple
        """
        if self._upperRightCorner is None:
            raise RuntimeError("upper right not set")

        return self._upperRightCorner

    def set_mask_workspace_name(self, ws_name):
        """
        set mask workspace name to this instance
        :param ws_name:
        :return:
        """
        assert isinstance(ws_name, str), "Mask workspace name {0} must be a string but not a {1}" "".format(ws_name, type(ws_name))

        # check workspace existing and type
        if ADS.doesExist(ws_name) is False:
            raise RuntimeError("Workspace {0} does not exist in ADS.")
        workspace = ADS.retrieve(ws_name)
        if workspace.id() != "MaskWorkspace":
            raise RuntimeError("Workspace {0} is not a MaskWorkspace but a {1}" "".format(ws_name, workspace.id()))

        self._maskWorkspaceName = ws_name

        return

    def set_roi_positions(self, lower_left_corner, upper_right_corner):
        """
        blabla
        :param lower_left_corner:
        :param upper_right_corner:
        :return:
        """
        # check input type and size
        assert not isinstance(lower_left_corner, str) and len(lower_left_corner) == 2, "Lower left corner {0} must be a 2-tuple.".format(
            lower_left_corner
        )
        assert not isinstance(upper_right_corner, str) and len(upper_right_corner) == 2, "Upper right corner {0} must be a 2-tuple".format(
            upper_right_corner
        )

        ll_x = int(lower_left_corner[0])
        ll_y = int(lower_left_corner[1])
        ur_x = int(upper_right_corner[0])
        ur_y = int(upper_right_corner[1])
        if ll_x == ur_x or ll_y == ur_y:
            err_msg = "Lower left corner ({0}, {1}) and upper right corner are in a line ({2}, {3})" "".format(ll_x, ll_y, ur_x, ur_y)
            raise RuntimeError(err_msg)

        self._lowerLeftCorner = min(ll_x, ur_x), min(ll_y, ur_y)
        self._upperRightCorner = max(ll_x, ur_x), max(ll_y, ur_y)

        return
