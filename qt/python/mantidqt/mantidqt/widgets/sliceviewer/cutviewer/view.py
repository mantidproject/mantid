from qtpy.QtWidgets import QVBoxLayout, QWidget, QTableWidget, QHeaderView
from qtpy.QtCore import Qt
from mantid.plots.plotfunctions import create_subplots
from mantidqt.MPLwidgets import FigureCanvas
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar


class CutViewerView(QWidget):
    """Displays a table view of the PeaksWorkspace along with controls
    to interact with the peaks.
    """
    TITLE_PREFIX = "Workspace: "

    def __init__(self, painter, sliceinfo_provider, parent=None):
        """
        :param painter: An object responsible for drawing the representation of the cut
        :param sliceinfo_provider: An object responsible for providing access to current slice information
        :param parent: An optional parent widget
        """
        super().__init__(parent)
        self._painter = painter
        self._sliceinfo_provider = sliceinfo_provider
        self._setup_ui()
        self.layout = None
        self.table = None
        self.figure = None
        self.figure_layout = None
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
        self.setup_figure_widget()
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

    def setup_figure_widget(self):
        fig, axes_matrix, _, _ = create_subplots(1)
        self.figure = fig
        self.figure.canvas = FigureCanvas(self.figure)
        toolbar = MantidNavigationToolbar(self.figure.canvas, self)
        self.figure_layout = QVBoxLayout()
        self.figure_layout.addWidget(toolbar)
        self.figure_layout.addWidget(self.figure.canvas)
        self.layout.addLayout(self.figure_layout)
