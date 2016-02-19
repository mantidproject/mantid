import sys
import numpy as np
import os

from PyQt4 import QtGui
from PyQt4.QtGui import QSizePolicy

from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
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
        self._myAxes = Axes3D(self._myFigure) # Canvas figure must be created for mouse rotation
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
            self._myAxes.collections.remove(plt)
        self._currPlotList = []

    def import_3d_data(self, points, intensities):
        """

        :param points:
        :param intensities:
        :return:
        """
        # check
        assert isinstance(points, np.ndarray) and points.shape[1] == 3, 'Shape is %s.' % str(points.shape)
        assert isinstance(intensities, np.ndarray) and len(points) == len(intensities)

        # set
        self._dataDict[self._dataKey] = (points, intensities)

        # update
        r_value = self._dataKey
        self._dataKey += 1

        return r_value

    def import_data_from_file(self, file_name):
        """ File will have more than 4 columns, as X, Y, Z, Intensity, ...
        :param file_name:
        :return:
        """
        # check
        assert isinstance(file_name, str) and os.path.exists(file_name)

        # parse
        data_file = open(file_name, 'r')
        raw_lines = data_file.readlines()
        data_file.close()

        # construct ND data array
        xyz_points = np.zeros((len(raw_lines), 3))
        intensities = np.zeros((len(raw_lines), ))

        # parse
        for i in xrange(len(raw_lines)):
            line = raw_lines[i].strip()

            # skip empty line
            if len(line) == 0:
                continue

            # set value
            terms = line.split(',')
            for j in xrange(3):
                xyz_points[i][j] = float(terms[j])
            intensities[i] = float(terms[3])
        # END-FOR

        # Add to data structure for managing
        self._dataDict[self._dataKey] = (xyz_points, intensities)
        return_value = self._dataKey
        self._dataKey += 1

        return return_value

    def plot_scatter(self, data_key, base_color=None):
        """
        Plot data in scatter plot
        :param data_key:
        :return:
        """
        # TODO/Now - Doc and check input

        # get data and check
        points = self._dataDict[data_key][0]
        intensities = self._dataDict[data_key][1]

        assert isinstance(points, np.ndarray)
        assert isinstance(points.shape, tuple)
        assert points.shape[1] == 3, '3D data %s.' % str(points.shape)

        if len(points) > 1:
            # set x, y and z limit
            x_min = min(points[:, 0])
            x_max = max(points[:, 0])
            dx = x_max - x_min
            print x_min, x_max
            y_min = min(points[:, 1])
            y_max = max(points[:, 1])
            dy = y_max - y_min
            print y_min, y_max
            z_min = min(points[:, 2])
            z_max = max(points[:, 2])
            dz = z_max - z_min
            print z_min, z_max

            # use default setup
            self._myAxes.set_xlim(x_min-dx, x_max+dx)
            self._myAxes.set_ylim(y_min-dy, y_max+dy)
            self._myAxes.set_zlim(z_min-dz, z_max+dz)
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
            b_list = b_list/diff

            num_points = len(points[:, 2])
            for index in xrange(num_points):
                color_tup = (color_r, color_g, b_list[index])
                color_list.append(color_tup)
        else:
            color_list.append((color_r, color_g, 0.5))

        # plot scatters
        self._myAxes.scatter(points[:, 0], points[:, 1],  points[:, 2], zdir='z', c=color_list)

        self.draw()

    def plot_surface(self):
        """
        Plot surface
        :return:
        """
        print 'Number of surf = ', len(self._currSurfaceList)
        for surf in self._currSurfaceList:
            plt = self._myAxes.plot_surface(surf["xx"], surf["yy"], surf["val"],
                                            rstride=5, cstride=5,  # color map??? cmap=cm.jet,
                                            linewidth=1, antialiased=True)
            self._currPlotList.append(plt)
        # END-FOR

        return

    def report_pixel(self, xd, yd):
        s = self.format_coord_org(xd, yd)
        s = s.replace(",", " ")
        return s

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
        # TODO/NOW/ Doc and check
        assert isinstance(color_r, float), 0 <= color_r < 1.

        self._colorMap = [color_r, color_g, color_b]

    def set_title(self, title, font_size):
        """
        Set super title
        :param title:
        :return:
        """
        self._myFigure.suptitle(title,  fontsize=font_size)

        return

    def update_view(self, d_key):
        """

        :param points:
        :return:
        """


        # Draw points
        # TODO/NOW: refactor to  method plot_scatter_points()

        """
        # prepare data
        points = np.zeros((2000, 3))
        X = np.array([.1, .0, .0])
        for i in range(points.shape[0]):
            points[i], X = X, lorenz_map(X)

        """
        points = self._dataDict[d_key][0]
        intensities = self._dataDict[d_key][1]
        assert isinstance(points, np.ndarray)
        assert isinstance(points.shape, tuple)
        assert points.shape[1] == 3, '3D data %s.' % str(points.shape)

        # Plotting
        self._myAxes.set_xlim(0.88, 0.89)
        self._myAxes.set_ylim(-0.37, 0.35)
        self._myAxes.set_zlim(4.08, 4.09)

        x_min = min(points[:, 0])
        x_max = max(points[:, 0])
        print x_min, x_max
        y_min = min(points[:, 1])
        y_max = max(points[:, 1])
        print y_min, y_max
        z_min = min(points[:, 2])
        z_max = max(points[:, 2])
        print z_min, z_max

        """
        self._myAxes.set_xlim(x_min-1, x_max+1)
        self._myAxes.set_ylim(y_min-1, y_max+1)
        self._myAxes.set_zlim(z_min-1, z_max+1)
        """

        self._myAxes.set_xlabel('X axis')
        self._myAxes.set_ylabel('Y axis')
        self._myAxes.set_zlabel('Z axis')
        # ax.set_title('Lorenz Attractor a=%0.2f b=%0.2f c=%0.2f' % (a, b, c))
        # 

        # color map
        print '[DB] List size = ', len(points[:, 2])  # result = 579
        num_points = len(points[:, 2])
        color_list = list()
        for index in xrange(len(points[:, 2])):
            color_tup = (0.6, 0.3, float(index)/num_points)
            color_list.append(color_tup)

        # plot scatters
        self._myAxes.scatter(points[:, 0], points[:, 1],  points[:, 2], zdir='z', c=color_list)

        self.draw()


# Data set generation
def generate_test_data():
    """
    """
    a, b, c = 10., 28., 8. / 3.

    def lorenz_map(X, dt = 1e-2):
        X_dt = np.array([a * (X[1] - X[0]), X[0] * (b - X[2]) - X[1], X[0] * X[1] - c * X[2]])
        return X + dt * X_dt

    data_points = np.zeros((2000, 3))
    linear_x_set = np.array([.1, .0, .0])
    for i in range(data_points.shape[0]):
        data_points[i], linear_x_set = linear_x_set, lorenz_map(linear_x_set)

    return data_points


class MplPlot3DCanvasTester(QtGui.QWidget):
    def __init__(self, parent = None):
       super(MplPlot3DCanvasTester, self).__init__(parent)
       self.canvas = MplPlot3dCanvas()
       self.toolbar = NavigationToolbar(self.canvas, self.canvas)
       self.vbox = QtGui.QVBoxLayout()
       self.vbox.addWidget(self.canvas)
       self.vbox.addWidget(self.toolbar)
       self.setLayout(self.vbox)
       self.to_update = False

    def update_view(self, d_key):
        self.canvas.update_view(d_key)


if __name__ == "__main__":
    """ main to launch """
    mainApp = QtGui.QApplication(sys.argv)

    myapp = MplPlot3DCanvasTester()
    myapp.show()

    # prepare data
    # points = generate_test_data()

    # data_key1 = myapp.canvas.import_data_from_file('exp355_scan38_pt11_qsample.dat')
    # myapp.update_view(data_key1)

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

    data_key2 = myapp.canvas.import_3d_data(centers, intensities2)
    # myapp.update_view(data_key2)
    myapp.canvas.plot_scatter(data_key2)

    avg_center = np.ndarray((1, 3), 'double')
    avg_center[0][0] = 0.88735938499471179
    avg_center[0][1] = -0.36045625545762
    avg_center[0][2] = 4.0820727566625354
    intensities3 = np.ndarray((1,), 'double')
    intensities3[0] = 10000.
    data_key3 = myapp.canvas.import_3d_data(avg_center, intensities3)
    myapp.canvas.set_color_map(0.99, 0.1, 0.1)
    myapp.canvas.plot_scatter(data_key3)

    sys.exit(mainApp.exec_())

