from reduction_gui.widgets.base_widget import BaseWidget

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

class TOFTOFSetupWidget(BaseWidget):
    ''' The one and only tab page. '''
    name = 'TOFTOF Reduction'

    class DataRunModel(QAbstractTableModel):
        ''' The list of data runs and correspnding comments. '''

        headers = ('Data runs', 'Comment')

        selectCell = pyqtSignal(int, int)

        def __init__(self, parent, dataRuns):
            QAbstractTableModel.__init__(self, parent)
            self.dataRuns = dataRuns

        def rowCount(self, index = QModelIndex()):
            return self._numDataRunsRows() + 1  # one additional row for new data

        def columnCount(self, index = QModelIndex()):
            return 2

        def headerData(self, section, orientation, role):
            if Qt.Horizontal == orientation and Qt.DisplayRole == role:
                return self.headers[section]

            return None

        def data(self, index, role):
            if Qt.DisplayRole == role or Qt.EditRole == role:
                return self._getDataRun(index.row(), index.column())

            return None

        def setData(self, index, text, role):

            row = index.row()
            col = index.column()

            self._setDataRun(row, col, text)
            self._updateDataRuns()

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

        def _numDataRunsRows(self):
            return len(self.dataRuns)

        def _getDataRunsRow(self, row):
            return self.dataRuns[row] if row < self._numDataRunsRows() else ('', '')

        def _isDataRunsRowEmpty(self, row):
            (t1, t2) = self._getDataRunsRow(row)
            return not t1.strip() and not t2.strip()

        def _ensureDataRunsRows(self, rows):
            while self._numDataRunsRows() < rows:
                self.dataRuns.append(('', ''))

        def _updateDataRuns(self):
            ''' Remove trailing empty rows. '''
            for row in reversed(range(self._numDataRunsRows())):
                if self._isDataRunsRowEmpty(row):
                    del self.dataRuns[row]
                else:
                    break

        def _setDataRun(self, row, col, text):
            self._ensureDataRunsRows(row + 1)
            (runText, comment) = self.dataRuns[row]

            text = text.strip()
            if 0 == col:
                runText = text
            else:
                comment = text
            self.dataRuns[row] = (runText, comment)

        def _getDataRun(self, row, col):
            return self._getDataRunsRow(row)[col]

    def __init__(self, settings, scriptElement):
        BaseWidget.__init__(self, settings = settings)

        # ui & data elements
        self.dataSearchDir          = QLineEdit()
        self.dataSearchDirBtn       = QPushButton('Browse')
        self.prefix                 = QLineEdit()

        self.chkSubtractECFromVan   = QCheckBox('Subtract empty can from vanadium')
        self.rbtCorrectTOFNone      = QRadioButton('none')
        self.rbtCorrectTOFVan       = QRadioButton('vanadium')
        self.rbtCorrectTOFSample    = QRadioButton('sample')

        self.vanRuns       = QLineEdit()
        self.vanCmts       = QLineEdit()

        self.ecRuns        = QLineEdit()
        self.ecCmts        = QLineEdit()
        self.ecFactor      = QDoubleSpinBox()

        self.ecFactor.setRange(0, 1)
        self.ecFactor.setDecimals(3)
        self.ecFactor.setSingleStep(0.01)

        self.dataRuns      = []

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

        grid.addWidget(QLabel('workspace prefix'),0, 0)
        grid.addWidget(self.prefix,               0, 1)
        grid.addWidget(QLabel('vanadium runs'),   1, 0)
        grid.addWidget(self.vanRuns,              1, 1)
        grid.addWidget(QLabel('comment'),         1, 2)
        grid.addWidget(self.vanCmts,              1, 3)

        grid.addWidget(QLabel('EC runs'),         2, 0)
        grid.addWidget(self.ecRuns,               2, 1)
        grid.addWidget(QLabel('comment'),         2, 2)
        grid.addWidget(self.ecCmts,               2, 3)
        grid.addWidget(QLabel('factor'),          2, 4)
        grid.addWidget(self.ecFactor,             2, 5)

        grid.addWidget(QLabel('rebin in energy'), 3, 0)
        grid.addWidget(self.rebinEnergy,          3, 1)
        grid.addWidget(QLabel('rebin in Q'),      4, 0)
        grid.addWidget(self.rebinQ,               4, 1)
        grid.addWidget(QLabel('mask detectors'),  5, 0)
        grid.addWidget(self.maskDetectors,        5, 1)

        box.addLayout(grid)

        # data runs
        self.dataRunsView = QTableView(self)
        self.dataRunsView.horizontalHeader().setStretchLastSection(True)
        self.dataRunsView.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)

        box.addWidget(self.dataRunsView)

        self.runDataModel = TOFTOFSetupWidget.DataRunModel(self, self.dataRuns)
        self.dataRunsView.setModel(self.runDataModel)

        self.dataSearchDirBtn.clicked.connect(self._onDataSearchDir)
        self.runDataModel.selectCell.connect(self._onSelectedCell)

        # make accessible by scriptElement
        self.state = scriptElement

    def get_state(self):
        el = self.state # scriptElement

        # set scriptElement data from ui

        el.dataSearchDir     = self.dataSearchDir.text().strip()
        el.subtractECFromVan = self.chkSubtractECFromVan.isChecked()
        el.correctTOF = 'v' if self.rbtCorrectTOFVan.isChecked()   else \
                        's' if self.rbtCorrectTOFSample.isChecked() else ''

        el.vanRuns           = self.vanRuns.text().strip()
        el.vanCmts           = self.vanCmts.text().strip()

        el.ecRuns            = self.ecRuns.text().strip()
        el.ecCmts            = self.ecCmts.text().strip()
        el.ecFactor          = self.ecFactor.value()

        el.dataRuns          = self.dataRuns

        el.rebinEnergy       = self.rebinEnergy.text().strip()
        el.rebinQ            = self.rebinQ.text().strip()
        el.maskDetectors     = self.maskDetectors.text().strip()

        return el

    def set_state(self, state):
        el = state # scriptElement

        # set ui from scriptElement

        self.prefix.setText(el.prefix)

        self.dataSearchDir.setText(el.dataSearchDir)
        self.chkSubtractECFromVan.setChecked(el.subtractECFromVan)

        if 'v' == el.correctTOF:
            self.rbtCorrectTOFVan.setChecked(True)
        elif 's' == el.correctTOF:
            self.rbtCorrectTOFSample.setChecked(True)
        else:
            self.rbtCorrectTOFNone.setChecked(True)

        self.vanRuns.setText(el.vanRuns)
        self.vanCmts.setText(el.vanCmts)

        self.ecRuns.setText(el.ecRuns)
        self.ecCmts.setText(el.ecCmts)
        self.ecFactor.setValue(el.ecFactor)

        self.dataRuns        = el.dataRuns

        self.rebinEnergy.setText(el.rebinEnergy)
        self.rebinQ.setText(el.rebinQ)
        self.maskDetectors.setText(el.maskDetectors)

    def _onDataSearchDir(self):
        dirname = self.dir_browse_dialog()
        if dirname:
            self.dataSearchDir.setText(dirname)

    def _onSelectedCell(self, row, col):
        index = self.runDataModel.index(row, col)
        self.dataRunsView.setCurrentIndex(index)
        self.dataRunsView.setFocus()
