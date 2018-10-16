# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
You can run this widget independently by for example:

    from mantidqt.widgets.samplelogs.presenter import SampleLogs
    from mantid.simpleapi import Load
    from qtpy.QtWidgets import QApplication

    ws=Load('CNCS_7860')

    app = QApplication([])
    window = SampleLogs(ws)
    app.exec_()
"""
