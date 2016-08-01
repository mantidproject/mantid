from interface import InstrumentInterface

from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement, TOFTOFReductionScripter

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

import traceback

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

        # ui data elements
        self.dataSearchDir          = QLineEdit()
        self.dataSearchDirBtn       = QPushButton('Browse')
        self.chkSubtractECFromVan   = QCheckBox('Subtract empty can from vanadium')
        self.rbtCorrectTOFNone      = QRadioButton('none')
        self.rbtCorrectTOFVan       = QRadioButton('vanadium')
        self.rbtCorrectTOFSample    = QRadioButton('sample')

        self.vanRuns       = QLineEdit()
        self.vanCmnt       = QLineEdit()

        self.ecRuns        = QLineEdit()
        self.ecCmts        = QLineEdit()
        self.ecFactor      = QLineEdit()

        self.rebinEnergy   = QLineEdit()
        self.rebinQ        = QLineEdit()
        self.maskDetectors = QLineEdit()

        # ui layout
        box = QVBoxLayout()
        self._layout.addLayout(box)

        # data path
        box.addWidget(QLabel('Data search directory'))

        hbox = QHBoxLayout()
        hbox.addWidget(self.dataSearchDir)
        hbox.addWidget(self.dataSearchDirBtn)

        box.addLayout(hbox)

        hbox = QHBoxLayout()
        hbox.addWidget(self.chkSubtractECFromVan)
        hbox.addStretch(1)

        box.addLayout(hbox)

        hbox = QHBoxLayout()
        hbox.addWidget(QLabel('correct TOF'))
        hbox.addWidget(self.rbtCorrectTOFNone)
        hbox.addWidget(self.rbtCorrectTOFVan)
        hbox.addWidget(self.rbtCorrectTOFSample)
        hbox.addStretch(1)

        box.addLayout(hbox)

        # vanadium and EC
        grid = QGridLayout()
        grid.addWidget(QLabel('vanadium runs'),   0, 0)
        grid.addWidget(self.vanRuns,              0, 1)
        grid.addWidget(QLabel('comment'),         0, 2)
        grid.addWidget(self.vanCmnt,              0, 3)

        grid.addWidget(QLabel('EC runs'),         1, 0)
        grid.addWidget(self.ecRuns,               1, 1)
        grid.addWidget(QLabel('comment'),         1, 2)
        grid.addWidget(self.ecCmts,               1, 3)
        grid.addWidget(QLabel('factor'),          1, 4)
        grid.addWidget(self.ecFactor,             1, 5)

        grid.addWidget(QLabel('rebin in energy'), 2, 0)
        grid.addWidget(self.rebinEnergy,          2, 1)
        grid.addWidget(QLabel('rebin in Q'),      2, 2)
        grid.addWidget(self.rebinQ,               2, 3)
        grid.addWidget(QLabel('mask detectors'),  3, 0)
        grid.addWidget(self.maskDetectors,        3, 1)

        box.addLayout(grid)

        # data runs
        self.dataRunsView = QTableView(self)
        self.dataRunsView.horizontalHeader().setStretchLastSection(True)
        self.dataRunsView.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)

        box.addWidget(self.dataRunsView)

        self.runDataModel = TOFTOFWidget.DataRunModel(self, scriptElement)
        self.dataRunsView.setModel(self.runDataModel)

        self.dataSearchDirBtn.clicked.connect(self._onDataSearchDir)
        self.runDataModel.selectCell.connect(self._onSelectedCell)

        # make accessible by scriptElement
        self.state = scriptElement

    def get_state(self):
        el = self.state # scriptElement
        el.dataSearchDir = self.dataSearchDir.text()

        return el

    def set_state(self, state):
        el = state # scriptElement, same as state
        print 'SET_STATE', el.dataSearchDir
        self.dataSearchDir.setText(el.dataSearchDir)

    def _onDataSearchDir(self):
        dirname = self.base.dir_browse_dialog()
        if dirname:
            self.dataSearchDir.setText(dirname)

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
