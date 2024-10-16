# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# from Interface import *

import systemtesting
from unittest import mock
import sys
import time

from qtpy.QtWidgets import QApplication, QFormLayout, QLineEdit, QCheckBox, QComboBox
from qtpy.QtTest import QTest
from qtpy.QtCore import Qt, QPoint

from mantid.kernel import config
from mantid.simpleapi import mtd, GroupWorkspaces
from mantidqtinterfaces.drill.view.DrillView import DrillView
from mantidqtinterfaces.drill.view.DrillSettingsDialog import DrillSettingsDialog


app = QApplication(sys.argv)


class DrillProcessSANSTest(systemtesting.MantidSystemTest):
    """This test runs the same as SANSILLAutoProcessTest::D11_AutoProcess_Test but through DrILL"""

    def __init__(self):
        super().__init__()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"
        self.disableChecking = ["Instrument"]
        config.appendDataSearchSubDir("ILL/D11/")

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking.append("Instrument")
        return ["out", "D11_AutoProcess_Reference.nxs"]

    def editCell(self, row, column, text):
        """
        Edit a specific cell of the DrILL table.

        Args:
            row (int): row index
            column (int): column index
            text (str): string to be written in the cell
        """
        columnIndex = self.drill.table._columns.index(column)
        y = self.drill.table.rowViewportPosition(row) + 5
        x = self.drill.table.columnViewportPosition(columnIndex) + 5
        QTest.mouseClick(self.drill.table.viewport(), Qt.LeftButton, Qt.NoModifier, QPoint(x, y))
        QTest.mouseDClick(self.drill.table.viewport(), Qt.LeftButton, Qt.NoModifier, QPoint(x, y))
        QTest.keyClicks(self.drill.table.viewport().focusWidget(), text)
        QTest.keyClick(self.drill.table.viewport().focusWidget(), Qt.Key_Tab)

    @mock.patch.object(DrillSettingsDialog, "show", return_value=None)
    def editSettings(self, settingValues, mDialog):
        """
        Edit the settings window. This method will edit all the provided
        settings and set them to their correcponding value.

        Args:
            settingValues (dict(str:str)): setting name and value to be set
        """
        QTest.mouseClick(self.drill.settings, Qt.LeftButton)
        sw = self.drill.children()[-1]
        form = sw.formLayout
        widgets = dict()
        for i in range(0, sw.formLayout.rowCount()):
            label = form.itemAt(i, QFormLayout.LabelRole).widget().text()
            widget = form.itemAt(i, QFormLayout.FieldRole).widget()
            widgets[label] = widget
        for name, value in settingValues.items():
            if name in widgets:
                if isinstance(widgets[name], QLineEdit):
                    QTest.mouseClick(widgets[name], Qt.LeftButton)
                    QTest.mouseDClick(widgets[name], Qt.LeftButton)
                    widgets[name].clear()
                    QTest.keyClicks(widgets[name], value)
                    QTest.keyClick(widgets[name], Qt.Key_Tab)
                elif isinstance(widgets[name], QComboBox):
                    v = widgets[name].view()
                    m = widgets[name].model()
                    for i in range(m.rowCount()):
                        index = m.index(i, 0)
                        text = index.data(Qt.DisplayRole)
                        if text == name:
                            v.scrollTo(index)
                            pos = index.center()
                            Qt.mouseClick(v.viewport(), Qt.LeftButton, 0, pos)
                            break
        QTest.mouseClick(sw.okButton, Qt.LeftButton)

    def cleanup(self):
        mtd.clear()

    def runTest(self):
        sampleRuns = ["2889,2885,2881", "2887,2883,2879", "3187,3177,3167"]
        sampleTransmissionRuns = ["2871", "2869", "3172"]
        beamRuns = "2866,2867+2868,2878"
        transmissionBeamRuns = "2867+2868"
        containerRuns = "2888+2971,2884+2960,2880+2949"
        containerTransmissionRuns = "2870+2954"
        sampleThickness = ["0.1", "0.2", "0.2"]
        maskFiles = "mask1.nxs,mask2.nxs,mask3.nxs"

        self.drill = DrillView()
        QTest.mouseClick(self.drill.addrow, Qt.LeftButton)
        QTest.mouseClick(self.drill.addrow, Qt.LeftButton)

        self.editSettings(
            {
                "SensitivityMaps": "sens-lamp.nxs",
                "BeamRadius": "0.05,0.05,0.05",
                "CalculateResolution": "MildnerCarpenter",
                "TransmissionBeamRadius": "0.2",
            }
        )

        # remove all exports
        QTest.mouseClick(self.drill.export, Qt.LeftButton)
        ew = self.drill.children()[-1]
        for i in range(ew.algoList.rowCount()):
            widget = ew.algoList.itemAtPosition(i, 0).widget()
            if isinstance(widget, QCheckBox) and widget.isChecked():
                QTest.mouseClick(widget, Qt.LeftButton)
        QTest.mouseClick(ew.okButton, Qt.LeftButton)

        self.editCell(0, "SampleRuns", sampleRuns[0])
        self.editCell(0, "SampleTransmissionRuns", sampleTransmissionRuns[0])
        self.editCell(0, "BeamRuns", beamRuns)
        self.editCell(0, "TransmissionBeamRuns", transmissionBeamRuns)
        self.editCell(0, "ContainerRuns", containerRuns)
        self.editCell(0, "ContainerTransmissionRuns", containerTransmissionRuns)
        self.editCell(0, "SampleThickness", sampleThickness[0])
        self.editCell(0, "MaskFiles", maskFiles)
        self.editCell(0, "OutputWorkspace", "iq_s1")

        self.editCell(1, "SampleRuns", sampleRuns[1])
        self.editCell(1, "SampleTransmissionRuns", sampleTransmissionRuns[1])
        self.editCell(1, "BeamRuns", beamRuns)
        self.editCell(1, "TransmissionBeamRuns", transmissionBeamRuns)
        self.editCell(1, "ContainerRuns", containerRuns)
        self.editCell(1, "ContainerTransmissionRuns", containerTransmissionRuns)
        self.editCell(1, "SampleThickness", sampleThickness[1])
        self.editCell(1, "MaskFiles", maskFiles)
        self.editCell(1, "OutputWorkspace", "iq_s2")

        self.editCell(2, "SampleRuns", sampleRuns[2])
        self.editCell(2, "SampleTransmissionRuns", sampleTransmissionRuns[2])
        self.editCell(2, "BeamRuns", beamRuns)
        self.editCell(2, "TransmissionBeamRuns", transmissionBeamRuns)
        self.editCell(2, "ContainerRuns", containerRuns)
        self.editCell(2, "ContainerTransmissionRuns", containerTransmissionRuns)
        self.editCell(2, "SampleThickness", sampleThickness[2])
        self.editCell(2, "MaskFiles", maskFiles)
        self.editCell(2, "OutputWorkspace", "iq_s3")

        QTest.mouseClick(self.drill.buttonProcessAll, Qt.LeftButton)

        while ("iq_s1" not in mtd) or ("iq_s2" not in mtd) or ("iq_s3" not in mtd):
            time.sleep(1)

        GroupWorkspaces(InputWorkspaces=["iq_s1", "iq_s2", "iq_s3"], OutputWorkspace="out")
