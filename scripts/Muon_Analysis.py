# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from Muon.GUI.MuonAnalysis.muon_analysis_2 import MuonAnalysisGui
from qtpy import QtCore
from Muon.GUI.Common.usage_report import report_interface_startup


Name = "Muon_Analysis_2"

if 'muon_analysis' in globals():
    muon_analysis = globals()['muon_analysis']
    if not muon_analysis.isHidden():
        muon_analysis.setWindowState(
            muon_analysis.windowState(
            ) & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive)
        muon_analysis.activateWindow()
    else:
        muon_analysis = MuonAnalysisGui()
        report_interface_startup(Name)
        muon_analysis.resize(700, 700)
        muon_analysis.show()
else:
    muon_analysis = MuonAnalysisGui()
    report_interface_startup(Name)
    muon_analysis.resize(700, 700)
    muon_analysis.show()
