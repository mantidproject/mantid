# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from __future__ import absolute_import, unicode_literals

from qtpy.QtCore import Qt

from mantidqt.utils.qt import load_ui

form, base = load_ui(__file__, "section_application.ui")


class ApplicationSettingsView(base, form):
    """
    The view of the General settings. The layout is constructed inside the loaded UI file.
    The connections are setup in the presenter. This view only sets up and deletes itself on close.
    """

    def __init__(self, parent=None, presenter=None):
        super(ApplicationSettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setVisible(False)
        self.presenter = presenter
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def closeEvent(self, event):
        self.deleteLater()
        super(ApplicationSettingsView, self).closeEvent(event)
