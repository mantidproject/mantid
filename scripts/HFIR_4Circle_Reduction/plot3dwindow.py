#pylint: disable=C0103
import sys
import numpy as np
from PyQt4 import QtGui, QtCore

import ui_View3DWidget
import guiutility

__author__ = 'wzz'


class Plot3DWindow(QtGui.QMainWindow):
    """

    """
    def __init__(self, parent=None):
        """

        :param parent:
        :return:
        """
        # Init
        QtGui.QMainWindow.__init__(self, parent)

        self.ui = ui_View3DWidget.Ui_MainWindow()
        self.ui.setupUi(self)

        # Event handling
        self.connect(self.ui.pushButton_plot3D, QtCore.SIGNAL('clicked()'),
                     self.do_plot_3d)

        # Set up

        # list of data keys for management
        self._dataKeyList = list()

        # dictionary for 3D data
        self._mergedDataDict = dict()

        return

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

    def clear_plots(self):
        """

        :return:
        """
        self.ui.graphicsView.clear_3d_plots()
        self._dataKeyList = list()

        return

    def do_plot_3d(self):
        """

        :return:
        """
        # color: get R, G, B
        color_list_str = str(self.ui.lineEdit_baseColorList.text())
        status, base_color_list = guiutility.parse_float_array(color_list_str)
        assert len(base_color_list) == 3

        # set the color to get change
        blabla()

        # get threshold
        blabla()

        # data key
        data_key = int(self.ui.comboBox_dataKey.currentText())

        # plot
        self.plot_3d([data_key], [base_color_list], threshold, [True, True, True])

        return

    def plot_3d(self, data_key, base_color, threshold, change_color):
        """
        Plot scatter data with specified base color
        :param data_key:
        :param base_color:
        :param change_color: [change_R, change_G, change_B]
        :return:
        """
        # Check
        assert isinstance(data_key, int)
        assert isinstance(base_color, list)
        assert isinstance(threshold, int)
        assert isinstance(change_color, list)

        # Reset data
        if threshold > 0:
            points, intensities = blabla()
        else:
            points, intensities = blabla()

        # Set limit
        blabla()

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
            # set up the scan list
            self.ui.comboBox_scans.addItem(str(scan_number))
        # END-FOR

        return


if __name__ == "__main__":
    mainApp = QtGui.QApplication(sys.argv)

    myapp = Plot3DWindow()

    # Test set up
    # 3D data set
    raw_list = [
        [0.887649, -0.360632, 4.081141, 14.000000],
        [0.887110, -0.360738, 4.082823, 11.000000],
        [0.887256, -0.360679, 4.082371, 17.000000],
        [0.887390, -0.360624, 4.081962, 9.0000000],
        [0.887546, -0.360565, 4.081490, 7.0000000],
        [0.887678, -0.360511, 4.081094, 21.000000],
        [0.887103, -0.360621, 4.082837, 238.00000],
        [0.887244, -0.360565, 4.082411, 755.00000],
        [0.887386, -0.360510, 4.081990, 3135.0000],
        [0.887538, -0.360452, 4.081541, 5426.0000],
        [0.887680, -0.360398, 4.081130, 7724.0000],
        [0.887070, -0.360512, 4.082922, 8167.0000],
        [0.887210, -0.360458, 4.082512, 6717.0000],
        [0.887351, -0.360404, 4.082106, 3835.0000],
        [0.887491, -0.360350, 4.081704, 856.00000],
        [0.887621, -0.360299, 4.081337, 108.00000],
        [0.887774, -0.360244, 4.080907, 17.000000],
        [0.887168, -0.360353, 4.082634, 11.000000],
        [0.887309, -0.360300, 4.082234, 11.000000],
        [0.887452, -0.360247, 4.081836, 5.0000000],
        [0.887607, -0.360192, 4.081407, 16.000000]
        ]

    num_pt = len(raw_list)
    centers = np.ndarray((num_pt, 3), 'double')
    intensities2 = np.ndarray((num_pt, ), 'double')
    for i in xrange(num_pt):
        for j in xrange(3):
            centers[i][j] = raw_list[i][j]
        intensities2[i] = raw_list[i][3]

    data_key2 = myapp.add_plot_by_array(centers, intensities2)

    # Single 3D data
    avg_center = np.ndarray((1, 3), 'double')
    avg_center[0][0] = 0.88735938499471179
    avg_center[0][1] = -0.36045625545762
    avg_center[0][2] = 4.0820727566625354
    intensities3 = np.ndarray((1,), 'double')
    intensities3[0] = 10000.
    data_key3 = myapp.add_plot_by_array(avg_center, intensities3)

    myapp.plot([data_key2, data_key3], [(0.01, 0.99, 0.1), (0.99, 0.1, 0.1)])

    # Show
    myapp.show()

    sys.exit(mainApp.exec_())
