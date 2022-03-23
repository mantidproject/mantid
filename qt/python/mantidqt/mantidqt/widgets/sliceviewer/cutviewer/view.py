from qtpy.QtWidgets import QVBoxLayout, QWidget, QTableWidget, QHeaderView, QTableWidgetItem
from qtpy.QtCore import Qt
from qtpy.QtGui import QColor
from mantid.plots.plotfunctions import create_subplots
from mantidqt.MPLwidgets import FigureCanvas
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar
import numpy as np


class CutViewerView(QWidget):
    """Displays a table view of the PeaksWorkspace along with controls
    to interact with the peaks.
    """
    def __init__(self, painter, sliceinfo_provider, parent=None):
        """
        :param painter: An object responsible for drawing the representation of the cut
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        super().__init__(parent)
        self.layout = None
        self.table = None
        self.figure = None
        self.figure_layout = None
        self._painter = painter
        self._sliceinfo_provider = sliceinfo_provider
        self._setup_ui()
        self.init_slice_table()
        self.hide()

    # private api
    def _setup_ui(self):
        """
        Arrange the widgets on the window
        """
        self.layout = QVBoxLayout()
        self.layout.sizeHint()
        self.layout.setContentsMargins(5, 0, 0, 0)
        self.layout.setSpacing(0)

        self._setup_table_widget()
        self._setup_figure_widget()
        self.setLayout(self.layout)

    def _setup_table_widget(self):
        """
        Make a table showing
        :return: A QTableWidget object which will contain plot widgets
        """
        table_widget = QTableWidget(3, 7, self)
        table_widget.setVerticalHeaderLabels(['u1', 'u2', 'u3'])
        table_widget.setHorizontalHeaderLabels(['a*', 'b*', 'c*', 'start', 'stop', 'nbins', 'step'])
        table_widget.setFixedHeight(table_widget.verticalHeader().defaultSectionSize()*(table_widget.rowCount()+1))  # +1 to include headers
        for icol in range(table_widget.columnCount()):
            table_widget.setColumnWidth(icol, 50)
        table_widget.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        table_widget.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        self.table = table_widget
        self.layout.addWidget(self.table)

    def _setup_figure_widget(self):
        fig, axes_matrix, _, _ = create_subplots(1)
        self.figure = fig
        self.figure.canvas = FigureCanvas(self.figure)
        toolbar = MantidNavigationToolbar(self.figure.canvas, self)
        self.figure_layout = QVBoxLayout()
        self.figure_layout.addWidget(toolbar)
        self.figure_layout.addWidget(self.figure.canvas)
        self.layout.addLayout(self.figure_layout)

    def init_slice_table(self):
        for icol in range(self.table.columnCount()):
            for irow in range(self.table.rowCount()):
                item = QTableWidgetItem()
                if icol == 5:
                    item.setData(Qt.EditRole, int(1))
                else:
                    item.setData(Qt.EditRole, float(1))
                if irow == self.table.rowCount()-1:
                    item.setFlags(item.flags() ^ Qt.ItemIsEditable)  # disable editing in last row (out of plane dim)
                    item.setBackground(QColor(250, 250, 250))
                else:
                    item.setFlags(item.flags() | Qt.ItemIsEditable)
                self.table.setItem(irow, icol, item)
        self.fill_new_table()

    def fill_new_table(self):
        # write proj matrix to table
        proj_matrix = self._sliceinfo_provider.get_proj_matrix().T  # .T so now each basis vector is a row as in table
        dims = self._sliceinfo_provider.get_dimensions()
        states = dims.get_states()
        states[states.index(None)] = 2  # set last index as out of plane dimension
        for idim in range(proj_matrix.shape[0]):
            for icol in range(proj_matrix.shape[0]):
                self.table.item(states[idim], icol).setData(Qt.EditRole, float(proj_matrix[idim, icol]))
        # write bin params for cut along horizontal axis (default)
        bin_params = np.array(dims.get_bin_params())[states]  # nbins except last element which is integration width
        lims = self._sliceinfo_provider.get_axes_limits() # (xlim, ylim, None)
        for irow in range(self.table.rowCount()-1):
            start, stop = lims[irow]
            if irow == 0:
                nbins = bin_params[irow]
            else:
                nbins = 1
                step = (stop - start) / bin_params[1]
                cen = (stop + start) / 2
                start, stop = cen-step, cen+step
            self.table.item(irow, 5).setData(Qt.EditRole, int(nbins))  # nbins
            self.table.item(irow, 3).setData(Qt.EditRole, float(start))  # start
            self.table.item(irow, 4).setData(Qt.EditRole, float(stop))  # stop
            self.table.item(irow, 6).setData(Qt.EditRole, float((stop - start) / nbins))  # step/width
        # update slicepoint and width
        slicept = dims.get_slicepoint()[states[-1]]
        self.table.item(2, 3).setData(Qt.EditRole, float(slicept - bin_params[-1]/2))  # start
        self.table.item(2, 4).setData(Qt.EditRole, float(slicept + bin_params[-1]/2))  # stop
        self.table.item(2, 6).setData(Qt.EditRole, float(bin_params[-1]))  # step (i.e. width)
