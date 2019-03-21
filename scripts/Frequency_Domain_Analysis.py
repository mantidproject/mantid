# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis2.frequency_domain_analysis_2 import FrequencyAnalysisGui
import PyQt4.QtCore as QtCore
from save_python import getWidgetIfOpen


Name = "Frequency_Domain_Analysis_2"


def main():
    try:
        global muon_freq
        if not muon_freq.isHidden():
            muon_freq.setWindowState(
                muon_freq.windowState(
                ) & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive)
            muon_freq.activateWindow()
        else:
            muon_freq = FrequencyAnalysisGui()
            muon_freq.resize(700, 700)
            muon_freq.show()
    except:
        muon_freq = FrequencyAnalysisGui()
        muon_freq.resize(700, 700)
        muon_freq.show()
    return muon_freq


def saveToProject():
    widget = getWidgetIfOpen(Name)
    if widget is None:
        return ""
    widget.update()
    project = widget.saveToProject()
    return project


def loadFromProject(project):
    global muon_freq
    muon_freq = main()
    muon_freq.dock_widget.loadFromProject(project)
    muon_freq.loadFromContext(project)
    return muon_freq


if __name__ == '__main__':
    muon = main()
