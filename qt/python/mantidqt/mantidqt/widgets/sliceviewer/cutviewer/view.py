# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from qtpy.QtWidgets import QVBoxLayout, QWidget, QTableWidget, QHeaderView, QTableWidgetItem
from qtpy.QtCore import Qt
from qtpy.QtGui import QColor
from mantid.plots.plotfunctions import create_subplots
from mantidqt.MPLwidgets import FigureCanvas
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar
import matplotlib.text as text
from mantid.simpleapi import AnalysisDataService as ADS
from mantid.kernel import SpecialCoordinateSystem
from numpy import zeros
from typing import Annotated, TypeAlias

# local imports
from .representation.cut_representation import CutRepresentation

# Forward declarations
CutViewPresenter: TypeAlias = Annotated[type, "CutViewPresenter"]
SliceViewerCanvas: TypeAlias = Annotated[type, "SliceViewerCanvas"]


class CutViewerView(QWidget):
    """Displays a table view of the PeaksWorkspace along with controls
    to interact with the peaks.
    """

    def __init__(self, canvas: SliceViewerCanvas, frame: SpecialCoordinateSystem):
        """
        :param painter: An object responsible for drawing the representation of the cut
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        super().__init__()
        self.presenter = None
        self.layout = None
        self.figure_layout = None
        self.table = None
        self.figure = None
        self.cut_rep = None
        self.canvas = canvas
        self.frame = frame
        self._setup_ui()
        self._init_slice_table()
        self.table.cellChanged.connect(self.on_cell_changed)

    def subscribe_presenter(self, presenter: CutViewPresenter):
        self.presenter = presenter

    def hide(self):
        super().hide()
        if self.cut_rep is not None:
            self.cut_rep.remove()
            self.cut_rep = None

    # signals

    def on_cell_changed(self, irow, icol):
        self.presenter.handle_cell_changed(irow, icol)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.figure.tight_layout()

    # getters

    def get_step(self, irow):
        return float(self.table.item(irow, 6).text())

    def get_bin_params(self):
        vectors = zeros((3, 3), dtype=float)
        extents = zeros((2, 3), dtype=float)
        nbins = zeros(3, dtype=int)
        for ivec in range(vectors.shape[0]):
            for icol in range(vectors.shape[0]):
                vectors[ivec, icol] = float(self.table.item(ivec, icol).text())
            extents[0, ivec] = float(self.table.item(ivec, 3).text())  # start
            extents[1, ivec] = float(self.table.item(ivec, 4).text())  # stop
            nbins[ivec] = int(self.table.item(ivec, 5).text())
        return vectors, extents, nbins

    # setters

    def set_vector(self, irow, vector):
        for icol in range(len(vector)):
            self.table.item(irow, icol).setData(Qt.EditRole, float(vector[icol]))

    def set_extent(self, irow, start=None, stop=None):
        if start is not None:
            self.table.item(irow, 3).setData(Qt.EditRole, float(start))
        if stop is not None:
            self.table.item(irow, 4).setData(Qt.EditRole, float(stop))

    def set_step(self, irow, step):
        self.table.item(irow, 6).setData(Qt.EditRole, float(step))

    def update_step(self, irow):
        _, extents, nbins = self.get_bin_params()
        self.set_step(irow, (extents[1, irow] - extents[0, irow]) / nbins[irow])

    def set_nbin(self, irow, nbin):
        self.table.item(irow, 5).setData(Qt.EditRole, int(nbin))
        self.update_step(irow)

    def set_bin_params(self, vectors, extents, nbins):
        self.table.blockSignals(True)
        for irow in range(len(nbins)):
            self.set_vector(irow, vectors[irow])
            self.set_extent(irow, *extents[:, irow])
            self.set_nbin(irow, nbins[irow])  # do this last as step automatically updated given extents
        self.table.blockSignals(False)
        self.plot_cut_representation()
        return vectors, extents, nbins

    def set_slicepoint(self, slicept, width):
        self.table.blockSignals(True)
        self.set_extent(2, slicept - width / 2, slicept + width / 2)
        self.set_step(2, width)
        self.table.blockSignals(False)

    # plotting

    def plot_cut_ws(self, wsname):
        if len(self.figure.axes[0].tracked_workspaces) == 0:
            self.figure.axes[0].errorbar(ADS.retrieve(wsname), wkspIndex=None, marker="o", capsize=2, color="k", markersize=3)
        self._format_cut_figure()
        self.figure.canvas.draw()

    def plot_cut_representation(self):
        if self.cut_rep is not None:
            self.cut_rep.remove()
        self.cut_rep = CutRepresentation(
            self.canvas, self.presenter.update_bin_params_from_cut_representation, *self.presenter.get_cut_representation_parameters()
        )

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
        table_widget.setVerticalHeaderLabels(["u1", "u2", "u3"])
        col_headers = ["a*", "b*", "c*"] if self.frame == SpecialCoordinateSystem.HKL else ["Qx", "Qy", "Qz"]
        col_headers.extend(["start", "stop", "nbins", "step"])
        table_widget.setHorizontalHeaderLabels(col_headers)
        table_widget.setFixedHeight(
            table_widget.verticalHeader().defaultSectionSize() * (table_widget.rowCount() + 1)
        )  # +1 to include headers
        for icol in range(table_widget.columnCount()):
            table_widget.setColumnWidth(icol, 50)
        table_widget.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        table_widget.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        self.table = table_widget
        self.layout.addWidget(self.table)

    def _setup_figure_widget(self):
        fig, _, _, _ = create_subplots(1)
        self.figure = fig
        self.figure.canvas = FigureCanvas(self.figure)
        toolbar = MantidNavigationToolbar(self.figure.canvas, self)
        self.figure_layout = QVBoxLayout()
        self.figure_layout.addWidget(toolbar)
        self.figure_layout.addWidget(self.figure.canvas)
        self.layout.addLayout(self.figure_layout)
        self.figure.canvas.mpl_connect("draw_event", self._on_figure_canvas_draw)
        self._is_drawing = False

    # When the figure redraws after e.g. the user changing an axis to log, then
    # for this view to see the update it requires an extra draw(). This method
    # is connected to the draw event, so we need to check we're only drawing once
    # and not causing a stack overflow.
    def _on_figure_canvas_draw(self, _):
        if self._is_drawing:
            self._is_drawing = False
            return
        self._is_drawing = True
        self.figure.canvas.draw()

    def _init_slice_table(self):
        for icol in range(self.table.columnCount()):
            for irow in range(self.table.rowCount()):
                item = QTableWidgetItem()
                if icol == 5:
                    item.setData(Qt.EditRole, int(1))
                else:
                    item.setData(Qt.EditRole, float(1))
                if irow == self.table.rowCount() - 1:
                    item.setFlags(item.flags() ^ Qt.ItemIsEditable)  # disable editing in last row (out of plane dim)
                    item.setBackground(QColor(250, 250, 250))
                else:
                    item.setFlags(item.flags() | Qt.ItemIsEditable)
                self.table.setItem(irow, icol, item)

    def _format_cut_figure(self):
        self.figure.axes[0].ignore_existing_data_limits = True
        self.figure.axes[0].autoscale(axis="both", tight=False)
        self._format_cut_xlabel()
        for textobj in self.figure.findobj(text.Text):
            textobj.set_fontsize(8)
        if not self.figure.get_figheight() == 0 and not self.figure.get_figwidth() == 0:
            # bug in tight_layout gives np.linalg error for singular matrix if above conditions not met
            # https://github.com/matplotlib/matplotlib/issues/9789  - says fixed for plt v2.1.0 (but seen on v3.1.2)
            self.figure.tight_layout()

    def _format_cut_xlabel(self):
        xlab = self.figure.axes[0].get_xlabel()
        istart = xlab.index("(")
        iend = xlab.index(")")
        xunit_str = xlab[iend + 1 :].replace("Ang^-1", "$\\AA^{-1}$)").replace("(", "").replace(")", "")
        xlab = xlab[0:istart] + xlab[istart : iend + 1].replace(" ", ", ") + xunit_str
        self.figure.axes[0].set_xlabel(xlab)
