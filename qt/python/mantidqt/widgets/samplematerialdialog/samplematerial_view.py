# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog, QTreeWidgetItem

from mantidqt.utils.qt import load_ui


class SampleMaterialDialogView(QDialog):
    """
    View for the sample material dialog. It displays sample material properties
    in a tree widget, and has buttons for setting and copying the sample material.
    """
    def __init__(self, presenter, parent=None):
        super(SampleMaterialDialogView, self).__init__(parent=parent)

        self.ui = load_ui(__file__, 'samplematerialdialog.ui', baseinstance=self)

        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setModal(True)

        self.presenter = presenter

        self.setAttribute(Qt.WA_DeleteOnClose, True)

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
        self.close_button.clicked.connect(self.close_request)
        self.copy_material_button.clicked.connect(self.copy_material_request)
        self.set_material_button.clicked.connect(self.set_material_request)

    def update_material(self, formula, number_density, temperature, pressure, absorption, total, coherent, incoherent):
        """
        Set the text of the second columns for all the relevant tree items.
        """
        self.formula_item.setText(1, formula)
        self.number_density_item.setText(1, str(number_density))
        self.temperature_item.setText(1, str(temperature))
        self.pressure_item.setText(1, str(pressure))
        self.absorption_item.setText(1, str(absorption))
        self.total_item.setText(1, str(total))
        self.coherent_item.setText(1, str(coherent))
        self.incoherent_item.setText(1, str(incoherent))

    def copy_material_request(self):
        self.presenter.copy_material()

    def set_material_request(self):
        self.presenter.set_material()

    def close_request(self):
        self.presenter.close_view()
