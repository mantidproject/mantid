#pylint: disable = too-many-instance-attributes, too-many-branches, too-many-public-methods
#pylint: disable = W0622
"""
TOFTOF reduction workflow gui.
"""
from __future__ import (absolute_import, division, print_function)
from PyQt4.QtCore import *
from PyQt4.QtGui  import *

from reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement
from reduction_gui.widgets.data_table_view import DataTableView, DataTableModel

#-------------------------------------------------------------------------------

class SmallQLineEdit(QLineEdit):
    '''just a smaller QLineEdit'''  
    def sizeHint(self):
        '''overriding the sizeHint() function to get a smaller lineEdit'''
        sh = super(SmallQLineEdit, self).sizeHint()
        sh.setWidth(sh.width() // 2)
        sh.setHeight(sh.height() // 2)
        return sh


class TOFTOFSetupWidget(BaseWidget):
    ''' The one and only tab page. '''
    name = 'TOFTOF Reduction'

    class TofTofDataTableModel(DataTableModel):

        def _dataToText(self, row, col, value):
            """
            converts the stored data to a displayable text.
            Override this function if you need data types other than str in your table. 
            """
            if col == 2:
                return str(value) if value is not None else ''
            else:
                return str(value) # just return the value, it is already str.

        def _textToData(self, row, col, text):
            """
            converts a displayable text back to stored data.
            Override this function if you need data types other than str in your table. 
            """
            if col == 2:
                return float(text) if text else None
            else:
                return text # just return the value, it is already str.

    # tooltips
    TIP_prefix  = ''
    TIP_dataDir = ''
    TIP_saveDir = ''
    TIP_btnDataDir = ''
    TIP_btnSaveDir = ''

    TIP_vanRuns = ''
    TIP_vanCmnt = ''
    TIP_vanTemp = 'Temperature (K). Optional.'

    TIP_ecRuns = ''
    TIP_ecTemp = 'Temperature (K). Optional.'
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
    TIP_chkReplaceNaNs = 'Replace NaNs with 0'
    TIP_chkCreateDiff = ''
    TIP_chkKeepSteps = ''

    TIP_chkSofQW = ''
    TIP_chkSofTW = ''
    TIP_chkNxspe = 'Save for MSlice'
    TIP_chkNexus = 'Save for Mantid'
    TIP_chkAscii = 'Will be available soon'

    TIP_rbtNormaliseNone = ''
    TIP_rbtNormaliseMonitor = ''
    TIP_rbtNormaliseTime = ''

    TIP_rbtCorrectTOFNone = ''
    TIP_rbtCorrectTOFVan = ''
    TIP_rbtCorrectTOFSample = ''

    def dir_browse_dialog(self, default_dir=''):
        """
            Pop up a directory dialog box.
        """
        dirname = str(QFileDialog.getExistingDirectory(self, "Select Directory", default_dir, QFileDialog.DontUseNativeDialog))

        return dirname

    def __init__(self, settings):
        BaseWidget.__init__(self, settings = settings)

        inf = float('inf')

        def set_spin(spin, minVal = -inf, maxVal = +inf, decimals = 3):
            spin.setRange(minVal, maxVal)
            spin.setDecimals(decimals)
            spin.setSingleStep(0.01)

        def tip(widget, text):
            if text:
                widget.setToolTip(text)
            return widget

        # ui data elements
        self.prefix    = tip(QLineEdit(), self.TIP_prefix)
        self.dataDir   = tip(QLineEdit(), self.TIP_dataDir)
        self.saveDir   = tip(QLineEdit(), self.TIP_saveDir)

        self.vanRuns   = tip(QLineEdit(), self.TIP_vanRuns)
        self.vanCmnt   = tip(QLineEdit(), self.TIP_vanCmnt)
        self.vanTemp   = tip(SmallQLineEdit(), self.TIP_vanTemp)

        self.ecRuns    = tip(SmallQLineEdit(), self.TIP_ecRuns)
        self.ecTemp    = tip(SmallQLineEdit(), self.TIP_ecTemp)
        self.ecFactor  = tip(QDoubleSpinBox(), self.TIP_ecFactor)

        set_spin(self.ecFactor, 0, 1)

        self.binEon    = tip(QCheckBox(),      self.TIP_binEon)
        self.binEstart = tip(QDoubleSpinBox(), self.TIP_binEstart)
        self.binEstep  = tip(QDoubleSpinBox(), self.TIP_binEstep)
        self.binEend   = tip(QDoubleSpinBox(), self.TIP_binEend)

        set_spin(self.binEstart)
        set_spin(self.binEstep, decimals = 4)
        set_spin(self.binEend)

        self.binQon    = tip(QCheckBox(),      self.TIP_binQon)
        self.binQstart = tip(QDoubleSpinBox(), self.TIP_binQstart)
        self.binQstep  = tip(QDoubleSpinBox(), self.TIP_binQstep)
        self.binQend   = tip(QDoubleSpinBox(), self.TIP_binQend)

        set_spin(self.binQstart)
        set_spin(self.binQstep)
        set_spin(self.binQend)

        self.maskDetectors = tip(QLineEdit(), self.TIP_maskDetectors)

        headers = ('Data runs', 'Comment', 'Temperature [K]')
        self.dataRunsView = tip(DataTableView(self, headers, TOFTOFSetupWidget.TofTofDataTableModel), self.TIP_dataRunsView)
        self.dataRunsView.horizontalHeader().setStretchLastSection(True)
        self.dataRunsView.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.runDataModel = self.dataRunsView.model()

        # ui controls
        self.btnDataDir          = tip(QPushButton('Browse'), self.TIP_btnDataDir)
        self.btnSaveDir          = tip(QPushButton('Browse'), self.TIP_btnSaveDir)

        self.chkSubtractECVan    = tip(QCheckBox('Subtract empty can from vanadium'), self.TIP_chkSubtractECVan)
        self.chkReplaceNaNs      = tip(QCheckBox('Replace special values in S(Q,W) with 0'), self.TIP_chkReplaceNaNs)
        self.chkCreateDiff       = tip(QCheckBox('Create diffractograms'), self.TIP_chkCreateDiff)
        self.chkKeepSteps        = tip(QCheckBox('Keep intermediate steps'), self.TIP_chkKeepSteps)

        self.chkSofQW            = tip(QCheckBox('S(Q,W)'), self.TIP_chkSofQW)
        self.chkSofTW            = tip(QCheckBox('S(2theta,W)'), self.TIP_chkSofTW)
        self.chkNxspe            = tip(QCheckBox('NXSPE'), self.TIP_chkNxspe)
        self.chkNexus            = tip(QCheckBox('NeXus'), self.TIP_chkNexus)
        self.chkAscii            = tip(QCheckBox('Ascii'), self.TIP_chkAscii)

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

        def hbox(*widgets):
            return _box(QHBoxLayout, widgets)

        def vbox(*widgets):
            return _box(QVBoxLayout, widgets)

        def label(text, tip):
            label = QLabel(text)
            if tip:
                label.setToolTip(tip)
            return label

        gbDataDir = QGroupBox('Data search directory')
        gbPrefix  = QGroupBox('Workspace prefix')
        gbOptions = QGroupBox('Options')
        gbSave    = QGroupBox('Save reduced data')
        gbInputs  = QGroupBox('Inputs')
        gbBinning = QGroupBox('Binning')
        gbData    = QGroupBox('Data')

        box = QVBoxLayout()
        self._layout.addLayout(box)

        box.addLayout(hbox(vbox(gbDataDir, gbInputs, gbBinning, gbOptions, 1), vbox(gbPrefix, gbData, gbSave)))

        gbDataDir.setLayout(hbox(self.dataDir, self.btnDataDir))
        gbPrefix.setLayout(hbox(self.prefix,))

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
        grid.addWidget(self.chkReplaceNaNs,   3, 0, 1, 4)
        grid.addWidget(self.chkCreateDiff,    4, 0, 1, 4)
        grid.addWidget(self.chkKeepSteps,     5, 0, 1, 4)
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
        grid.addWidget(self.vanCmnt,            1, 1, 1, 2)
        grid.addLayout(hbox(QLabel('Temp.'), self.vanTemp),         1, 3)
        grid.addWidget(QLabel('Empty can runs'),2, 0)
        grid.addWidget(self.ecRuns,             2, 1, 1, 1)
        grid.addLayout(hbox(QLabel('EC factor'), self.ecFactor), 2, 2, 1, 1)
        grid.addLayout(hbox(QLabel('Temp.'), self.ecTemp),         2, 3)
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

        gbData.setLayout(hbox(self.dataRunsView))

        grid = QGridLayout()
        grid.addWidget(QLabel('Workspaces'),  0, 0)
        grid.addWidget(self.chkSofQW,         1, 0)
        grid.addWidget(self.chkSofTW,         1, 1)
        grid.addWidget(QLabel('Format'),      2, 0)
        grid.addWidget(self.chkNxspe,         3, 0)
        grid.addWidget(self.chkNexus,         3, 1)
        grid.addWidget(self.chkAscii,         3, 2)
        grid.setColumnStretch(3, 1)

        # disable save Ascii, it is not available for the moment
        self.chkAscii.setEnabled(False)

        gbSave.setLayout(vbox(label('Directory',''), hbox(self.saveDir, self.btnSaveDir), grid))

        # handle signals
        self.btnDataDir.clicked.connect(self._onDataDir)
        self.btnSaveDir.clicked.connect(self._onSaveDir)
        self.binEon.clicked.connect(self._onBinEon)
        self.binQon.clicked.connect(self._onBinQon)
        self.runDataModel.selectCell.connect(self._onSelectedCell)

    def _onDataDir(self):
        dirname = self.dir_browse_dialog(self.dataDir.text())
        if dirname:
            self.dataDir.setText(dirname)

    def _onSaveDir(self):
        dirname = self.dir_browse_dialog(self.saveDir.text())
        if dirname:
            self.saveDir.setText(dirname)

    def _onBinEon(self, onVal):
        if not onVal:
            self.chkNxspe.setChecked(False)
            self.chkReplaceNaNs.setChecked(False)
            self.binQon.setChecked(False)
        for widget in (self.binEstart, self.binEstep, self.binEend, self.chkCreateDiff, self.chkNxspe, self.binQon,
                       self.binQstart, self.binQstep, self.binQend, self.chkReplaceNaNs, self.chkSofQW):
            widget.setEnabled(onVal)

    def _onBinQon(self, onVal):
        for widget in (self.binQstart, self.binQstep, self.binQend, self.chkReplaceNaNs, self.chkSofQW):
            widget.setEnabled(onVal)

    def _onSelectedCell(self, index):
        self.dataRunsView.setCurrentIndex(index)
        self.dataRunsView.setFocus()

    def get_state(self):
        elem = TOFTOFScriptElement()

        def line_text(lineEdit):
            return lineEdit.text().strip()

        def float_or_none(string):
            return float(string) if string else None

        elem.facility_name   = self._settings.facility_name
        elem.instrument_name = self._settings.instrument_name

        elem.prefix        = line_text(self.prefix)
        elem.dataDir       = line_text(self.dataDir)

        elem.vanRuns       = line_text(self.vanRuns)
        elem.vanCmnt       = line_text(self.vanCmnt)
        elem.vanTemp       = float_or_none(line_text(self.vanTemp))

        elem.ecRuns        = line_text(self.ecRuns)
        elem.ecTemp        = float_or_none(line_text(self.ecTemp))
        elem.ecFactor      = self.ecFactor.value()

        elem.dataRuns      = self.runDataModel.tableData

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
        elem.replaceNaNs   = self.chkReplaceNaNs.isChecked()
        elem.createDiff    = self.chkCreateDiff.isChecked()
        elem.keepSteps     = self.chkKeepSteps.isChecked()

        elem.saveDir       = line_text(self.saveDir)
        elem.saveSofQW     = self.chkSofQW.isChecked()
        elem.saveSofTW     = self.chkSofTW.isChecked()
        elem.saveNXSPE     = self.chkNxspe.isChecked()
        elem.saveNexus     = self.chkNexus.isChecked()
        elem.saveAscii     = self.chkAscii.isChecked()

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
        self.vanTemp.setText(str(elem.vanTemp) if elem.vanTemp is not None else '')

        self.ecRuns.setText(elem.ecRuns)
        self.ecTemp.setText(str(elem.ecTemp) if elem.ecTemp is not None else '')
        self.ecFactor.setValue(elem.ecFactor)

        self.runDataModel.tableData = elem.dataRuns
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
        self.chkReplaceNaNs.setChecked(elem.replaceNaNs)
        self.chkCreateDiff.setChecked(elem.createDiff)
        self.chkKeepSteps.setChecked(elem.keepSteps)

        self.saveDir.setText(elem.saveDir)
        self.chkSofQW.setChecked(elem.saveSofQW)
        self.chkSofTW.setChecked(elem.saveSofTW)
        self.chkNxspe.setChecked(elem.saveNXSPE)
        self.chkNexus.setChecked(elem.saveNexus)
        self.chkAscii.setChecked(elem.saveAscii)

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
