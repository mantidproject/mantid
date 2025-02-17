# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from mantidqt.utils.qt import load_ui

Ui_cropping, _ = load_ui(__file__, "cropping_widget.ui")


class CroppingView(QtWidgets.QWidget, Ui_cropping):
    def __init__(self, parent):
        super(CroppingView, self).__init__(parent)
        self.setupUi(self)
        self.widget_custom.hide()
        self.widget_crop.hide()
        self.finder_custom.setFileExtensions([".cal", ".xml"])
        self.finder_custom.allowMultipleFiles(False)
        self.finder_custom.setLabelText("Custom CalFile:")
        self.finder_custom.allowMultipleFiles(False)

    # =================
    # Slot Connectors
    # =================

    def set_on_combo_changed(self, slot):
        self.combo_bank.currentIndexChanged.connect(slot)

    def set_on_custom_calfile_changed(self, slot):
        self.finder_custom.fileEditingFinished.connect(slot)
        self.finder_custom.filesFound.connect(slot)

    def set_on_custom_spectra_changed(self, slot):
        self.edit_crop.textChanged.connect(slot)

    # =================
    # Component Setters
    # =================

    def set_custom_calfile_widget_hidden(self):
        self.widget_custom.hide()

    def set_custom_calfile_widget_visible(self):
        self.widget_custom.show()

    def set_custom_spectra_widget_hidden(self):
        self.widget_crop.hide()

    def set_custom_spectra_widget_visible(self):
        self.widget_crop.show()

    def set_crop_invalid_indicator_hidden(self):
        self.label_cropValid.hide()

    def set_crop_invalid_indicator_visible(self, string):
        self.label_cropValid.setToolTip(string)
        self.label_cropValid.show()

    # =================
    # Component Getters
    # =================

    def get_combo_value(self):
        return self.combo_bank.currentText()

    def get_custom_calfile(self):
        return self.finder_custom.getFirstFilename()

    def get_custom_spectra_text(self):
        return self.edit_crop.text()
