# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

from qtpy import QtWidgets

from MultiPlotting.AxisChanger.axis_changer_presenter import AxisChangerPresenter
from MultiPlotting.AxisChanger.axis_changer_view import AxisChangerView

class QuickEditWidget(QtWidgets.QWidget):
    def __init__(self):
        super(QuickEditWidget,self).__init__()
        button_layout = QtWidgets.QHBoxLayout()
        self.plot_selector = QtWidgets.QComboBox()

        self.x_axis_changer = AxisChangerPresenter(AxisChangerView("X"))
        #self.x_axis_changer.on_upper_bound_changed(self._update_x_axis_upper)
        #self.x_axis_changer.on_lower_bound_changed(self._update_x_axis_lower)

        self.autoscale = QtWidgets.QCheckBox("Autoscale y")
        self.autoscale.stateChanged.connect(self.changeAutoscale)
        self.y_axis_changer = AxisChangerPresenter(AxisChangerView("Y"))
        #self.y_axis_changer.on_upper_bound_changed(self._update_y_axis_upper)
        #self.y_axis_changer.on_lower_bound_changed(self._update_y_axis_lower)

        self.errors = QtWidgets.QCheckBox("Errors")
        #self.errors.stateChanged.connect(self._errors_changed)

        button_layout.addWidget(self.plot_selector)
        button_layout.addWidget(self.x_axis_changer.view)
        button_layout.addWidget(self.autoscale)
        button_layout.addWidget(self.y_axis_changer.view)
        button_layout.addWidget(self.errors)
        self.setLayout(button_layout)

    def changeAutoscale(self,state):
        self.y_axis_changer.set_enabled(state)