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

form, base = load_ui(__file__, "untitled.ui")
general_form, general_base = load_ui(__file__, "section_general.ui")

class GeneralSettingsView(general_base, general_form):
    def __init__(self, parent=None):
        super(GeneralSettingsView, self).__init__(parent)
        self.setupUi(self)


class SettingsView(base, form):
    def __init__(self, parent=None):
        super(SettingsView, self).__init__(parent)
        self.setupUi(self)
        self.container.addWidget(GeneralSettingsView(self))


