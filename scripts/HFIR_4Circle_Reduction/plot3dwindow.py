__author__ = 'wzz'

import os
import sys
import numpy as np

from PyQt4 import QtCore, QtGui

import ui_View3DWidget


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

        # Set up

        #
        self._dataKeyList = list()

        return

    def add_plot_by_file(self, file_name):
        """
        Add a 3D plot via a file
        :return:
        """
        data_key = self.ui.graphicsView.import_data_from_file(file_name)
        self._dataKeyList.append(data_key)

        return data_key

    def add_plot_by_array(self, points, intensity):
        """
        Add a 3D plot via ndarrays
        :param points:
        :param intensity:
        :return:
        """
        data_key = self.ui.graphicsView.import_3d_data(points, intensity)
        self._dataKeyList.append(data_key)

        return data_key

    def clear_plots(self):
        """

        :return:
        """
        self.ui.graphicsView.clear_3d_plots()
        self._dataKeyList = list()

        return

    def plot(self, data_key_list, base_color_list):
        """

        :param data_key_list:
        :param base_color_list:
        :return:
        """
        # TODO/DOC/CHECK

        for i_plot in xrange(len(data_key_list)):
            data_key = data_key_list[i_plot]
            base_color = base_color_list[i_plot]
            self.ui.graphicsView.plot_scatter(data_key, base_color)


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
        intensities2[j] = raw_list[i][3]

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
