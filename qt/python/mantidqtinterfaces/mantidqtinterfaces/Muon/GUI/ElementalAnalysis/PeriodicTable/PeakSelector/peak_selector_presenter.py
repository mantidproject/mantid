# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class PeakSelectorPresenter(object):
    def __init__(self, view):
        self.view = view
        self.primary_checkboxes = self.view.primary_checkboxes
        self.secondary_checkboxes = self.view.secondary_checkboxes
        self.gamma_checkboxes = self.view.gamma_checkboxes
        self.electron_checkboxes = self.view.electron_checkboxes

    def finish_selection(self):
        self.view.finish_selection()

    def update_peak_data(self, data):
        self.view.update_new_data(data)

    def on_finished(self, slot):
        self.view.on_finished(slot)

    def unreg_on_finished(self, slot):
        self.view.unreg_on_finished(slot)

    def get_checked(self):
        return self.view.get_checked()
