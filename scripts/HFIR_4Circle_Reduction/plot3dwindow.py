#pylint: disable=C0103,W0403
import numpy as np
from PyQt4 import QtGui, QtCore

import ui_View3DWidget
import guiutility

__author__ = 'wzz'


class Plot3DWindow(QtGui.QMainWindow):
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
        QtGui.QMainWindow.__init__(self, parent)

        self.ui = ui_View3DWidget.Ui_MainWindow()
        self.ui.setupUi(self)

        # Initialize widgets
        self.ui.lineEdit_baseColorRed.setText('0.5')
        self.ui.lineEdit_baseColorGreen.setText('0.5')
        self.ui.lineEdit_baseColorBlue.setText('0.5')

        # Event handling
        self.connect(self.ui.pushButton_plot3D, QtCore.SIGNAL('clicked()'),
                     self.do_plot_3d)
        self.connect(self.ui.pushButton_checkCounts, QtCore.SIGNAL('clicked()'),
                     self.do_check_counts)
        self.connect(self.ui.pushButton_clearPlots, QtCore.SIGNAL('clicked()'),
                     self.do_clear_plots)
        self.connect(self.ui.pushButton_quit, QtCore.SIGNAL('clicked()'),
                     self.do_quit)

        # Set up
        # list of data keys for management
        self._dataKeyList = list()

        # dictionary for 3D data
        self._mergedDataDict = dict()

        return

    def do_clear_plots(self):
        """
        Clear all the plots from the canvas
        :return:
        """
        self.ui.graphicsView.clear_3d_plots()
        self._dataKeyList = list()

        return

    def do_check_counts(self):
        """ Check the intensity and count how many data points are above threshold of specified data-key
        :return:
        """
        # get threshold
        status, ret_obj = guiutility.parse_integers_editors([self.ui.lineEdit_countsThreshold])
        assert status, ret_obj
        threshold = ret_obj
        assert isinstance(threshold, int) and threshold >= 0

        # get data key
        data_key = int(self.ui.comboBox_dataKey.currentText())
        assert data_key in self._dataKeyList, 'Data key %d does not exist in key list %s.' % (data_key,
                                                                                              str(self._dataKeyList))

        # get intensity
        points, intensity_array = self.ui.graphicsView.get_data(data_key)
        assert points is not None
        num_above_threshold = 0
        array_size = len(intensity_array)
        for index in xrange(array_size):
            if intensity_array[index] >= threshold:
                num_above_threshold += 1
        # END-FOR

        # set value
        self.ui.label_numberDataPoints.setText('%d' % num_above_threshold)

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
        :return:
        """
        data_key = self.ui.graphicsView.import_data_from_file(file_name)
        self._dataKeyList.append(data_key)

        self.ui.comboBox_dataKey.addItem(str(data_key))

        return data_key

    def add_plot_by_array(self, points, intensities):
        """
        Add a 3D plot via ndarrays
        :param points:
        :param intensities:
        :return:
        """
        # check
        assert isinstance(points, np.ndarray) and points.shape[1] == 3, 'Shape is %s.' % str(points.shape)
        assert isinstance(intensities, np.ndarray) and len(points) == len(intensities)

        data_key = self.ui.graphicsView.import_3d_data(points, intensities)
        self._dataKeyList.append(data_key)

        self.ui.comboBox_dataKey.addItem(str(data_key))

        return data_key

    def do_plot_3d(self):
        """

        :return:
        """
        # color: get R, G, B
        status, rgb_values = guiutility.parse_float_editors([self.ui.lineEdit_baseColorRed,
                                                             self.ui.lineEdit_baseColorGreen,
                                                             self.ui.lineEdit_baseColorBlue])
        assert status

        # set the color to get change
        change_r = self.ui.checkBox_changeRed.isChecked()
        change_g = self.ui.checkBox_changeGreen.isChecked()
        change_b = self.ui.checkBox_changeBlue.isChecked()

        # get threshold
        status, threshold = guiutility.parse_integers_editors([self.ui.lineEdit_countsThreshold])
        assert status

        # data key
        data_key = int(self.ui.comboBox_dataKey.currentText())

        # plot
        self.plot_3d([data_key], rgb_values, threshold, [change_r, change_g, change_b])

        return

    def plot_3d(self, data_key, base_color, threshold, change_color):
        """
        Plot scatter data with specified base color
        Requirements: data key does exist.  color values are within (0, 1)
        :param data_key:
        :param base_color:
        :param threshold:
        :param change_color: [change_R, change_G, change_B]
        :return:
        """
        # Check
        assert isinstance(data_key, int)
        assert isinstance(base_color, list)
        assert isinstance(threshold, int) and threshold >= 0
        assert isinstance(change_color, list)

        # Reset data
        points, intensities = self.ui.graphicsView.get_data(data_key)

        if threshold > 0:
            # threshold is larger than 0, then filter the data
            points, intensities = filter_points_by_intensity(points, intensities, threshold)

        # Set limit
        self.ui.graphicsView.set_xyz_limits(points, None)

        # Format intensity to color map
        color_list = map_to_color(intensities, base_color, change_color)

        # self.ui.graphicsView.plot_scatter(data_key, base_color)
        self.ui.graphicsView.set_xyz_limits()
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


def filter_points_by_intensity(points, intensities, threshold):
    """ Filter the data points by intensity threshold
    :param points:
    :param intensities:
    :param threshold:
    :return: filtered data points and intensities (2-tuple of ndarray)
    """
    # check
    assert len(points) == len(intensities)

    # calculate data size
    raw_array_size = len(intensities)
    new_array_size = 0
    for index in xrange(raw_array_size):
        if intensities[index] >= threshold:
            new_array_size += 1
    # END-FOR

    # initialize output arrays
    new_points = np.ndarray(shape=(new_array_size, 3), dtype='float')
    new_intensities = np.ndarray(shape=(new_array_size,), dtype='float')
    new_index = 0
    for raw_index in xrange(raw_array_size):
        if intensities[raw_index] >= threshold:
            assert new_index < new_array_size
            new_points[new_index] = points[raw_index]
            new_intensities[new_index] = intensities[raw_index]
            new_index += 1
    # END-FOR

    return new_points, new_intensities
