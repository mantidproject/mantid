# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QFontDialog

from mantidqt.utils.qt import load_ui

general_form, general_base = load_ui(__file__, "section_general.ui")


class GeneralSettingsView(general_base, general_form):
    """
    The view of the General settings. The layout is constructed inside the loaded UI file.
    The connections are setup in the presenter. This view only sets up and deletes itself on close.
    """

    def __init__(self, parent=None, presenter=None):
        super(GeneralSettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setVisible(False)
        self.presenter = presenter
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def closeEvent(self, event):
        self.deleteLater()
        super(GeneralSettingsView, self).closeEvent(event)

    def create_font_dialog(self, parent, font=None):
        if not font:
            font = parent.font()
        font_dialog = QFontDialog(font, parent)
        font_dialog.open()
        return font_dialog
