# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from mantid.api import AlgorithmObserver
from mantidqt.widgets.samplematerialdialog.samplematerial_view import SampleMaterialDialogView
from mantidqt.interfacemanager import InterfaceManager


class SampleMaterialDialogPresenter(AlgorithmObserver):
    """
    Presenter for the sample material dialog view. User can view the material,
    set the material, or copy the material to another workspace.
    """

    def __init__(self, workspace, parent=None, view=None):
        super(SampleMaterialDialogPresenter, self).__init__()
        # Optional view argument allows mocking of the view.
        self.view = view or SampleMaterialDialogView(self, parent)

        self.parent = parent
        self.workspace = workspace
        self.update_material()

    def finishHandle(self):
        """
        Update the values in the dialog when one of the algorithms is run.
        """
        self.update_material()

    def update_material(self):
        material = self.workspace.sample().getMaterial()
        self.view.update_material(
            formula=material.name(),
            number_density=material.numberDensity,
            temperature=material.temperature,
            pressure=material.pressure,
            absorption=material.absorbXSection(),
            total=material.totalScatterXSection(),
            coherent=material.cohScatterXSection(),
            incoherent=material.incohScatterXSection(),
        )

    def copy_material(self):
        """
        Open a CopySample algorithm dialog with the CopyMaterial option.
        """
        presets = {
            "InputWorkspace": self.workspace.name(),
            "CopyName": "0",
            "CopyMaterial": "1",
            "CopyEnvironment": "0",
            "CopyShape": "0",
            "CopyLattice": "0",
            "CopyOrientationOnly": "0",
        }

        manager = InterfaceManager()
        dialog = manager.createDialogFromName("CopySample", -1, self.view, False, presets)
        # Subscribe to the algorithm so we can update the view when the values are changed.
        dialog.addAlgorithmObserver(self)
        dialog.setModal(True)
        dialog.show()

    def set_material(self):
        """
        Open a SetSampleMaterial algorithm dialog.
        """
        presets = {"InputWorkspace": self.workspace.name()}

        manager = InterfaceManager()
        dialog = manager.createDialogFromName("SetSampleMaterial", -1, self.view, False, presets)
        # Subscribe to the algorithm so we can update the view when the values are changed.
        dialog.addAlgorithmObserver(self)
        dialog.setModal(True)
        dialog.show()

    def show_view(self):
        self.view.show()

    def close_view(self):
        self.view.close()
