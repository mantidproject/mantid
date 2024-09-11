# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# pylint: disable = too-many-instance-attributes, too-many-branches, too-many-public-methods
# pylint: disable = W0622
"""
TOFTOF reduction workflow gui.
"""

from qtpy.QtCore import Qt
from qtpy.QtGui import QDoubleValidator
from qtpy.QtWidgets import (
    QButtonGroup,
    QCheckBox,
    QDoubleSpinBox,
    QFileDialog,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLayout,
    QLineEdit,
    QPushButton,
    QRadioButton,
    QSizePolicy,
    QSpacerItem,
    QWidget,
    QVBoxLayout,
)
from mantidqtinterfaces.reduction_gui.widgets.base_widget import BaseWidget
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement, OptionalFloat
from mantidqtinterfaces.reduction_gui.widgets.data_table_view import DataTableView, DataTableModel

# -------------------------------------------------------------------------------


class SmallQLineEdit(QLineEdit):
    """just a smaller QLineEdit"""

    def sizeHint(self):
        """overriding the sizeHint() function to get a smaller lineEdit"""
        sh = super(SmallQLineEdit, self).sizeHint()
        sh.setWidth(sh.width() // 2)
        sh.setHeight(sh.height() // 2)
        return sh


class TOFTOFSetupWidget(BaseWidget):
    """The one and only tab page."""

    name = "TOFTOF Reduction"

    class TofTofDataTableModel(DataTableModel):
        def _textToData(self, row, col, text):
            """
            converts a displayable text back to stored data.
            """
            if col == 2:
                return OptionalFloat(text)
            else:
                return text  # just return the value, it is already str.

    # tooltips
    TIP_prefix = ""
    TIP_dataDir = ""
    TIP_saveDir = ""
    TIP_btnDataDir = ""
    TIP_btnSaveDir = ""

    TIP_vanRuns = ""
    TIP_vanCmnt = ""
    TIP_vanTemp = "Temperature (K). Optional."
    TIP_vanEcFactor = ""

    TIP_ecRuns = ""
    TIP_ecTemp = "Temperature (K). Optional."
    TIP_ecFactor = ""

    TIP_binEon = ""
    TIP_binEstart = ""
    TIP_binEstep = ""
    TIP_binEend = ""

    TIP_binQon = ""
    TIP_binQstart = ""
    TIP_binQstep = ""
    TIP_binQend = ""

    TIP_maskDetectors = ""

    TIP_dataRunsView = ""

    TIP_chkSubtractECVan = ""
    TIP_chkReplaceNaNs = "Replace NaNs with 0"
    TIP_chkCreateDiff = ""
    TIP_chkKeepSteps = ""

    TIP_chkSofQW = ""
    TIP_chkSofTW = ""
    TIP_chkNxspe = "Save for MSlice"
    TIP_chkNexus = "Save for Mantid"
    TIP_chkAscii = "Save as text"

    TIP_rbtNormaliseNone = ""
    TIP_rbtNormaliseMonitor = ""
    TIP_rbtNormaliseTime = ""

    TIP_rbtCorrectTOFNone = ""
    TIP_rbtCorrectTOFVan = ""
    TIP_rbtCorrectTOFSample = ""

    def dir_browse_dialog(self, default_dir=""):
        """
        Pop up a directory dialog box.
        """
        dirname = QFileDialog.getExistingDirectory(self, "Select Directory", default_dir, QFileDialog.DontUseNativeDialog)
        if isinstance(dirname, tuple):
            dirname = dirname[0]

        return dirname

    def __init__(self, settings):
        BaseWidget.__init__(self, settings=settings)

        inf = float("inf")

        def set_spin(spin, minVal=-inf, maxVal=+inf, decimals=3):
            spin.setRange(minVal, maxVal)
            spin.setDecimals(decimals)
            spin.setSingleStep(0.01)

        def tip(widget, text):
            if text:
                widget.setToolTip(text)
            return widget

        def setEnabled(widget, *widgets):
            """enables widget, when value of all widgets evaluates to true"""

            def setEnabled():
                widget.setEnabled(all(w.isChecked() for w in widgets))

            for w in widgets:
                w.toggled.connect(setEnabled)
            return widget

        def DoubleEdit():
            edit = SmallQLineEdit()
            edit.setValidator(QDoubleValidator())
            return edit

        # ui data elements
        self.prefix = tip(QLineEdit(), self.TIP_prefix)
        self.dataDir = tip(QLineEdit(), self.TIP_dataDir)
        self.saveDir = tip(QLineEdit(), self.TIP_saveDir)

        self.vanRuns = tip(QLineEdit(), self.TIP_vanRuns)
        self.vanCmnt = tip(QLineEdit(), self.TIP_vanCmnt)
        self.vanTemp = tip(DoubleEdit(), self.TIP_vanTemp)

        self.ecRuns = tip(SmallQLineEdit(), self.TIP_ecRuns)
        self.ecTemp = tip(DoubleEdit(), self.TIP_ecTemp)
        self.ecFactor = tip(QDoubleSpinBox(), self.TIP_ecFactor)

        set_spin(self.ecFactor, 0, 1)

        self.binEon = tip(QCheckBox(), self.TIP_binEon)
        self.binEstart = setEnabled(tip(QDoubleSpinBox(), self.TIP_binEstart), self.binEon)
        self.binEstep = setEnabled(tip(QDoubleSpinBox(), self.TIP_binEstep), self.binEon)
        self.binEend = setEnabled(tip(QDoubleSpinBox(), self.TIP_binEend), self.binEon)

        set_spin(self.binEstart)
        set_spin(self.binEstep, decimals=4)
        set_spin(self.binEend)

        self.binQon = setEnabled(tip(QCheckBox(), self.TIP_binQon), self.binEon)
        self.binQstart = setEnabled(tip(QDoubleSpinBox(), self.TIP_binQstart), self.binEon, self.binQon)
        self.binQstep = setEnabled(tip(QDoubleSpinBox(), self.TIP_binQstep), self.binEon, self.binQon)
        self.binQend = setEnabled(tip(QDoubleSpinBox(), self.TIP_binQend), self.binEon, self.binQon)

        set_spin(self.binQstart)
        set_spin(self.binQstep)
        set_spin(self.binQend)

        self.maskDetectors = tip(QLineEdit(), self.TIP_maskDetectors)

        headers = ("Data runs", "Comment", "T (K)")
        self.dataRunsView = tip(DataTableView(self, headers, TOFTOFSetupWidget.TofTofDataTableModel), self.TIP_dataRunsView)
        self.dataRunsView.horizontalHeader().setStretchLastSection(True)
        self.dataRunsView.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.runDataModel = self.dataRunsView.model()

        # ui controls
        self.btnDataDir = tip(QPushButton("Browse"), self.TIP_btnDataDir)
        self.btnSaveDir = tip(QPushButton("Browse"), self.TIP_btnSaveDir)

        self.chkSubtractECVan = tip(QCheckBox("Subtract empty can from vanadium"), self.TIP_chkSubtractECVan)
        self.vanEcFactor = setEnabled(tip(QDoubleSpinBox(), self.TIP_vanEcFactor), self.chkSubtractECVan)
        set_spin(self.vanEcFactor, 0, 1)
        self.chkReplaceNaNs = setEnabled(tip(QCheckBox("Replace special values in S(Q, ω) with 0"), self.TIP_chkReplaceNaNs), self.binEon)
        self.chkCreateDiff = setEnabled(tip(QCheckBox("Create diffractograms"), self.TIP_chkCreateDiff), self.binEon)
        self.chkKeepSteps = tip(QCheckBox("Keep intermediate steps"), self.TIP_chkKeepSteps)

        self.chkSofTWNxspe = setEnabled(tip(QCheckBox("NXSPE"), self.TIP_chkNxspe), self.binEon)
        self.chkSofTWNexus = tip(QCheckBox("NeXus"), self.TIP_chkNexus)
        self.chkSofTWAscii = tip(QCheckBox("Ascii"), self.TIP_chkAscii)

        self.chkSofQWNexus = setEnabled(tip(QCheckBox("NeXus"), self.TIP_chkNexus), self.binEon, self.binQon)
        self.chkSofQWAscii = setEnabled(tip(QCheckBox("Ascii"), self.TIP_chkAscii), self.binEon, self.binQon)

        self.rbtNormaliseNone = tip(QRadioButton("none"), self.TIP_rbtNormaliseNone)
        self.rbtNormaliseMonitor = tip(QRadioButton("to monitor"), self.TIP_rbtNormaliseMonitor)
        self.rbtNormaliseTime = tip(QRadioButton("to time"), self.TIP_rbtNormaliseTime)

        self.rbtCorrectTOFNone = tip(QRadioButton("none"), self.TIP_rbtCorrectTOFNone)
        self.rbtCorrectTOFVan = tip(QRadioButton("vanadium"), self.TIP_rbtCorrectTOFVan)
        self.rbtCorrectTOFSample = tip(QRadioButton("sample"), self.TIP_rbtCorrectTOFSample)

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

        self.gbSave = QGroupBox("Save reduced data")
        self.gbSave.setCheckable(True)

        gbDataDir = QGroupBox("Data search directory")
        gbPrefix = QGroupBox("Workspace prefix")
        gbOptions = QGroupBox("Options")
        gbInputs = QGroupBox("Inputs")
        gbBinning = QGroupBox("Binning")
        gbData = QGroupBox("Data")

        box = QVBoxLayout()
        self._layout.addLayout(box)

        box.addLayout(hbox(vbox(gbDataDir, gbInputs, gbBinning, gbOptions, 1), vbox(gbPrefix, gbData, self.gbSave)))

        gbDataDir.setLayout(hbox(self.dataDir, self.btnDataDir))
        gbPrefix.setLayout(
            hbox(
                self.prefix,
            )
        )

        grid = QGridLayout()
        grid.addWidget(self.chkSubtractECVan, 0, 0, 1, 4)
        grid.addWidget(label("Normalise", "tip"), 1, 0)
        grid.addWidget(self.rbtNormaliseNone, 1, 1)
        grid.addWidget(self.rbtNormaliseMonitor, 1, 2)
        grid.addWidget(self.rbtNormaliseTime, 1, 3)
        grid.addWidget(QLabel("Correct TOF"), 2, 0)
        grid.addWidget(self.rbtCorrectTOFNone, 2, 1)
        grid.addWidget(self.rbtCorrectTOFVan, 2, 2)
        grid.addWidget(self.rbtCorrectTOFSample, 2, 3)
        grid.addWidget(self.chkReplaceNaNs, 3, 0, 1, 4)
        grid.addWidget(self.chkCreateDiff, 4, 0, 1, 4)
        grid.addWidget(self.chkKeepSteps, 5, 0, 1, 4)
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
        grid.addWidget(QLabel("Vanadium runs"), 0, 0)
        grid.addWidget(self.vanRuns, 0, 1, 1, 3)
        grid.addWidget(QLabel("Van. comment"), 1, 0)
        grid.addWidget(self.vanCmnt, 1, 1, 1, 1)
        grid.addLayout(hbox(QLabel("EC factor"), self.vanEcFactor), 1, 2, 1, 1)
        grid.addLayout(hbox(QLabel("T (K)"), self.vanTemp), 1, 3)
        grid.addWidget(QLabel("Empty can runs"), 2, 0)
        grid.addWidget(self.ecRuns, 2, 1, 1, 1)
        grid.addLayout(hbox(QLabel("EC factor"), self.ecFactor), 2, 2, 1, 1)
        grid.addLayout(hbox(QLabel("T (K)"), self.ecTemp), 2, 3)
        grid.addWidget(QLabel("Mask detectors"), 3, 0)
        grid.addWidget(self.maskDetectors, 3, 1, 1, 3)

        gbInputs.setLayout(grid)

        grid = QGridLayout()
        grid.addWidget(QLabel("on"), 0, 1)
        grid.addWidget(QLabel("start"), 0, 2)
        grid.addWidget(QLabel("step"), 0, 3)
        grid.addWidget(QLabel("end"), 0, 4)

        grid.addWidget(QLabel("Energy"), 1, 0)
        grid.addWidget(self.binEon, 1, 1)
        grid.addWidget(self.binEstart, 1, 2)
        grid.addWidget(self.binEstep, 1, 3)
        grid.addWidget(self.binEend, 1, 4)

        grid.addWidget(QLabel("Q"), 2, 0)
        grid.addWidget(self.binQon, 2, 1)
        grid.addWidget(self.binQstart, 2, 2)
        grid.addWidget(self.binQstep, 2, 3)
        grid.addWidget(self.binQend, 2, 4)

        for col in (0, 2, 3, 4):
            grid.setColumnStretch(col, 1)

        gbBinning.setLayout(grid)

        gbData.setLayout(hbox(self.dataRunsView))

        grid = QGridLayout()
        saveDirGroup = hbox(self.saveDir, self.btnSaveDir)
        grid.addWidget(QLabel("Directory"), 0, 0)
        grid.addLayout(saveDirGroup, 0, 1, 1, 4)
        grid.addWidget(setEnabled(QLabel("S(Q, ω):"), self.binEon), 1, 0)
        grid.addWidget(self.chkSofQWNexus, 1, 1)
        grid.addWidget(self.chkSofQWAscii, 1, 2)
        grid.addItem(QSpacerItem(5, 5, hPolicy=QSizePolicy.Expanding), 1, 4)
        grid.addWidget(QLabel("S(2θ, ω):"), 2, 0)
        grid.addWidget(self.chkSofTWNexus, 2, 1)
        grid.addWidget(self.chkSofTWAscii, 2, 2)
        grid.addWidget(self.chkSofTWNxspe, 2, 3)

        self.gbSave.setLayout(grid)

        # handle signals
        self.btnDataDir.clicked.connect(self._onDataDir)
        self.btnSaveDir.clicked.connect(self._onSaveDir)
        self.runDataModel.selectCell.connect(self._onSelectedCell)

    def _onDataDir(self):
        dirname = self.dir_browse_dialog(self.dataDir.text())
        if dirname:
            self.dataDir.setText(dirname)

    def _onSaveDir(self):
        dirname = self.dir_browse_dialog(self.saveDir.text())
        if dirname:
            self.saveDir.setText(dirname)

    def _onSelectedCell(self, index):
        self.dataRunsView.setCurrentIndex(index)
        self.dataRunsView.setFocus()

    def get_state(self):
        elem = TOFTOFScriptElement()

        def line_text(lineEdit):
            return str(lineEdit.text()).strip()

        def is_checked(checkBox):
            return checkBox.isChecked() and checkBox.isEnabled()

        elem.facility_name = self._settings.facility_name
        elem.instrument_name = self._settings.instrument_name

        elem.prefix = line_text(self.prefix)
        elem.dataDir = line_text(self.dataDir)

        elem.vanRuns = line_text(self.vanRuns)
        elem.vanCmnt = line_text(self.vanCmnt)
        elem.vanTemp = OptionalFloat(line_text(self.vanTemp))
        elem.vanEcFactor = self.vanEcFactor.value()

        elem.ecRuns = line_text(self.ecRuns)
        elem.ecTemp = OptionalFloat(line_text(self.ecTemp))
        elem.ecFactor = self.ecFactor.value()

        elem.dataRuns = self.runDataModel.tableData

        elem.binEon = is_checked(self.binEon)
        elem.binEstart = self.binEstart.value()
        elem.binEstep = self.binEstep.value()
        elem.binEend = self.binEend.value()

        elem.binQon = is_checked(self.binQon)
        elem.binQstart = self.binQstart.value()
        elem.binQstep = self.binQstep.value()
        elem.binQend = self.binQend.value()

        elem.maskDetectors = line_text(self.maskDetectors)

        elem.subtractECVan = is_checked(self.chkSubtractECVan)
        elem.replaceNaNs = is_checked(self.chkReplaceNaNs)
        elem.createDiff = is_checked(self.chkCreateDiff)
        elem.keepSteps = is_checked(self.chkKeepSteps)

        elem.saveDir = line_text(self.saveDir)
        elem.saveSofTWNxspe = is_checked(self.chkSofTWNxspe)
        elem.saveSofTWNexus = is_checked(self.chkSofTWNexus)
        elem.saveSofTWAscii = is_checked(self.chkSofTWAscii)
        elem.saveSofQWNexus = is_checked(self.chkSofQWNexus)
        elem.saveSofQWAscii = is_checked(self.chkSofQWAscii)

        elem.normalise = (
            elem.NORM_MONITOR
            if self.rbtNormaliseMonitor.isChecked()
            else elem.NORM_TIME
            if self.rbtNormaliseTime.isChecked()
            else elem.NORM_NONE
        )

        elem.correctTof = (
            elem.CORR_TOF_VAN
            if self.rbtCorrectTOFVan.isChecked()
            else elem.CORR_TOF_SAMPLE
            if self.rbtCorrectTOFSample.isChecked()
            else elem.CORR_TOF_NONE
        )
        return elem

    def set_state(self, toftofScriptElement):
        elem = toftofScriptElement

        self.prefix.setText(elem.prefix)

        self.dataDir.setText(elem.dataDir)

        self.vanRuns.setText(elem.vanRuns)
        self.vanCmnt.setText(elem.vanCmnt)
        self.vanTemp.setText(str(elem.vanTemp))
        self.vanEcFactor.setValue(elem.vanEcFactor)

        self.ecRuns.setText(elem.ecRuns)
        self.ecTemp.setText(str(elem.ecTemp))
        self.ecFactor.setValue(elem.ecFactor)

        self.runDataModel.beginResetModel()
        self.runDataModel.tableData = elem.dataRuns
        self.runDataModel.endResetModel()

        self.binEon.setChecked(elem.binEon)

        self.binEstart.setValue(elem.binEstart)
        self.binEstep.setValue(elem.binEstep)
        self.binEend.setValue(elem.binEend)

        self.binQon.setChecked(elem.binQon)

        self.binQstart.setValue(elem.binQstart)
        self.binQstep.setValue(elem.binQstep)
        self.binQend.setValue(elem.binQend)

        self.maskDetectors.setText(elem.maskDetectors)

        self.chkSubtractECVan.setChecked(elem.subtractECVan)
        self.chkReplaceNaNs.setChecked(elem.replaceNaNs)
        self.chkCreateDiff.setChecked(elem.createDiff)
        self.chkKeepSteps.setChecked(elem.keepSteps)

        self.saveDir.setText(elem.saveDir)
        self.chkSofTWNxspe.setChecked(elem.saveSofTWNxspe)
        self.chkSofTWNexus.setChecked(elem.saveSofTWNexus)
        self.chkSofTWAscii.setChecked(elem.saveSofTWAscii)
        self.chkSofQWNexus.setChecked(elem.saveSofQWNexus)
        self.chkSofQWAscii.setChecked(elem.saveSofQWAscii)
        self.gbSave.setChecked(
            any((elem.saveSofTWNxspe, elem.saveSofTWNexus, elem.saveSofTWAscii, elem.saveSofQWNexus, elem.saveSofQWAscii))
        )

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


# -------------------------------------------------------------------------------
# eof
