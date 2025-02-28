# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from presenter import Presenter
from model import Model
from view import View

from qtpy.QtWidgets import QApplication

import sys


def _get_qapplication_instance() -> QApplication:
    if app := QApplication.instance():
        return app
    return QApplication(sys.argv)


if __name__ == "__main__":
    app = _get_qapplication_instance()
    view = View()
    model = Model()
    presenter = Presenter(view, model)
    view.show()
    app.exec_()
