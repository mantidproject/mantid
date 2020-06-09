# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from qtpy.QtCore import Qt, Signal, Slot

from mantidqt.utils.qt import import_qt
from mantidqt.widgets.fitpropertybrowser import FitPropertyBrowser, FitInteractiveTool
from mantid import logger
BaseBrowser = import_qt('.._common', 'mantidqt.widgets', 'FitPropertyBrowser')

class EngDiffFitPropertyBrowser(FitPropertyBrowser):
    """
    A wrapper around python FitPropertyBrowser altered for
    engineering diffraction UI
    """

    def __init__(self, canvas, toolbar_manager, parent=None):
        super(EngDiffFitPropertyBrowser, self).__init__(canvas, toolbar_manager, parent)

    def set_output_window_names(self):
        """
         Override this function as no window
        :return: None
        """
        return None

