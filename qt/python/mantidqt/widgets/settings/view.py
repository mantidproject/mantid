# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)

from mantidqt.utils.qt import load_ui

form, base = load_ui(__file__, "ui/main.ui")
general_form, general_base = load_ui(__file__, "ui/section_general.ui")
plots_form, plots_base = load_ui(__file__, "ui/section_plots.ui")


class PlotsSettingsView(plots_base, plots_form):
    def __init__(self, parent=None):
        super(PlotsSettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setVisible(False)


class GeneralSettingsView(general_base, general_form):
    def __init__(self, parent=None):
        super(GeneralSettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setVisible(False)


class SettingsView(base, form):
    def __init__(self, parent, presenter):
        super(SettingsView, self).__init__(parent)
        self.setupUi(self)
        self.presenter = presenter
        self.sections.currentRowChanged.connect(self.presenter.action_current_row_changed)

        self.general_settings_view = GeneralSettingsView(self)
        self.current = self.general_settings_view
        self.container.addWidget(self.general_settings_view)
        self.current.show()

        self.plots_settings_view = PlotsSettingsView(self)

