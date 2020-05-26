# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

#from mantid.api import FileFinder

from qtpy.QtWidgets import QDialog
from qtpy import uic

import os


class SansSettingsView(QDialog):

    ui_filename = "ui/SANS_settings.ui"

    def __init__(self, parent=None):
        super(SansSettingsView, self).__init__(parent)
        self.here = os.path.dirname(os.path.realpath(__file__))

        # setup ui
        uic.loadUi(os.path.join(self.here, self.ui_filename), self)
        self.okButton.clicked.connect(self.accept)
        self.cancelButton.clicked.connect(self.reject)
        self.applyButton.clicked.connect(
                lambda : self.accepted.emit()
                )

        # keep parameters accessing functions
        self.params = {
                "ThetaDependent" : [
                    self.thetaDependent.setChecked,
                    self.thetaDependent.isChecked
                    ],
                "SensitivityMaps" : [
                    self.sensitivityMaps.setUserInput,
                    self.sensitivityMaps.getUserInput
                    ],
                "DefaultMaskFile" : [
                    self.defaultMaskFile.setUserInput,
                    self.defaultMaskFile.getUserInput
                    ],
                "NormaliseBy" : [
                    self.normaliseBy.setCurrentText,
                    self.normaliseBy.currentText
                    ],
                "SampleThickness" : [
                    self.sampleThickness.setValue,
                    self.sampleThickness.value
                    ],
                "BeamRadius" : [
                    self.beamRadius.setValue,
                    self.beamRadius.value
                    ],
                "WaterCrossSection" : [
                    self.waterCrossSection.setValue,
                    self.waterCrossSection.value
                    ],
                "OutputType" : [
                    self.outputType.setCurrentText,
                    self.outputType.currentText
                    ],
                "CalculateResolution" : [
                    self.calculateResolution.setCurrentText,
                    self.calculateResolution.currentText
                    ],
                "DefaultQBinning" : [
                    self.defaultQBinning.setCurrentText,
                    self.defaultQBinning.currentText
                    ],
                "BinningFactor": [
                    self.binningFactor.setValue,
                    self.binningFactor.value
                    ],
                "OutputBinning" : [
                    self.outputBinning.setText,
                    self.outputBinning.text
                    ],
                "NPixelDivision" : [
                    self.nPixelDivision.setValue,
                    self.nPixelDivision.value
                    ],
                "NumberOfWedges" : [
                    self.numberOfWedges.setValue,
                    self.numberOfWedges.value
                    ],
                "WedgeAngle" : [
                    self.wedgeAngle.setValue,
                    self.wedgeAngle.value
                    ],
                "WedgeOffset" : [
                    self.wedgeOffset.setValue,
                    self.wedgeOffset.value
                    ],
                "AsymmetricWedges" : [
                    self.asymmetricWedges.setChecked,
                    self.asymmetricWedges.isChecked
                    ],
                "MaxQxy" : [
                    self.maxQxy.setValue,
                    self.maxQxy.value
                    ],
                "DeltaQ" : [
                    self.deltaQ.setValue,
                    self.deltaQ.value
                    ],
                "IQxQyLogBinning" : [
                    self.iQxQyLogBinning.setChecked,
                    self.iQxQyLogBinning.isChecked
                    ],
                "PanelOutputWorkspaces" : [
                    self.panelOutputWorkspaces.setUserInput,
                    self.panelOutputWorkspaces.getUserInput
                    ]
                }

        self.normaliseBy.addItems(["None", "Timer", "Monitor"])
        self.outputType.addItems(["I(Q)", "I(Qx,Qy)", "I(Phi,Q)"])
        self.calculateResolution.addItems(["MildnerCarpenter", "None"])
        self.defaultQBinning.addItems(["PixelSizeBased", "ResolutionBased"])

    def setSettings(self, settings):
        """
        Set the settings values.

        Args:
            settings (dict(str: str)): settings values
        """
        for (k, v) in settings.items():
            if k in self.params:
                self.params[k][0](v)

    def getSettings(self):
        """
        Get the settings values.

        Returns:
            dict(str: str): settings values
        """
        settings = dict()
        for (k, v) in self.params.items():
            settings[k] = v[1]()
        return settings
