# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class PeaksPresenter(object):
    def __init__(self, view):
        self.view = view
        self.major = self.view.major
        self.minor = self.view.minor
        self.gamma = self.view.gamma
        self.electron = self.view.electron
        self.peak_checkboxes = self.view.peak_checkboxes

    def set_deselect_elements_slot(self,slot):
        self.view.set_deselect_elements_slot(slot)

    def enable_deselect_elements_btn(self):
        self.view.enable_deselect_elements_btn()

    def disable_deselect_elements_btn(self):
        self.view.disable_deselect_elements_btn()