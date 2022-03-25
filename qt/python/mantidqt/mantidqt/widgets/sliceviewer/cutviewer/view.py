from qtpy.QtWidgets import QVBoxLayout, QWidget, QTableWidget, QHeaderView, QTableWidgetItem
from qtpy.QtCore import Qt
from qtpy.QtGui import QColor
from mantid.plots.plotfunctions import create_subplots
from mantidqt.MPLwidgets import FigureCanvas
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar
import numpy as np
from mantid.simpleapi import AnalysisDataService as ADS


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
        self._init_slice_table()
        self.table.cellChanged.connect(self.on_cell_changed)
        self.hide()  # hide initially as visibility is toggled

    def reset_table_data(self):
        # write proj matrix to table
        proj_matrix = self._sliceinfo_provider.get_proj_matrix().T  # .T so now each basis vector is a row as in table
        dims = self._sliceinfo_provider.get_dimensions()
        states = dims.get_states()
        states[states.index(None)] = 2  # set last index as out of plane dimension
        for idim in range(proj_matrix.shape[0]):
            self._write_vector_to_table(states[idim], proj_matrix[idim, :])
        # write bin params for cut along horizontal axis (default)
        bin_params = np.array(dims.get_bin_params())[states]  # nbins except last element which is integration width
        lims = self._sliceinfo_provider.get_data_limits()  # (xlim, ylim, None)
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
        self.update_slicepoint()
        # update slicepoint and width
        slicept = dims.get_slicepoint()[states[-1]]
        self.table.item(2, 3).setData(Qt.EditRole, float(slicept - bin_params[-1]/2))  # start
        self.table.item(2, 4).setData(Qt.EditRole, float(slicept + bin_params[-1]/2))  # stop
        self.table.item(2, 6).setData(Qt.EditRole, float(bin_params[-1]))  # step (i.e. width)

    def update_slicepoint(self):
        dims = self._sliceinfo_provider.get_dimensions()
        states = dims.get_states()
        states[states.index(None)] = 2  # set last index as out of plane dimension
        bin_params = np.array(dims.get_bin_params())[states]
        slicept = self._sliceinfo_provider.get_sliceinfo().z_value
        self.table.item(2, 3).setData(Qt.EditRole, float(slicept - bin_params[-1]/2))  # start
        self.table.item(2, 4).setData(Qt.EditRole, float(slicept + bin_params[-1]/2))  # stop
        self.table.item(2, 6).setData(Qt.EditRole, float(bin_params[-1]))  # step (i.e. width)

    def plot_cut_ws(self, wsname):
        self.figure.axes[0].clear()
        self.figure.axes[0].errorbar(ADS.retrieve(wsname), wkspIndex=None, marker='o', capsize=2, color='k',
                                     markersize=3)
        self.figure.canvas.draw()
        self.figure.tight_layout()

    def read_bin_params_from_table(self):
        vectors = np.zeros((3, 3), dtype=float)
        extents = np.zeros((2, 3), dtype=float)
        nbins = np.zeros(3, dtype=int)
        for ivec in range(vectors.shape[0]):
            for icol in range(vectors.shape[0]):
                vectors[ivec, icol] = float(self.table.item(ivec, icol).text())
            extents[0, ivec] = float(self.table.item(ivec, 3).text())  # start
            extents[1, ivec] = float(self.table.item(ivec, 4).text())  # stop
            nbins[ivec] = int(self.table.item(ivec, 5).text())
        return vectors, extents, nbins

    def validate_table(self, irow, icol):
        vectors, extents, nbins = self.read_bin_params_from_table()
        self.table.blockSignals(True)
        ivec = int(not bool(irow))  # index of u1 or u2 - which ever not changed (3rd row not editable)
        if icol < 3:
            # choose a new vector in plane that is not a linear combination of two other
            vectors[ivec] = np.cross(vectors[irow], vectors[2])
            self._write_vector_to_table(ivec, vectors[ivec])
        elif icol == 3 or icol == 4:
            # extents changed - adjust width based on nbins
            self.table.item(irow, 6).setData(Qt.EditRole,
                                             float((extents[1, irow] - extents[0, irow]) / nbins[irow]))
        elif icol == 5:
            # nbins changed - adjust step
            if nbins[irow] < 1:
                nbins[irow] = 1
                self.table.item(irow, 5).setData(Qt.EditRole, int(nbins[irow]))  # nbins
            self.table.item(irow, 6).setData(Qt.EditRole, float((extents[1, irow] - extents[0, irow]) / nbins[irow]))
            if nbins[irow] == 1 and nbins[ivec] == 1:
                nbins[ivec] = 100
            elif nbins[irow] > 1 and nbins[ivec] > 1:
                nbins[ivec] = 1
            self.table.item(ivec, 5).setData(Qt.EditRole, int(nbins[ivec]))
            self.table.item(ivec, 6).setData(Qt.EditRole,
                                             float((extents[1, ivec] - extents[0, ivec]) / nbins[ivec]))
        elif icol == 6:
            # step changed - adjust nbins
            step = float(self.table.item(irow, 6).text())
            if step < 0:
                step = abs(step)
                self.table.item(irow, 6).setData(Qt.EditRole, step)
            nbins[irow] = (extents[1, irow] - extents[0, irow]) / step
            if nbins[irow] < 1:
                nbins[irow] = 1
                self.table.item(ivec, 6).setData(Qt.EditRole, float((extents[1, irow] - extents[0, irow])))
            if nbins[irow] % 1 > 0:
                extents[irow, 1] = extents[irow, 1] - (nbins[irow] % 1) * step  # so integer number of bins
                self.table.item(irow, 4).setData(Qt.EditRole, float(extents[irow, 1]))
            self.table.item(irow, 5).setData(Qt.EditRole, int(nbins[irow]))  # nbins
        self.table.blockSignals(False)

    def on_cell_changed(self, irow, icol):
        self.validate_table(irow, icol)
        vectors, extents, nbins = self.read_bin_params_from_table()
        self.send_bin_params(vectors, extents, nbins)

    def send_bin_params(self, vectors, extents, nbins):
        if (extents[-1, :] - extents[0, :] > 0).all() and np.sum(nbins > 1) == 1 \
                and not np.isclose(np.linalg.det(vectors), 0.0):
            self._sliceinfo_provider.perform_non_axis_aligned_cut(vectors, extents.flatten(order='F'), nbins)

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
        fig, _, _, _ = create_subplots(1)
        fig.axes[0].autoscale(enable=True, tight=False)
        self.figure = fig
        self.figure.canvas = FigureCanvas(self.figure)
        toolbar = MantidNavigationToolbar(self.figure.canvas, self)
        self.figure_layout = QVBoxLayout()
        self.figure_layout.addWidget(toolbar)
        self.figure_layout.addWidget(self.figure.canvas)
        self.layout.addLayout(self.figure_layout)

    def _init_slice_table(self):
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
        self.reset_table_data()

    def _write_vector_to_table(self, irow, vector):
        for icol in range(len(vector)):
            self.table.item(irow, icol).setData(Qt.EditRole, float(vector[icol]))
