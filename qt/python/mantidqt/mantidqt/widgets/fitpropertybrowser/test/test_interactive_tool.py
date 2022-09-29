# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
import unittest
from mantidqt.mantidqt.widgets.fitpropertybrowser.interactive_tool import FitInteractiveTool
from unittest.mock import MagicMock


class FitInteractiveToolTest(unittest.TestCase):

    def setUp(self) -> None:
        canvas = MagicMock()
        toolbar_manager = MagicMock()
        current_peak_type = MagicMock()
        self.interactor = FitInteractiveTool(canvas, toolbar_manager, current_peak_type)
