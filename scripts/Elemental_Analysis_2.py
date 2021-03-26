# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from Muon.GUI.ElementalAnalysis2.elemental_analysis import ElementalAnalysisGui
from qtpy import QtCore
from Muon.GUI.Common.usage_report import report_interface_startup

Name = "Elemental_Analysis_2"

if 'elemental_analysis_2' in globals():
    elemental_analysis = globals()['elemental_analysis_2']
    # If the object is deleted in the C++ side it can still exist in the
    # python globals list. The try catch block below checks for this.
    try:
        is_hidden = elemental_analysis.isHidden()
    except RuntimeError:
        is_hidden = True
    if not is_hidden:
        elemental_analysis.setWindowState(elemental_analysis.windowState() & ~QtCore.Qt.WindowMinimized
                                          | QtCore.Qt.WindowActive)
        elemental_analysis.activateWindow()
    else:
        elemental_analysis = ElementalAnalysisGui()
        report_interface_startup(Name)
        elemental_analysis.resize(700, 700)
        elemental_analysis.show()
else:
    elemental_analysis = ElementalAnalysisGui()
    report_interface_startup(Name)
    elemental_analysis.resize(700, 700)
    elemental_analysis.show()
