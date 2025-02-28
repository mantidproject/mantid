# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QApplication

import sys

from plot_model import PlotModel
from main_view import MainView
from main_presenter import MainPresenter


def _get_qapplication_instance() -> QApplication:
    if app := QApplication.instance():
        return app
    return QApplication(sys.argv)


app = _get_qapplication_instance()
model = PlotModel()
view = MainView()
presenter = MainPresenter(view, model)
view.show()
app.exec_()
