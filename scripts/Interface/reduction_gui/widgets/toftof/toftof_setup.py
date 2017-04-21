#pylint: disable = too-many-instance-attributes, too-many-branches, too-many-public-methods
#pylint: disable = W0622
"""
TOFTOF reduction workflow gui.
"""
from PyQt4.QtCore import *
from PyQt4.QtGui  import *

from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement

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
            (runs, comment) = self._getRow(row)
            return not runs.strip() and not comment.strip()

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
            if col == 0:
                runText = text
            else:
                comment = text

            self.dataRuns[row] = (runText, comment)

        def _getCellText(self, row, col):
            return self._getRow(row)[col].strip()

        # reimplemented QAbstractTableModel methods

        headers    = ('Data runs', 'Comment')
        selectCell = pyqtSignal(int, int)

        def rowCount(self, _ = QModelIndex()):
            # one additional row for new data
            return self._numRows() + 1

        def columnCount(self, _ = QModelIndex()):
            return 2

        def headerData(self, section, orientation, role):
            if Qt.Horizontal == orientation and Qt.DisplayRole == role:
                return self.headers[section]

            return None

        def data(self, index, role):
            if Qt.DisplayRole == role or Qt.EditRole == role:
                return self._getCellText(index.row(), index.column())

            return None

        def setData(self, index, text, _):
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

        def flags(self, _):
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

        def set_spin(spin, minVal = -inf, maxVal = +inf):
            spin.setRange(minVal, maxVal)
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

        set_spin(self.ecFactor, 0, 1)

        self.binEon    = tip(QCheckBox(),      self.TIP_binEon)
        self.binEstart = tip(QDoubleSpinBox(), self.TIP_binEstart)
        self.binEstep  = tip(QDoubleSpinBox(), self.TIP_binEstep)
        self.binEend   = tip(QDoubleSpinBox(), self.TIP_binEend)

        set_spin(self.binEstart)
        set_spin(self.binEstep)
        set_spin(self.binEend)

        self.binQon    = tip(QCheckBox(),      self.TIP_binQon)
        self.binQstart = tip(QDoubleSpinBox(), self.TIP_binQstart)
        self.binQstep  = tip(QDoubleSpinBox(), self.TIP_binQstep)
        self.binQend   = tip(QDoubleSpinBox(), self.TIP_binQend)

        set_spin(self.binQstart)
        set_spin(self.binQstep)
        set_spin(self.binQend)

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
            for wgt in widgets:
                if isinstance(wgt, QLayout):
                    box.addLayout(wgt)
                elif isinstance(wgt, QWidget):
                    box.addWidget(wgt)
                else:
                    box.addStretch(wgt)
            return box

        def hbox(widgets):
            return _box(QHBoxLayout, widgets)

        def vbox(widgets):
            return _box(QVBoxLayout, widgets)

        def label(text, tip):
            label = QLabel(text)
            if tip:
                label.setToolTip(tip)
            return label

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

    def _onBinEon(self, onVal):
        for widget in (self.binEstart, self.binEstep, self.binEend):
            widget.setEnabled(onVal)

    def _onBinQon(self, onVal):
        for widget in (self.binQstart, self.binQstep, self.binQend):
            widget.setEnabled(onVal)

    def _onSelectedCell(self, row, col):
        index = self.runDataModel.index(row, col)
        self.dataRunsView.setCurrentIndex(index)
        self.dataRunsView.setFocus()

    def get_state(self):
        elem = TOFTOFScriptElement()

        def line_text(lineEdit):
            return lineEdit.text().strip()

        elem.facility_name   = self._settings.facility_name
        elem.instrument_name = self._settings.instrument_name

        elem.prefix        = line_text(self.prefix)
        elem.dataDir       = line_text(self.dataDir)

        elem.vanRuns       = line_text(self.vanRuns)
        elem.vanCmnt       = line_text(self.vanCmnt)

        elem.ecRuns        = line_text(self.ecRuns)
        elem.ecFactor      = self.ecFactor.value()

        elem.dataRuns      = self.runDataModel.dataRuns

        elem.binEon        = self.binEon.isChecked()
        elem.binEstart     = self.binEstart.value()
        elem.binEstep      = self.binEstep.value()
        elem.binEend       = self.binEend.value()

        elem.binQon        = self.binQon.isChecked()
        elem.binQstart     = self.binQstart.value()
        elem.binQstep      = self.binQstep.value()
        elem.binQend       = self.binQend.value()

        elem.maskDetectors = line_text(self.maskDetectors)

        elem.subtractECVan = self.chkSubtractECVan.isChecked()

        elem.normalise     = elem.NORM_MONITOR    if self.rbtNormaliseMonitor.isChecked() else \
            elem.NORM_TIME       if self.rbtNormaliseTime.isChecked()    else \
            elem.NORM_NONE

        elem.correctTof    = elem.CORR_TOF_VAN    if self.rbtCorrectTOFVan.isChecked()    else \
            elem.CORR_TOF_SAMPLE if self.rbtCorrectTOFSample.isChecked() else \
            elem.CORR_TOF_NONE
        return elem

    def set_state(self, toftofScriptElement):
        elem = toftofScriptElement

        self.prefix.setText(elem.prefix)

        self.dataDir.setText(elem.dataDir)

        self.vanRuns.setText(elem.vanRuns)
        self.vanCmnt.setText(elem.vanCmnt)

        self.ecRuns.setText(elem.ecRuns)
        self.ecFactor.setValue(elem.ecFactor)

        self.runDataModel.dataRuns = elem.dataRuns
        self.runDataModel.reset()

        self.binEon.setChecked(elem.binEon)
        self._onBinEon(elem.binEon)

        self.binEstart.setValue(elem.binEstart)
        self.binEstep.setValue(elem.binEstep)
        self.binEend.setValue(elem.binEend)

        self.binQon.setChecked(elem.binQon)
        self._onBinQon(elem.binQon)

        self.binQstart.setValue(elem.binQstart)
        self.binQstep.setValue(elem.binQstep)
        self.binQend.setValue(elem.binQend)

        self.maskDetectors.setText(elem.maskDetectors)

        self.chkSubtractECVan.setChecked(elem.subtractECVan)

        if elem.normalise == elem.NORM_MONITOR:
            self.rbtNormaliseMonitor.setChecked(True)
        elif elem.normalise == elem.NORM_TIME:
            self.rbtNormaliseTime.setChecked(True)
        else:
            self.rbtNormaliseNone.setChecked(True)

        if elem.correctTof == elem.CORR_TOF_VAN:
            self.rbtCorrectTOFVan.setChecked(True)
        elif elem.correctTof == elem.CORR_TOF_SAMPLE:
            self.rbtCorrectTOFSample.setChecked(True)
        else:
            self.rbtCorrectTOFNone.setChecked(True)

#-------------------------------------------------------------------------------
# eof
