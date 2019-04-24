# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.FrequencyDomainAnalysis.frequency_domain_analysis_2 import FrequencyAnalysisGui
from qtpy import QtCore

Name = "Frequency_Domain_Analysis_2"


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

