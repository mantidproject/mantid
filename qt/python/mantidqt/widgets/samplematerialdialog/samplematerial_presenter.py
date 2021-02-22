# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from mantidqt.widgets.samplematerialdialog.samplematerial_view import SampleMaterialDialogView

from mantidqt.interfacemanager import InterfaceManager

from qtpy.QtWidgets import QApplication


class SampleMaterialDialogPresenter:

    def __init__(self, workspace, parent=None):
        self.view = SampleMaterialDialogView(parent, self)
        self.parent = parent
        self.workspace = workspace
        self.update_material()

    def update_material(self):
        material = self.workspace.sample().getMaterial()
        self.view.update_material(formula=material.name(),
                                  number_density=material.numberDensity,
                                  temperature=material.temperature,
                                  pressure=material.pressure,
                                  absorption=material.absorbXSection(),
                                  total=material.totalScatterXSection(),
                                  coherent=material.cohScatterXSection(),
                                  incoherent=material.incohScatterXSection())

    def copy_material(self):
        presets = {"InputWorkspace": self.workspace.name(),
                   "CopyName": "0",
                   "CopyMaterial": "1",
                   "CopyEnvironment": "0",
                   "CopyShape": "0",
                   "CopyLattice": "0",
                   "CopyOrientationOnly": "0"}

        manager = InterfaceManager()
        dialog = manager.createDialogFromName("CopySample", -1, self.view, False, presets)
        dialog.setModal(True)
        dialog.show()

    def set_material(self):
        presets = {"InputWorkspace": self.workspace.name()}

        manager = InterfaceManager()
        dialog = manager.createDialogFromName("SetSampleMaterial", -1, self.view, False, presets)
        dialog.setModal(True)
        dialog.show()

    def show_view(self):
        self.view.show()

    def close_view(self):
        self.view.close()


if __name__ == "__main__":
    app = QApplication([])
    presenter = SampleMaterialDialogPresenter()
    window = presenter.view
    window.show()
    app.exec_()
