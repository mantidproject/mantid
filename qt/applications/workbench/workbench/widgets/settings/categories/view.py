# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QTreeWidgetItem

from mantidqt.utils.qt import load_ui

categories_form, categories_base = load_ui(__file__, "section_categories.ui")


class CategoriesSettingsView(categories_base, categories_form):
    """
    The view of the categories settings. The layout is constructed inside the loaded UI file.
    The connections are setup in the presenter. This view only sets up and deletes itself on close.
    """

    def __init__(self, parent=None, presenter=None):
        super(CategoriesSettingsView, self).__init__(parent)
        self.setupUi(self)
        self.setVisible(False)
        self.presenter = presenter
        self.setAttribute(Qt.WA_DeleteOnClose, True)

    def closeEvent(self, event):
        self.deleteLater()
        super(CategoriesSettingsView, self).closeEvent(event)

    def add_checked_widget_item(self, widget, name, is_hidden, parent=None):
        item = QTreeWidgetItem([name])
        item.setFlags(item.flags() | Qt.ItemIsUserCheckable)
        state = Qt.Unchecked
        if not is_hidden:
            state = Qt.Checked
        item.setCheckState(0, state)
        if parent:
            parent.addChild(item)
        else:
            widget.addTopLevelItem(item)
        return item
