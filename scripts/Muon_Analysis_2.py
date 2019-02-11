# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from Muon.GUI.MuonAnalysis.muon_analysis_2 import MuonAnalysisGui
import PyQt4.QtCore as QtCore
from save_python import getWidgetIfOpen


Name = "Muon_Analysis_2"


def main():
    try:
        global muon
        if not muon.isHidden():
            muon.setWindowState(muon.windowState() & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive)
            muon.activateWindow()
        else:
            muon = MuonAnalysisGui()
            muon.resize(700, 700)
            muon.show()
    except:
        muon = MuonAnalysisGui()
        muon.resize(700, 700)
        muon.show()
    return muon


def saveToProject():
    widget = getWidgetIfOpen(Name)
    if widget is None:
        return ""
    widget.update()
    project = widget.saveToProject()
    return project


def loadFromProject(project):
    global muonGUI
    muonGUI = main()
    muonGUI.dock_widget.loadFromProject(project)
    muonGUI.loadFromContext(project)
    return muonGUI


if __name__ == '__main__':
    muon = main()
