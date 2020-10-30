# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.QuickEdit.axis_changer.axis_changer_view import AxisChangerView

class AxisChangerPresenter(object):
    def __init__(self,view):
        self._view = view

class AxisChangerWidget(object):
    def __init__(self, label, parent=None):
        self._view = AxisChangerView(label, parent)
        self._presenter = AxisChangerPresenter(self._view)

    @property
    def view(self):
        return self._view

    def get_limits(self):
        return self._view.get_limits()

    def set_limits(self, limits):
        self._view.set_limits(limits)

    def on_range_changed(self, slot):
        self._view.on_range_changed(slot)
