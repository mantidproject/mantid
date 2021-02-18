# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from qtpy import QtCore
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog, QTreeWidgetItem

from mantidqt.utils.qt import load_ui


class SampleMaterialDialogView(QDialog):

    set_material_signal = QtCore.Signal()
    copy_material_signal = QtCore.Signal()

    def __init__(self, parent=None):
        super(SampleMaterialDialogView, self).__init__(parent=parent)

        self.ui = load_ui(__file__, 'samplematerialdialog.ui', baseinstance=self)

        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setModal(True)

        # Top level items in the tree.
        self.formula_item = QTreeWidgetItem(self.material_properties_tree, ["Formula", ""])
        self.number_density_item = QTreeWidgetItem(self.material_properties_tree, ["Number Density", ""])
        self.temperature_item = QTreeWidgetItem(self.material_properties_tree, ["Temperature", ""])
        self.pressure_item = QTreeWidgetItem(self.material_properties_tree, ["Pressure", ""])
        self.cross_sections_item = QTreeWidgetItem(self.material_properties_tree, ["Cross Sections", ""])

        # Items under "Cross Section".
        self.absorption_item = QTreeWidgetItem(self.cross_sections_item, ["Absorption", ""])
        self.scattering_item = QTreeWidgetItem(self.cross_sections_item, ["Scattering", ""])

        # Items under "Scattering".
        self.total_item = QTreeWidgetItem(self.scattering_item, ["Total", ""])
        self.coherent_item = QTreeWidgetItem(self.scattering_item, ["Coherent", ""])
        self.incoherent_item = QTreeWidgetItem(self.scattering_item, ["Incoherent", ""])

        # Expand all items with child items.
        self.cross_sections_item.setExpanded(True)
        self.scattering_item.setExpanded(True)

        # Connect button signals.
        self.close_button.clicked.connect(self.close)
        self.copy_material_button.clicked.connect(self.copy_material_request)
        self.set_material_button.clicked.connect(self.set_material_request)

    # Set the text of the second columns for all the relevant tree items.
    def update_material(self, formula, number_density, temperature, pressure, absorption, total, coherent, incoherent):
        self.formula_item.setText(1, formula)
        self.number_density_item.setText(1, number_density)
        self.temperature_item.setText(1, temperature)
        self.pressure_item.setText(1, pressure)
        self.absorption_item.setText(1, absorption)
        self.total_item.setText(1, total)
        self.coherent_item.setText(1, coherent)
        self.incoherent_item.setText(1, incoherent)

    def copy_material_request(self):
        self.copy_material_signal.emit()

    def set_material_request(self):
        self.set_material_signal.emit()
