# coding=utf-8
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, unicode_literals

from qtpy.QtWidgets import QApplication  # noqa: F402

from workbench.widgets.settings.presenter import SettingsPresenter

app = QApplication([])
settings = SettingsPresenter(None)
settings.view.show()
app.exec_()
