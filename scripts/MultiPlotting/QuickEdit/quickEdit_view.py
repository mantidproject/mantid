# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function
from qtpy import QtWidgets, QtCore

from MultiPlotting.AxisChanger.axis_changer_presenter import AxisChangerPresenter
from MultiPlotting.AxisChanger.axis_changer_view import AxisChangerView

class QuickEditView(QtWidgets.QWidget):
    autoscale_signal = QtCore.Signal(object)
    selection_signal = QtCore.Signal(object)

    def __init__(self, subcontext, parent = None):
        super(QuickEditView,self).__init__(parent)

        button_layout = QtWidgets.QHBoxLayout()
        self.plot_selector = QtWidgets.QComboBox()
        self.plot_selector.addItem("All")

        self.x_axis_changer = AxisChangerPresenter(AxisChangerView("X"))

        self.autoscale = QtWidgets.QCheckBox("Autoscale y")
        self.y_axis_changer = AxisChangerPresenter(AxisChangerView("Y"))

        self.errors = QtWidgets.QCheckBox("Errors")

        button_layout.addWidget(self.plot_selector)
        button_layout.addWidget(self.x_axis_changer.view)
        button_layout.addWidget(self.autoscale)
        button_layout.addWidget(self.y_axis_changer.view)
        button_layout.addWidget(self.errors)
        self.setLayout(button_layout)

    def current_selection(self):
        return self.plot_selector.currentText()

    def find_index(self,name):
        return self.plot_selector.findText(name)

    def set_index(self,index):
        self.plot_selector.setCurrentIndex(index)

    def plot_at_index(self,index):
        return self.plot_selector.itemText(index)

    def number_of_plots(self):
        return self.plot_selector.count()

    def add_subplot(self,name):
        self.plot_selector.addItem(name)

    def connect_errors_changed(self,slot):
        self.errors.stateChanged.connect(slot)

    def connect_autoscale_changed(self,slot):
        self.autoscale.stateChanged.connect(slot)

    def change_autoscale(self,state):
        self.y_axis_changer.set_enabled(state)

    def connect_x_range_changed(self,slot):
        self.x_axis_changer.on_bound_changed(slot)

    def connect_y_range_changed(self,slot):
        self.y_axis_changer.on_bound_changed(slot)

    def connect_plot_selection(self,slot):
        self.plot_selector.currentIndexChanged.connect(slot)

    def loadFromContext(self,context):
        self.x_axis_changer.set_bounds(context["x bounds"])
        self.y_axis_changer.set_bounds(context["y bounds"])

    def getSubContext(self):
        subcontext = {}
        subcontext["x bounds"] = self.x_axis_changer.get_bounds()
        subcontext["y bounds"] = self.y_axis_changer.get_bounds()
        return subcontext

    def set_y_autoscale(self,state):
        self.autoscale.setCheckState(state)

    def set_plot_x_range(self,range):
        self.x_axis_changer.set_bounds(range)
    def set_plot_y_range(self,range):
        self.y_axis_changer.set_bounds(range)

    def get_y_bounds(self):
        return self.y_axis_changer.get_bounds()
