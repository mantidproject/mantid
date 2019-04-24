# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from Muon.GUI.MuonAnalysis.muon_analysis_2 import MuonAnalysisGui
import qtpy as QtCore


Name = "Muon_Analysis_2"


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

