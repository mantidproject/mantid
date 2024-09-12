# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=R0901,R0902,R0904

import numpy as np
import os

from qtpy.QtWidgets import QSizePolicy
from mantidqt.MPLwidgets import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from mpl_toolkits.mplot3d import Axes3D


class MplPlot3dCanvas(FigureCanvas):
    """
    Matplotlib 3D canvas class
    """

    def __init__(self, parent=None):
        """
        Initialization
        :return:
        """
        #
        self._myParentWindow = parent

        # Initialize the figure
        self._myFigure = Figure()

        # Init canvas
        FigureCanvas.__init__(self, self._myFigure)
        FigureCanvas.setSizePolicy(self, QSizePolicy.Expanding, QSizePolicy.Expanding)
        FigureCanvas.updateGeometry(self)

        # Axes
        self._myAxes = Axes3D(self._myFigure, auto_add_to_figure=False)  # Canvas figure must be created for mouse rotation
        self._myFigure.add_axes(self._myAxes)
        self.format_coord_org = self._myAxes.format_coord
        self._myAxes.format_coord = self.report_pixel

        # color
        self._colorMap = [0.5, 0.5, 0.5]

        # Others
        self._dataKey = 0
        self._dataDict = dict()

        # List of plots on canvas NOW
        self._currPlotList = list()
        self._currSurfaceList = list()  # [{"xx":,"yy:","val:"}]

        return

    def clear_3d_plots(self):
        """
        Clear all the figures from canvas
        :return:
        """
        for plt in self._currPlotList:
            # del plt
            plt.remove()
        self._currPlotList = []

        return

    def get_data(self, data_key):
        """Get data by data key
        :param data_key:
        :return:
        """
        assert data_key in self._dataDict, "Data key %s does not exist in %s." % (str(data_key), str(self._dataDict.keys()))

        return self._dataDict[data_key]

    def import_3d_data(self, points, intensities):
        """

        :param points:
        :param intensities:
        :return:
        """
        # check
        assert isinstance(points, np.ndarray) and points.shape[1] == 3, "Shape is %s." % str(points.shape)
        assert isinstance(intensities, np.ndarray) and len(points) == len(intensities)

        # set
        self._dataDict[self._dataKey] = (points, intensities)

        # update
        r_value = self._dataKey
        self._dataKey += 1

        return r_value

    def import_data_from_file(self, file_name):
        """File will have more than 4 columns, as X, Y, Z, Intensity, ...
        :param file_name:
        :return:
        """
        # check
        assert isinstance(file_name, str) and os.path.exists(file_name)

        # parse
        data_file = open(file_name, "r")
        raw_lines = data_file.readlines()
        data_file.close()

        # construct ND data array
        xyz_points = np.zeros((len(raw_lines), 3))
        intensities = np.zeros((len(raw_lines),))

        # parse
        for i in range(len(raw_lines)):
            line = raw_lines[i].strip()

            # skip empty line
            if len(line) == 0:
                continue

            # set value
            terms = line.split(",")
            for j in range(3):
                xyz_points[i][j] = float(terms[j])
            intensities[i] = float(terms[3])
        # END-FOR

        # Add to data structure for managing
        self._dataDict[self._dataKey] = (xyz_points, intensities)
        return_value = self._dataKey
        self._dataKey += 1

        return return_value

    def plot_scatter(self, points, color_list):
        """
        Plot points with colors in scatter mode
        :param points:
        :param color_list:
        :return:
        """
        # check: [TO DO] need MORE!
        assert isinstance(points, np.ndarray)
        assert len(points) == len(color_list)
        assert points.shape[1] == 3, "3D data %s." % str(points.shape)

        #
        # plot scatters
        plt = self._myAxes.scatter(points[:, 0], points[:, 1], points[:, 2], zdir="z", c=color_list)
        self._currPlotList.append(plt)

        self.draw()

        return

    def plot_scatter_auto(self, data_key, base_color=None):
        """
        Plot data in scatter plot in an automatic mode
        :param data_key: key to locate the data stored to this class
        :param base_color: None or a list of 3 elements from 0 to 1 for RGB
        :return:
        """
        # Check
        assert isinstance(data_key, int) and data_key >= 0
        assert base_color is None or len(base_color) == 3

        # get data and check
        points = self._dataDict[data_key][0]
        intensities = self._dataDict[data_key][1]

        assert isinstance(points, np.ndarray)
        assert isinstance(points.shape, tuple)
        assert points.shape[1] == 3, "3D data %s." % str(points.shape)

        if len(points) > 1:
            # set x, y and z limit
            x_min = min(points[:, 0])
            x_max = max(points[:, 0])
            d_x = x_max - x_min
            # print(x_min, x_max)
            y_min = min(points[:, 1])
            y_max = max(points[:, 1])
            d_y = y_max - y_min
            # print(y_min, y_max)
            z_min = min(points[:, 2])
            z_max = max(points[:, 2])
            d_z = z_max - z_min
            print(z_min, z_max)

            # use default setup
            self._myAxes.set_xlim(x_min - d_x, x_max + d_x)
            self._myAxes.set_ylim(y_min - d_y, y_max + d_y)
            self._myAxes.set_zlim(z_min - d_z, z_max + d_z)
        # END-IF

        # color map for intensity
        color_list = list()
        if base_color is None:
            color_r = self._colorMap[0]
            color_g = self._colorMap[1]
        else:
            color_r = base_color[0]
            color_g = base_color[1]

        if len(intensities) > 1:
            min_intensity = min(intensities)
            max_intensity = max(intensities)
            diff = max_intensity - min_intensity
            b_list = intensities - min_intensity
            b_list = b_list / diff

            num_points = len(points[:, 2])
            for index in range(num_points):
                color_tup = (color_r, color_g, b_list[index])
                color_list.append(color_tup)
        else:
            color_list.append((color_r, color_g, 0.5))

        # plot scatters
        self._myAxes.scatter(points[:, 0], points[:, 1], points[:, 2], zdir="z", c=color_list)

        self.draw()

    def plot_surface(self):
        """
        Plot surface
        :return:
        """
        print("Number of surf = ", len(self._currSurfaceList))
        for surf in self._currSurfaceList:
            plt = self._myAxes.plot_surface(
                surf["xx"],
                surf["yy"],
                surf["val"],
                rstride=5,
                cstride=5,
                linewidth=1,
                antialiased=True,  # color map??? cmap=cm.jet,
            )
            self._currPlotList.append(plt)
        # END-FOR

        return

    def report_pixel(self, x_d, y_d):
        report = self.format_coord_org(x_d, y_d)
        report = report.replace(",", " ")
        return report

    def set_axes_labels(self, x_label, y_label, z_label):
        """

        :return:
        """
        if x_label is not None:
            self._myAxes.set_xlabel(x_label)

        if y_label is not None:
            self._myAxes.set_ylabel(y_label)

        if z_label is not None:
            self._myAxes.set_zlabel(z_label)

        return

    def set_color_map(self, color_r, color_g, color_b):
        """
        Set the base line of color map
        :param color_r:
        :param color_g:
        :param color_b:
        :return:
        """
        # Set color map
        assert isinstance(color_r, float), 0 <= color_r < 1.0
        assert isinstance(color_g, float), 0 <= color_g < 1.0
        assert isinstance(color_b, float), 0 <= color_b < 1.0

        self._colorMap = [color_r, color_g, color_b]

    def set_title(self, title, font_size):
        """
        Set super title
        :param title:
        :return:
        """
        self._myFigure.suptitle(title, fontsize=font_size)

        return

    def set_xyz_limits(self, points, limits=None):
        """Set XYZ axes limits
        :param points:
        :param limits: if None, then use default; otherwise, 3-tuple of 2-tuple
        :return:
        """
        # check
        assert isinstance(points, np.ndarray)

        # get limit
        if limits is None:
            limits = get_auto_xyz_limit(points)

        # set limit to axes
        self._myAxes.set_xlim(limits[0][0], limits[0][1])
        self._myAxes.set_ylim(limits[1][0], limits[1][1])
        self._myAxes.set_zlim(limits[2][0], limits[2][1])

        return


def get_auto_xyz_limit(points):
    """Get default limit on X, Y, Z
    Requirements: number of data points must be larger than 0.
    :param points:
    :return: 3-tuple of 2-tuple as (min, max) for X, Y and Z respectively
    """
    # check
    assert isinstance(points, np.ndarray)
    dim = points.shape[1]
    assert dim == 3

    # set x, y and z limit
    x_min = min(points[:, 0])
    x_max = max(points[:, 0])
    d_x = x_max - x_min

    # print(x_min, x_max)
    y_min = min(points[:, 1])
    y_max = max(points[:, 1])
    d_y = y_max - y_min

    # print(y_min, y_max)
    z_min = min(points[:, 2])
    z_max = max(points[:, 2])
    d_z = z_max - z_min
    print(z_min, z_max)

    # use default setup
    x_lim = (x_min - d_x, x_max + d_x)
    y_lim = (y_min - d_y, y_max + d_y)
    z_lim = (z_min - d_z, z_max + d_z)

    return x_lim, y_lim, z_lim
