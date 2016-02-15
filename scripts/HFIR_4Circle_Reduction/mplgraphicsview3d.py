import sys
import numpy as np

from PyQt4 import QtGui
from PyQt4.QtGui import QSizePolicy

from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
from matplotlib.figure import Figure
from mpl_toolkits.mplot3d import Axes3D
# import matplotlib.image


class MplPlot3dCanvas(FigureCanvas):
    """
    Matplotlib 3D canvas class
    """
    def __init__(self):
        """
        Initialization
        :return:
        """
        #
        self.surfs = [] # [{"xx":,"yy:","val:"}]
        self.fig = Figure()
        self.fig.suptitle("this is  the  figure  title",  fontsize=12)
        FigureCanvas.__init__(self, self.fig)
        FigureCanvas.setSizePolicy(self, QSizePolicy.Expanding, QSizePolicy.Expanding)
        FigureCanvas.updateGeometry(self)
        self.ax = Axes3D(self.fig) # Canvas figure must be created for mouse rotation
        self.ax.set_xlabel('row (m CCD)')
        self.ax.set_ylabel('col (m CCD)')
        self.ax.set_zlabel('Phi (m)')
        self.format_coord_org = self.ax.format_coord
        self.ax.format_coord = self.report_pixel

        self.plots = list()
    
    def report_pixel(self, xd, yd):
        s = self.format_coord_org(xd, yd)
        s = s.replace(",", " ")
        return s
    
    def update_view(self, points):
        """

        :param points:
        :return:
        """
        # TODO/NOW: separate to method clear_3d_figures()
        for plt in self.plots:
            # del plt 
            self.ax.collections.remove(plt)
        self.plots = []

        # TODO/NOW: refactor to method plot_surface()
        print 'Number of surf = ', len(self.surfs)
        for surf in self.surfs:
            plt = self.ax.plot_surface(surf["xx"], surf["yy"], surf["val"], rstride=5, 
                    cstride=5, cmap=cm.jet, linewidth=1, antialiased=True)
            self.plots.append(plt)

        # Draw points
        # TODO/NOW: refactor to  method plot_scatter_points()

        """
        # prepare data
        points = np.zeros((2000, 3))
        X = np.array([.1, .0, .0])
        for i in range(points.shape[0]):
            points[i], X = X, lorenz_map(X)

        """
        assert isinstance(points, np.ndarray)
        assert isinstance(points.shape, tuple)
        assert points.shape[1] == 3, '3D data %s.' % str(points.shape)

        # Plotting
        self.ax.set_xlim(0.0, 10.0)
        self.ax.set_ylim(0.0, 10.0)
        self.ax.set_zlim(0.0, 10.0)

        x_min = min(points[:, 0])
        x_max = max(points[:, 0])
        print x_min, x_max
        y_min = min(points[:, 1])
        y_max = max(points[:, 1])
        print y_min, y_max
        z_min = min(points[:, 2])
        z_max = max(points[:, 2])
        print z_min, z_max

        self.ax.set_xlim(x_min-1, x_max+1)
        self.ax.set_ylim(y_min-1, y_max+1)
        self.ax.set_zlim(z_min-1, z_max+1)

        self.ax.set_xlabel('X axis')
        self.ax.set_ylabel('Y axis')
        self.ax.set_zlabel('Z axis')
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
        self.ax.scatter(points[:, 0], points[:, 1],  points[:, 2], zdir='z', c=color_list)

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


def parse3DFile(filename):
    """
    """
    ifile = open(filename, 'r')
    lines = ifile.readlines()
    ifile.close()

    points = np.zeros((len(lines), 3))
    for i in xrange(len(lines)):
        line = lines[i].strip()
        terms = line.split(',')
        for j in xrange(3):
            points[i][j] = float(terms[j])
    return points


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

    def update_view(self, data_points):
        self.canvas.update_view(data_points)


if __name__ == "__main__":
    """ main to launch """
    mainApp = QtGui.QApplication(sys.argv)

    myapp = MplPlot3DCanvasTester()
    myapp.show()

    # prepare data
    # points = generate_test_data()
    points = parse3DFile('exp355_scan38_pt11_qsample.dat')
    myapp.update_view(points)

    sys.exit(mainApp.exec_())

