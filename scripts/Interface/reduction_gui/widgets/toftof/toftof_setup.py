from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------

class TOFTOFSetupWidget(BaseWidget):
    ''' The one and only tab page. '''
    name = 'TOFTOF Reduction'

    class DataRunModel(QAbstractTableModel):
        ''' The list of data runs and correspnding comments. '''

        def __init__(self, parent):
            QAbstractTableModel.__init__(self, parent)
            self.dataRuns = [] # [(runs, comment), ...]

        def _numRows(self):
            return len(self.dataRuns)

        def _getRow(self, row):
            return self.dataRuns[row] if row < self._numRows() else ('', '')

        def _isRowEmpty(self, row):
            (t1, t2) = self._getRow(row)
            return not t1.strip() and not t2.strip()

        def _removeTrailingEmptyRows(self):
            for row in reversed(range(self._numRows())):
                if self._isRowEmpty(row):
                    del self.dataRuns[row]
                else:
                    break

        def _ensureHasRows(self, numRows):
            while self._numRows() < numRows:
                self.dataRuns.append(('', ''))

        def _setCellText(self, row, col, text):
            self._ensureHasRows(row + 1)
            (runText, comment) = self.dataRuns[row]

            text = text.strip()
            if 0 == col:
                runText = text
            else:
                comment = text

            self.dataRuns[row] = (runText, comment)

        def _getCellText(self, row, col):
            return self._getRow(row)[col].strip()

        # reimplemented QAbstractTableModel methods

        headers    = ('Data runs', 'Comment')
        selectCell = pyqtSignal(int, int)

        def rowCount(self, index = QModelIndex()):
            # one additional row for new data
            return self._numRows() + 1

        def columnCount(self, index = QModelIndex()):
            return 2

        def headerData(self, section, orientation, role):
            if Qt.Horizontal == orientation and Qt.DisplayRole == role:
                return self.headers[section]

            return None

        def data(self, index, role):
            if Qt.DisplayRole == role or Qt.EditRole == role:
                return self._getCellText(index.row(), index.column())

            return None

        def setData(self, index, text, role):

            row = index.row()
            col = index.column()

            self._setCellText(row, col, text)
            self._removeTrailingEmptyRows()

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

    def __init__(self, settings):
        BaseWidget.__init__(self, settings = settings)

        # ui data elements
        self.prefix        = QLineEdit()

        self.dataDir       = QLineEdit()

        self.vanRuns       = QLineEdit()
        self.vanCmnt       = QLineEdit()

        self.ecRuns        = QLineEdit()
        self.ecFactor      = QDoubleSpinBox()

        self.ecFactor.setRange(0, 1)
        self.ecFactor.setDecimals(3)
        self.ecFactor.setSingleStep(0.01)

        self.rebinEnergy   = QLineEdit()
        self.rebinQ        = QLineEdit()
        self.maskDetectors = QLineEdit()

        self.dataRunsView = QTableView(self)
        self.dataRunsView.horizontalHeader().setStretchLastSection(True)
        self.dataRunsView.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)

        self.runDataModel = TOFTOFSetupWidget.DataRunModel(self)
        self.dataRunsView.setModel(self.runDataModel)

        # ui controls
        self.btnDataDir          = QPushButton('Browse')

        self.chkSubtractECVan    = QCheckBox('Subtract empty can from vanadium')

        self.rbtNormaliseNone    = QRadioButton('none')
        self.rbtNormaliseMonitor = QRadioButton('to monitor')
        self.rbtNormaliseTime    = QRadioButton('to time')

        self.rbtCorrectTOFNone   = QRadioButton('none')
        self.rbtCorrectTOFVan    = QRadioButton('vanadium')
        self.rbtCorrectTOFSample = QRadioButton('sample')

        # ui layout
        def _box(cls, widgets):
            box = cls()
            for w in widgets:
                if isinstance(w, QLayout):
                    box.addLayout(w)
                elif isinstance(w, QWidget):
                    box.addWidget(w)
                else:
                    box.addStretch(w)
            return box

        def hbox(widgets):
            return _box(QHBoxLayout, widgets)

        def vbox(widgets):
            return _box(QVBoxLayout, widgets)

        def label(text, tip):
            l = QLabel(text)
            if tip:
                l.setToolTip(tip)
            return l

        gbDataDir = QGroupBox('Data search directory')
        gbPrefix  = QGroupBox('Workspace prefix')
        gbOptions = QGroupBox('Options')
        gbInputs  = QGroupBox('Inputs')
        gbBinning = QGroupBox('Binning')
        gbData    = QGroupBox('Data')

        box = QVBoxLayout()
        self._layout.addLayout(box)

        box.addLayout(hbox((gbDataDir, gbPrefix)))
        box.addLayout(hbox((vbox((gbInputs, gbBinning, gbOptions, 1)), gbData)))

        gbDataDir.setLayout(hbox((self.dataDir, self.btnDataDir)))
        gbPrefix.setLayout(hbox((self.prefix,)))

        grid = QGridLayout()
        grid.addWidget(self.chkSubtractECVan,   0, 0, 1, 4)
        grid.addWidget(label('Normalise','tip'),     1, 0)
        grid.addWidget(self.rbtNormaliseNone,   1, 1)
        grid.addWidget(self.rbtNormaliseMonitor,1, 2)
        grid.addWidget(self.rbtNormaliseTime,   1, 3)
        grid.addWidget(QLabel('Correct TOF'),   2, 0)
        grid.addWidget(self.rbtCorrectTOFNone,  2, 1)
        grid.addWidget(self.rbtCorrectTOFVan,   2, 2)
        grid.addWidget(self.rbtCorrectTOFSample,2, 3)
        grid.setColumnStretch(4, 1)

        gbOptions.setLayout(grid)

        btnGroup = QButtonGroup(self)
        btnGroup.addButton(self.rbtNormaliseNone)
        btnGroup.addButton(self.rbtNormaliseMonitor)
        btnGroup.addButton(self.rbtNormaliseTime)

        btnGroup = QButtonGroup(self)
        btnGroup.addButton(self.rbtCorrectTOFNone)
        btnGroup.addButton(self.rbtCorrectTOFVan)
        btnGroup.addButton(self.rbtCorrectTOFSample)

        grid = QGridLayout()
        grid.addWidget(QLabel('Vanadium runs'), 0, 0)
        grid.addWidget(self.vanRuns,            0, 1)
        grid.addWidget(QLabel('Van. comment'),  1, 0)
        grid.addWidget(self.vanCmnt,            1, 1)
        grid.addWidget(QLabel('Empty can runs'),2, 0)
        grid.addWidget(self.ecRuns,             2, 1)
        grid.addWidget(QLabel('EC factor'),     3, 0)
        grid.addWidget(self.ecFactor,           3, 1)
        grid.addWidget(QLabel('mask detectors'),4, 0)
        grid.addWidget(self.maskDetectors,      4, 1)

        gbInputs.setLayout(grid)

        grid = QGridLayout()
        grid.addWidget(QLabel('Energy'),        0, 0)
        grid.addWidget(self.rebinEnergy,        0, 1)
        grid.addWidget(QLabel('Q'),             1, 0)
        grid.addWidget(self.rebinQ,             1, 1)
        gbBinning.setLayout(grid)

        gbData.setLayout(hbox((self.dataRunsView,)))





        # handle signals
        self.btnDataDir.clicked.connect(self._onDataSearchDir)
        self.runDataModel.selectCell.connect(self._onSelectedCell)

    def _onDataSearchDir(self):
        dirname = self.dir_browse_dialog()
        if dirname:
            self.dataDir.setText(dirname)

    def _onSelectedCell(self, row, col):
        index = self.runDataModel.index(row, col)
        self.dataRunsView.setCurrentIndex(index)
        self.dataRunsView.setFocus()

    def get_state(self):
        el = TOFTOFScriptElement()

        def getText(lineEdit):
            return lineEdit.text().strip()

        el.facility_name   = self._settings.facility_name
        el.instrument_name = self._settings.instrument_name

        el.prefix        = getText(self.prefix)
        el.dataDir       = getText(self.dataDir)

        el.vanRuns       = getText(self.vanRuns)
        el.vanCmnt       = getText(self.vanCmnt)

        el.ecRuns        = getText(self.ecRuns)
        el.ecFactor      = self.ecFactor.value()

        el.dataRuns      = self.runDataModel.dataRuns

        el.rebinEnergy   = getText(self.rebinEnergy)
        el.rebinQ        = getText(self.rebinQ)
        el.maskDetectors = getText(self.maskDetectors)

        el.subtractECVan = self.chkSubtractECVan.isChecked()

        el.normalise     = el.NORM_MONITOR    if self.rbtNormaliseMonitor.isChecked() else \
                           el.NORM_TIME       if self.rbtNormaliseTime.isChecked()    else \
                           el.NORM_NONE

        el.correctTof    = el.CORR_TOF_VAN    if self.rbtCorrectTOFVan.isChecked()    else \
                           el.CORR_TOF_SAMPLE if self.rbtCorrectTOFSample.isChecked() else \
                           el.CORR_TOF_NONE
        return el

    def set_state(self, toftofScriptElement):
        el = toftofScriptElement

        self.prefix.setText(el.prefix)

        self.dataDir.setText(el.dataDir)

        self.vanRuns.setText(el.vanRuns)
        self.vanCmnt.setText(el.vanCmnt)

        self.ecRuns.setText(el.ecRuns)
        self.ecFactor.setValue(el.ecFactor)

        self.runDataModel.dataRuns = el.dataRuns
        self.runDataModel.reset()

        self.rebinEnergy.setText(el.rebinEnergy)
        self.rebinQ.setText(el.rebinQ)
        self.maskDetectors.setText(el.maskDetectors)

        self.chkSubtractECVan.setChecked(el.subtractECVan)

        if el.normalise == el.NORM_MONITOR:
            self.rbtNormaliseMonitor.setChecked(True)
        elif el.normalise == el.NORM_TIME:
            self.rbtNormaliseTime.setChecked(True)
        else:
            self.rbtNormaliseNone.setChecked(True)

        if el.correctTof == el.CORR_TOF_VAN:
            self.rbtCorrectTOFVan.setChecked(True)
        elif el.correctTof == el.CORR_TOF_SAMPLE:
            self.rbtCorrectTOFSample.setChecked(True)
        else:
            self.rbtCorrectTOFNone.setChecked(True)

#-------------------------------------------------------------------------------
# eof
