# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from qtpy import QtCore
from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui


def main():
    try:
        global ElementalAnalysis
        if not ElementalAnalysis.isHidden():
            ElementalAnalysis.setWindowState(
                ElementalAnalysis.windowState(
                ) & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive)
            ElementalAnalysis.activateWindow()
        else:
            ElementalAnalysis = ElementalAnalysisGui()
            ElementalAnalysis.show()
    except:
        ElementalAnalysis = ElementalAnalysisGui()
        ElementalAnalysis.show()
    return ElementalAnalysis

if __name__ == '__main__':
    ElementalAnalysis = main()
