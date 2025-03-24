# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import Qt

from mantidqt.utils.qt import load_ui

form, base = load_ui(__file__, "section_plots.ui")


class PlotsSettingsView(form, base):
    """
    The view of the plots settings. The layout is constructed inside the loaded UI file.
    The connections are setup in the presenter. This view only sets up and deletes itself on close.
    """

    def __init__(self, parent=None, presenter=None):
        super(PlotsSettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setVisible(False)
        self.presenter = presenter
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def closeEvent(self, event):
        self.deleteLater()
        super(PlotsSettingsView, self).closeEvent(event)
