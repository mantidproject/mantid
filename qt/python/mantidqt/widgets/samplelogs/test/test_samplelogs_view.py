# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.utils.qt.test import GuiTest
from mantidqt.widgets.samplelogs.presenter import SampleLogs


class SampleLogsViesTest(GuiTest):
    def test_deleted_on_close(self):
        ws = CreateSampleWorkspace()
        pres = SampleLogs(ws)
