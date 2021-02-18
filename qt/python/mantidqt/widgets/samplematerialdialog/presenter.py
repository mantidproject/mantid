# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from mantidqt.widgets.samplematerialdialog.view import SampleMaterialDialogView

from mantidqt.widgets.workspacewidget import mantidtreemodel

from qtpy.QtWidgets import QApplication


class SampleMaterialDialogPresenter:

    def __init__(self, parent=None):
        self.view = SampleMaterialDialogView(parent)

        self.view.copy_material_signal.connect(self.copy_material)
        self.view.set_material_signal.connect(self.set_material)

    def copy_material(self):
        print("copy material")
        # Add the presets list to this call...
        # mantidtreemodel.showAlgorithmDialog("CopySample")

    def set_material(self):
        print("set material")

    def show_view(self):
        self.view.show()


if __name__ == "__main__":
    app = QApplication([])
    presenter = SampleMaterialDialogPresenter()
    window = presenter.view
    window.show()
    app.exec_()
