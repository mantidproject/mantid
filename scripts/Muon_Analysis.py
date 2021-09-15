# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from Muon.GUI.MuonAnalysis.muon_analysis_2 import MuonAnalysisGui
from qtpy import QtCore
from Muon.GUI.Common.usage_report import report_interface_startup
import sys

Name = "Muon_Analysis_2"

if 'workbench' in sys.modules:
    from workbench.config import get_window_config

    parent, flags = get_window_config()
else:
    parent, flags = None, None

if 'muon_analysis' in globals():
    muon_analysis = globals()['muon_analysis']
    # If the object is deleted in the C++ side it can still exist in the
    # python globals list. The try catch block below checks for this.
    try:
        is_hidden = muon_analysis.isHidden()
    except RuntimeError:
        is_hidden = True
    if not is_hidden:
        muon_analysis.setWindowState(
            muon_analysis.windowState(
            ) & ~QtCore.Qt.WindowMinimized | QtCore.Qt.WindowActive)
        muon_analysis.activateWindow()
    else:
        muon_analysis = MuonAnalysisGui(parent, flags)
        report_interface_startup(Name)
        muon_analysis.resize(750, 750)
        muon_analysis.show()
else:
    muon_analysis = MuonAnalysisGui(parent, flags)
    report_interface_startup(Name)
    muon_analysis.resize(800, 800)
    muon_analysis.show()
