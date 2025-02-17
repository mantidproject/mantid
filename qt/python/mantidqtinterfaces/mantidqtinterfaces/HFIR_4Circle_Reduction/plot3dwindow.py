# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=C0103,W0403
import sys
import numpy as np
from qtpy.QtWidgets import QMainWindow
from mantid.kernel import Logger

try:
    from mantidqt.utils.qt import load_ui
except ImportError:
    Logger("HFIR_4Circle_Reduction").information("Using legacy ui importer")
    from mantidplot import load_ui
from qtpy.QtWidgets import QVBoxLayout


from mantidqtinterfaces.HFIR_4Circle_Reduction.mplgraphicsview3d import MplPlot3dCanvas
from mantidqtinterfaces.HFIR_4Circle_Reduction import guiutility

__author__ = "wzz"


class Plot3DWindow(QMainWindow):
    """
    Main window to view merged data in 3D
    """

    def __init__(self, parent=None):
        """
        Initialization
        :param parent:
        :return:
        """
        # Init
        QMainWindow.__init__(self, parent)

        ui_path = "View3DWidget.ui"
        self.ui = load_ui(__file__, ui_path, baseinstance=self)
        self._promote_widgets()

        # Initialize widgets
        self.ui.lineEdit_baseColorRed.setText("0.5")
        self.ui.lineEdit_baseColorGreen.setText("0.5")
        self.ui.lineEdit_baseColorBlue.setText("0.5")
        self.ui.lineEdit_countsThresholdLower.setText("0")
        self.ui.comboBox_scans.addItem("unclassified")

        # Event handling
        self.ui.pushButton_plot3D.clicked.connect(self.do_plot_3d)
        self.ui.pushButton_checkCounts.clicked.connect(self.do_check_counts)
        self.ui.pushButton_clearPlots.clicked.connect(self.do_clear_plots)
        self.ui.pushButton_quit.clicked.connect(self.do_quit)
        self.ui.comboBox_scans.currentIndexChanged.connect(self.evt_change_scan)

        # Set up
        # list of data keys for management
        self._dataKeyList = list()

        # dictionary for 3D data
        self._mergedDataDict = dict()

        # dictionary for group control, key = str(scan), value = list of data keys
        self._groupDict = dict()
        self._currSessionName = "unclassified"
        self._groupDict["unclassified"] = []

        return

    def _promote_widgets(self):
        graphicsView_layout = QVBoxLayout()
        self.ui.frame_graphicsView.setLayout(graphicsView_layout)
        self.ui.graphicsView = MplPlot3dCanvas(self)
        graphicsView_layout.addWidget(self.ui.graphicsView)

        return

    def close_session(self):
        """Close session
        :return:
        """
        self._currSessionName = "unclassified"

        return

    def initialize_group(self, session_key):
        """
        Create a 'virtual' session to group input data
        :param session_key:
        :return:
        """
        assert isinstance(session_key, str)

        # store original one and clear
        if session_key in self._groupDict:
            self._groupDict["unclassified"].extend(self._groupDict[session_key])
            self._groupDict[session_key] = []
            self._currSessionName = session_key

        self.ui.comboBox_scans.addItem(session_key)

        return

    def do_clear_plots(self):
        """
        Clear all the plots from the canvas
        :return:
        """
        self.ui.graphicsView.clear_3d_plots()
        self._dataKeyList = list()
        self.ui.comboBox_dataKey.clear()

        return

    def do_check_counts(self):
        """Check the intensity and count how many data points are above threshold of specified data-key
        :return:
        """
        # get threshold (upper and lower)
        status, ret_obj = guiutility.parse_integers_editors(
            [self.ui.lineEdit_countsThresholdLower, self.ui.lineEdit_countsThresholdUpper], allow_blank=True
        )
        assert status, ret_obj
        threshold_lower, threshold_upper = ret_obj
        if threshold_lower is None:
            threshold_lower = 0
        if threshold_upper is None:
            threshold_upper = sys.maxsize
        assert 0 <= threshold_lower < threshold_upper

        # get data key
        data_key = int(self.ui.comboBox_dataKey.currentText())
        assert data_key in self._dataKeyList, "Data key %d does not exist in key list %s." % (data_key, str(self._dataKeyList))

        # get intensity
        points, intensity_array = self.ui.graphicsView.get_data(data_key)
        assert points is not None
        num_within_threshold = 0
        array_size = len(intensity_array)
        for index in range(array_size):
            if threshold_lower <= intensity_array[index] <= threshold_upper:
                num_within_threshold += 1
        # END-FOR

        # set value
        self.ui.label_numberDataPoints.setText("%d" % num_within_threshold)

        return

    def do_quit(self):
        """
        Close the window
        :return:
        """
        self.close()

    def add_plot_by_file(self, file_name):
        """
        Add a 3D plot via a file
        :param file_name:
        :return:
        """
        data_key = self.ui.graphicsView.import_data_from_file(file_name)
        self._dataKeyList.append(data_key)

        # add to box
        self.ui.comboBox_dataKey.addItem(str(data_key))
        self._groupDict[self._currSessionName].append(data_key)

        return data_key

    def add_plot_by_array(self, points, intensities):
        """
        Add a 3D plot via ndarrays
        :param points:
        :param intensities:
        :return:
        """
        # check
        assert isinstance(points, np.ndarray) and points.shape[1] == 3, "Shape is %s." % str(points.shape)
        assert isinstance(intensities, np.ndarray) and len(points) == len(intensities)

        data_key = self.ui.graphicsView.import_3d_data(points, intensities)
        self._dataKeyList.append(data_key)

        # add to combo box and group managing dictionary
        self.ui.comboBox_dataKey.addItem(str(data_key))
        self._groupDict[self._currSessionName].append(data_key)

        return data_key

    def do_plot_3d(self):
        """

        :return:
        """
        # color: get R, G, B
        status, rgb_values = guiutility.parse_float_editors(
            [self.ui.lineEdit_baseColorRed, self.ui.lineEdit_baseColorGreen, self.ui.lineEdit_baseColorBlue]
        )
        assert status

        # set the color to get change
        change_r = self.ui.checkBox_changeRed.isChecked()
        change_g = self.ui.checkBox_changeGreen.isChecked()
        change_b = self.ui.checkBox_changeBlue.isChecked()

        # get threshold
        status, thresholds = guiutility.parse_integers_editors(
            [self.ui.lineEdit_countsThresholdLower, self.ui.lineEdit_countsThresholdUpper], allow_blank=True
        )
        assert status, thresholds
        if thresholds[0] is None:
            thresholds[0] = 0
        if thresholds[1] is None:
            thresholds[1] = sys.maxsize
        assert 0 <= thresholds[0] < thresholds[1]

        # data key
        data_key = int(self.ui.comboBox_dataKey.currentText())

        # plot
        self.plot_3d(data_key, rgb_values, thresholds, [change_r, change_g, change_b])

        return

    def evt_change_scan(self):
        """Handling the event that the scan number is changed
        :return:
        """
        # get session name
        session_name = str(self.ui.comboBox_scans.currentText())

        # clear data key
        self.ui.comboBox_dataKey.clear()

        # reset data key
        for data_key in self._groupDict[session_name]:
            self.ui.comboBox_dataKey.addItem(str(data_key))

        return

    def plot_3d(self, data_key, base_color, thresholds, change_color):
        """
        Plot scatter data with specified base color
        Requirements: data key does exist.  color values are within (0, 1)
        :param data_key:
        :param base_color:
        :param threshold: list of 2-item
        :param change_color: [change_R, change_G, change_B]
        :return:
        """
        # Check
        assert isinstance(data_key, int), "Date key %s must be an integer but not %s" % (str(data_key), str(type(data_key)))
        assert isinstance(base_color, list)
        assert isinstance(thresholds, list) and len(thresholds) == 2
        assert isinstance(change_color, list)

        lower_boundary, upper_boundary = thresholds
        assert 0 <= lower_boundary < upper_boundary

        # Reset data
        points, intensities = self.ui.graphicsView.get_data(data_key)

        if lower_boundary > min(intensities) or upper_boundary < max(intensities):
            # threshold is larger than 0, then filter the data
            points, intensities = filter_points_by_intensity(points, intensities, lower_boundary, upper_boundary)

        # Set limit
        self.ui.graphicsView.set_xyz_limits(points, None)

        # Format intensity to color map
        color_list = guiutility.map_to_color(intensities, base_color, change_color)
        # print(color_list)
        assert len(color_list) == len(points)

        # self.ui.graphicsView.plot_scatter(data_key, base_color)
        self.ui.graphicsView.set_xyz_limits(points)
        self.ui.graphicsView.plot_scatter(points, color_list)

        return

    def set_merged_data_set(self, merged_data_set):
        """
        Set up a set of merged data including scan number and data in 3D
        Requirements: each merged data is a 2-tuple as scan number and 3D data
        :param merged_data_set:
        :return:
        """
        # check
        assert isinstance(merged_data_set, list)

        # set up
        for merged_data in merged_data_set:
            scan_number = merged_data[0]
            assert isinstance(scan_number, int)
            # add data to storage
            md_data = merged_data[1]
            self._mergedDataDict[scan_number] = md_data
            self._dataKeyList.append(scan_number)
            # set up the scan list
            self.ui.comboBox_scans.addItem(str(scan_number))
        # END-FOR

        return


def filter_points_by_intensity(points, intensities, lower_boundary, upper_boundary):
    """Filter the data points by intensity threshold
    :param points:
    :param intensities:
    :param lower_boundary:
    :param upper_boundary:
    :return: filtered data points and intensities (2-tuple of ndarray)
    """
    # check
    assert len(points) == len(intensities)

    # calculate data size
    raw_array_size = len(intensities)
    new_array_size = 0
    for index in range(raw_array_size):
        if lower_boundary <= intensities[index] <= upper_boundary:
            new_array_size += 1
    # END-FOR

    # initialize output arrays
    new_points = np.ndarray(shape=(new_array_size, 3), dtype="float")
    new_intensities = np.ndarray(shape=(new_array_size,), dtype="float")
    new_index = 0
    for raw_index in range(raw_array_size):
        if lower_boundary <= intensities[raw_index] <= upper_boundary:
            assert new_index < new_array_size
            new_points[new_index] = points[raw_index]
            new_intensities[new_index] = intensities[raw_index]
            new_index += 1
    # END-FOR

    return new_points, new_intensities
