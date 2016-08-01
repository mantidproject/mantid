from interface import InstrumentInterface

from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement, TOFTOFReductionScripter

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

class TOFTOFWidget(BaseWidget):
    ''' The one and only tab page. '''
    name = 'TOFTOF Reduction'

    class DataRunModel(QAbstractTableModel):
        ''' The list of data runs and correspnding comments. '''

        headers = ('Data runs', 'Comment')

        selectCell = pyqtSignal(int, int)

        def __init__(self, parent, scriptElement):
            QAbstractTableModel.__init__(self, parent)
            self.data = scriptElement

        def rowCount(self, index = QModelIndex()):
            return self.data.numDataRunsRows() + 1  # one additional row for new data

        def columnCount(self, index = QModelIndex()):
            return 2

        def headerData(self, section, orientation, role):
            if Qt.Horizontal == orientation and Qt.DisplayRole == role:
                return self.headers[section]

            return None

        def data(self, index, role):
            if Qt.DisplayRole == role or Qt.EditRole == role:
                return self.data.getDataRun(index.row(), index.column())

            return None

        def setData(self, index, text, role):

            row = index.row()
            col = index.column()

            self.data.setDataRun(row, col, text)
            self.data.updateDataRuns()

            # signal the attached view
            self.reset()

            # move selection to the next column or row
            col = col + 1

            if col >= 2:
                row = row + 1
                col = 0

            row = min(row, self.rowCount() - 1)

            self.selectCell.emit(row, col)

            return True

        def flags(self, index):
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable

    def __init__(self, settings, scriptElement):
        BaseWidget.__init__(self, settings = settings)
        self.state = scriptElement

        box = QVBoxLayout()
        self._layout.addLayout(box)

        # data path
        box.addWidget(QLabel('Data search directory'))

        self.dataPath    = QLineEdit()
        self.dataPathBtn = QPushButton('Browse')

        hbox = QHBoxLayout()
        hbox.addWidget(self.dataPath)
        hbox.addWidget(self.dataPathBtn)

        box.addLayout(hbox)

        # data runs
        self.dataRunsView = QTableView(self)
        self.dataRunsView.horizontalHeader().setStretchLastSection(True)
        self.dataRunsView.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)

        box.addWidget(self.dataRunsView)

        self.runDataModel = TOFTOFWidget.DataRunModel(self, scriptElement)
        self.dataRunsView.setModel(self.runDataModel)

        self.dataPathBtn.clicked.connect(self._onDataPath)
        self.runDataModel.selectCell.connect(self._onSelectedCell)

    def get_state(self):
        return self.state

    def _onDataPath(self):
        dirname = self.base.dir_browse_dialog()
        if dirname:
            self.dataPath.setText(dirname)

    def _onSelectedCell(self, row, col):
        index = self.runDataModel.index(row, col)
        self.dataRunsView.setCurrentIndex(index)
        self.dataRunsView.setFocus()

class TOFTOFInterface(InstrumentInterface):
    ''' The interface for TOFTOF reduction. '''

    def __init__(self, name, settings):
        InstrumentInterface.__init__(self, name, settings)

        self.ERROR_REPORT_NAME   = '%s_error_report.xml' % name
        self.LAST_REDUCTION_NAME = '.mantid_last_%s_reduction.xml' % name

        self.scriptElement = TOFTOFScriptElement()

        self.scripter = TOFTOFReductionScripter(name, settings.facility_name, self.scriptElement)
        self.attach(TOFTOFWidget(settings, self.scriptElement))
