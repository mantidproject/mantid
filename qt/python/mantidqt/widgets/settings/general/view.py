from __future__ import (absolute_import, division, print_function)

from mantidqt.utils.qt import load_ui

general_form, general_base = load_ui(__file__, "section_general.ui")


class GeneralSettingsView(general_base, general_form):
    def __init__(self, parent=None, presenter=None):
        super(GeneralSettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setVisible(False)
        self.presenter = presenter
