# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import absolute_import, unicode_literals

from qtpy.QtCore import Qt

from mantidqt.utils.qt import load_ui

general_form, general_base = load_ui(__file__, "section_general.ui")


class GeneralSettingsView(general_base, general_form):
    def __init__(self, parent=None, presenter=None):
        super(GeneralSettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setVisible(False)
        self.presenter = presenter
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def closeEvent(self, event):
        self.deleteLater()
        super(GeneralSettingsView, self).closeEvent(event)
