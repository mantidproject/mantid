from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------

class TOFTOFSetupWidget(BaseWidget):
    ''' The one and only tab page. '''
    name = 'TOFTOF Reduction'

    class DataRunModel(QAbstractTableModel):
        ''' The list of data runs and corresponding comments. '''

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

    # tooltips
    TIP_prefix  = ''
    TIP_dataDir = ''
    TIP_btnDataDir = ''

    TIP_vanRuns = ''
    TIP_vanCmnt = ''

    TIP_ecRuns = ''
    TIP_ecFactor = ''

    TIP_binEon = ''
    TIP_binEstart = ''
    TIP_binEstep = ''
    TIP_binEend = ''

    TIP_binQon = ''
    TIP_binQstart = ''
    TIP_binQstep = ''
    TIP_binQend = ''

    TIP_maskDetectors = ''

    TIP_dataRunsView = ''

    TIP_chkSubtractECVan = ''

    TIP_rbtNormaliseNone = ''
    TIP_rbtNormaliseMonitor = ''
    TIP_rbtNormaliseTime = ''

    TIP_rbtCorrectTOFNone = ''
    TIP_rbtCorrectTOFVan = ''
    TIP_rbtCorrectTOFSample = ''

    def __init__(self, settings):
        BaseWidget.__init__(self, settings = settings)

        inf = float('inf')

        def setSpin(spin, min = -inf, max = +inf):
            spin.setRange(min, max)
            spin.setDecimals(3)
            spin.setSingleStep(0.01)

        def tip(widget, text):
          if text:
            widget.setToolTip(text)
          return widget

        # ui data elements
        self.prefix    = tip(QLineEdit(), self.TIP_prefix)
        self.dataDir   = tip(QLineEdit(), self.TIP_dataDir)

        self.vanRuns   = tip(QLineEdit(), self.TIP_vanRuns)
        self.vanCmnt   = tip(QLineEdit(), self.TIP_vanCmnt)

        self.ecRuns    = tip(QLineEdit(), self.TIP_ecRuns)
        self.ecFactor  = tip(QDoubleSpinBox(), self.TIP_ecFactor)

        setSpin(self.ecFactor, 0, 1)

        self.binEon    = tip(QCheckBox(),      self.TIP_binEon)
        self.binEstart = tip(QDoubleSpinBox(), self.TIP_binEstart)
        self.binEstep  = tip(QDoubleSpinBox(), self.TIP_binEstep)
        self.binEend   = tip(QDoubleSpinBox(), self.TIP_binEend)

        setSpin(self.binEstart)
        setSpin(self.binEstep)
        setSpin(self.binEend)

        self.binQon    = tip(QCheckBox(),      self.TIP_binQon)
        self.binQstart = tip(QDoubleSpinBox(), self.TIP_binQstart)
        self.binQstep  = tip(QDoubleSpinBox(), self.TIP_binQstep)
        self.binQend   = tip(QDoubleSpinBox(), self.TIP_binQend)

        setSpin(self.binQstart)
        setSpin(self.binQstep)
        setSpin(self.binQend)

        self.maskDetectors = tip(QLineEdit(), self.TIP_maskDetectors)

        self.dataRunsView  = tip(QTableView(self), self.TIP_dataRunsView)
        self.dataRunsView.horizontalHeader().setStretchLastSection(True)
        self.dataRunsView.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)

        self.runDataModel = TOFTOFSetupWidget.DataRunModel(self)
        self.dataRunsView.setModel(self.runDataModel)

        # ui controls
        self.btnDataDir          = tip(QPushButton('Browse'), self.TIP_btnDataDir)

        self.chkSubtractECVan    = tip(QCheckBox('Subtract empty can from vanadium'), self.TIP_chkSubtractECVan)

        self.rbtNormaliseNone    = tip(QRadioButton('none'), self.TIP_rbtNormaliseNone)
        self.rbtNormaliseMonitor = tip(QRadioButton('to monitor'), self.TIP_rbtNormaliseMonitor)
        self.rbtNormaliseTime    = tip(QRadioButton('to time'), self.TIP_rbtNormaliseTime)

        self.rbtCorrectTOFNone   = tip(QRadioButton('none'), self.TIP_rbtCorrectTOFNone)
        self.rbtCorrectTOFVan    = tip(QRadioButton('vanadium'), self.TIP_rbtCorrectTOFVan)
        self.rbtCorrectTOFSample = tip(QRadioButton('sample'), self.TIP_rbtCorrectTOFSample)

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
        grid.addWidget(label('Normalise','tip'),1, 0)
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
        grid.addWidget(self.vanRuns,            0, 1, 1, 3)
        grid.addWidget(QLabel('Van. comment'),  1, 0)
        grid.addWidget(self.vanCmnt,            1, 1, 1, 3)
        grid.addWidget(QLabel('Empty can runs'),2, 0)
        grid.addWidget(self.ecRuns,             2, 1)
        grid.addWidget(QLabel('EC factor'),     2, 2)
        grid.addWidget(self.ecFactor,           2, 3)
        grid.addWidget(QLabel('Mask detectors'),3, 0)
        grid.addWidget(self.maskDetectors,      3, 1, 1, 3)

        gbInputs.setLayout(grid)

        grid = QGridLayout()
        grid.addWidget(QLabel('on'),            0, 1)
        grid.addWidget(QLabel('start'),         0, 2)
        grid.addWidget(QLabel('step'),          0, 3)
        grid.addWidget(QLabel('end'),           0, 4)

        grid.addWidget(QLabel('Energy'),        1, 0)
        grid.addWidget(self.binEon,             1, 1)
        grid.addWidget(self.binEstart,          1, 2)
        grid.addWidget(self.binEstep,           1, 3)
        grid.addWidget(self.binEend,            1, 4)

        grid.addWidget(QLabel('Q'),             2, 0)
        grid.addWidget(self.binQon,             2, 1)
        grid.addWidget(self.binQstart,          2, 2)
        grid.addWidget(self.binQstep,           2, 3)
        grid.addWidget(self.binQend,            2, 4)

        for col in (0, 2, 3, 4):
            grid.setColumnStretch(col, 1)

        gbBinning.setLayout(grid)

        gbData.setLayout(hbox((self.dataRunsView,)))

        # handle signals
        self.btnDataDir.clicked.connect(self._onDataDir)
        self.binEon.clicked.connect(self._onBinEon)
        self.binQon.clicked.connect(self._onBinQon)
        self.runDataModel.selectCell.connect(self._onSelectedCell)

    def _onDataDir(self):
        dirname = self.dir_browse_dialog()
        if dirname:
            self.dataDir.setText(dirname)

    def _onBinEon(self, on):
        for widget in (self.binEstart, self.binEstep, self.binEend):
            widget.setEnabled(on)

    def _onBinQon(self, on):
        for widget in (self.binQstart, self.binQstep, self.binQend):
            widget.setEnabled(on)

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

        el.binEon        = self.binEon.isChecked()
        el.binEstart     = self.binEstart.value()
        el.binEstep      = self.binEstep.value()
        el.binEend       = self.binEend.value()

        el.binQon        = self.binQon.isChecked()
        el.binQstart     = self.binQstart.value()
        el.binQstep      = self.binQstep.value()
        el.binQend       = self.binQend.value()

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

        self.binEon.setChecked(el.binEon); self._onBinEon(el.binEon)
        self.binEstart.setValue(el.binEstart)
        self.binEstep.setValue(el.binEstep)
        self.binEend.setValue(el.binEend)

        self.binQon.setChecked(el.binQon); self._onBinQon(el.binQon)
        self.binQstart.setValue(el.binQstart)
        self.binQstep.setValue(el.binQstep)
        self.binQend.setValue(el.binQend)

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
