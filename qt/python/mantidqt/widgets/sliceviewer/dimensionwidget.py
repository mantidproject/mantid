# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)
from qtpy.QtWidgets import QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton, QSlider, QDoubleSpinBox, QSpinBox
from qtpy.QtCore import Qt, Signal
from mantid.py3compat.enum import Enum


class State(Enum):
    X = 0
    Y = 1
    NONE = 2
    DISABLE = 3


class DimensionWidget(QWidget):
    dimensionsChanged = Signal()
    valueChanged = Signal()
    """
    Hold all the individual dimensions


    from mantidqt.widgets.sliceviewer.dimensionwidget import DimensionWidget
    from qtpy.QtWidgets import QApplication
    app = QApplication([])
    dims = [{'minimum':-1.1, 'number_of_bins':11, 'width':0.2, 'name':'Dim0', 'units':'A'},
            {'minimum':-2.1, 'number_of_bins':21, 'width':0.2, 'name':'Dim1', 'units':'B'},
            {'minimum':-10.05, 'number_of_bins':201, 'width':0.1, 'name':'Dim2', 'units':'C'}]
    window = DimensionWidget(dims)
    window.show()
    app.exec_()
    """
    def __init__(self, dims_info, parent=None):
        super(DimensionWidget, self).__init__(parent)

        self.layout = QVBoxLayout(self)

        self.dims = []
        for n, dim in enumerate(dims_info):
            if dim['type'] == 'MDE':
                self.dims.append(DimensionMDE(dim, number=n, parent=self))
            else:
                self.dims.append(Dimension(dim, number=n, parent=self))

        for widget in self.dims:
            widget.stateChanged.connect(self.change_dims)
            widget.valueChanged.connect(self.valueChanged)
            if hasattr(widget, 'binningChanged'):
                widget.binningChanged.connect(self.dimensionsChanged)
            self.layout.addWidget(widget)

        self.set_initial_states()

        self.normalize_name_widths()

        self.transpose = False

    def change_dims(self, number):
        states = [d.get_state() for n, d in enumerate(self.dims)]

        if states.count(State.X) == 0:
            for n, d in enumerate(self.dims):
                if n != number and d.get_state() == State.Y:
                    d.set_state(State.X)
        elif states.count(State.Y) == 0:
            for n, d in enumerate(self.dims):
                if n != number and d.get_state() == State.X:
                    d.set_state(State.Y)
        elif states.count(State.X) == 2:
            for n, d in enumerate(self.dims):
                if n != number and d.get_state() == State.X:
                    d.set_state(State.NONE)
        elif states.count(State.Y) == 2:
            for n, d in enumerate(self.dims):
                if n != number and d.get_state() == State.Y:
                    d.set_state(State.NONE)

        self.check_transpose()

        self.dimensionsChanged.emit()

    def check_transpose(self):
        for d in reversed(self.dims):
            if d.get_state() == State.X:
                self.transpose = False
            elif d.get_state() == State.Y:
                self.transpose = True

    def set_initial_states(self):
        # set first 2 State.NONE dimensions as x and y
        none_state_dims = [d for d in self.dims if d.state==State.NONE]
        none_state_dims[0].set_state(State.X)
        none_state_dims[1].set_state(State.Y)

    def normalize_name_widths(self):
        max_name_width = max(d.name.sizeHint().width() for d in self.dims)
        max_unit_width = max(d.units.sizeHint().width() for d in self.dims)
        for d in self.dims:
            d.name.setMinimumWidth(max_name_width)
            d.units.setMinimumWidth(max_unit_width)

    def get_slicepoint(self):
        return [None if d.get_state() in (State.X, State.Y) else d.get_value() for d in self.dims]

    def get_bin_params(self):
        return [d.get_bins() if d.get_state() in (State.X, State.Y) else d.get_thickness() for d in self.dims]


class Dimension(QWidget):
    stateChanged = Signal(int)
    valueChanged = Signal()
    """
    pass in dimension

    state: one of (State.X, State.Y, State.NONE, State.DISBALE)

    Can be run independently by:

    from mantidqt.widgets.sliceviewer.dimensionwidget import Dimension
    from qtpy.QtWidgets import QApplication
    app = QApplication([])
    window = Dimension({'minimum':-1.1, 'number_of_bins':11, 'width':0.2, 'name':'Dim0', 'units':'A'})
    window.show()
    app.exec_()
    """
    def __init__(self, dim_info, number=0, state=State.NONE, parent=None):
        super(Dimension, self).__init__(parent)

        self.minimum = dim_info['minimum']
        self.nbins = dim_info['number_of_bins']
        self.width = dim_info['width']
        self.number = number

        self.layout = QHBoxLayout(self)

        self.name = QLabel(dim_info['name'])
        self.units = QLabel(dim_info['units'])

        self.x = QPushButton('X')
        self.x.setFixedSize(32,32)
        self.x.setCheckable(True)
        self.x.clicked.connect(self.x_clicked)

        self.y = QPushButton('Y')
        self.y.setFixedSize(32,32)
        self.y.setCheckable(True)
        self.y.clicked.connect(self.y_clicked)

        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(0, self.nbins-1)
        self.slider.valueChanged.connect(self.slider_changed)

        self.spinbox = QDoubleSpinBox()
        self.spinbox.setDecimals(3)
        self.spinbox.setRange(self.get_bin_center(0), self.get_bin_center(self.nbins-1))
        self.spinbox.setSingleStep(self.width)
        self.spinbox.valueChanged.connect(self.spinbox_changed)

        self.layout.addWidget(self.name)
        self.layout.addWidget(self.x)
        self.layout.addWidget(self.y)
        self.layout.addWidget(self.slider, stretch=1)
        self.layout.addStretch(0)
        self.layout.addWidget(self.spinbox)
        self.layout.addWidget(self.units)

        self.set_value(0)

        if self.nbins < 2:
            state = State.DISABLE

        self.set_state(state)

    def set_state(self, state):
        self.state = state
        if self.state == State.X:
            self.x.setChecked(True)
            self.y.setChecked(False)
            self.slider.hide()
            self.spinbox.hide()
            self.units.hide()
        elif self.state == State.Y:
            self.x.setChecked(False)
            self.y.setChecked(True)
            self.slider.hide()
            self.spinbox.hide()
            self.units.hide()
        elif self.state == State.NONE:
            self.x.setChecked(False)
            self.y.setChecked(False)
            self.slider.show()
            self.spinbox.show()
            self.units.show()
        else:
            self.x.setChecked(False)
            self.x.setDisabled(True)
            self.y.setChecked(False)
            self.y.setDisabled(True)
            self.slider.hide()
            self.spinbox.show()
            self.spinbox.setDisabled(True)
            self.units.show()

    def get_state(self):
        return self.state

    def x_clicked(self):
        old_state = self.state
        self.set_state(State.X)
        if self.state != old_state:
            self.stateChanged.emit(self.number)

    def y_clicked(self):
        old_state = self.state
        self.set_state(State.Y)
        if self.state != old_state:
            self.stateChanged.emit(self.number)

    def spinbox_changed(self):
        self.value = self.spinbox.value()
        self.update_slider()
        self.valueChanged.emit()

    def slider_changed(self):
        self.value = self.get_bin_center(self.slider.value())
        self.update_spinbox()
        self.valueChanged.emit()

    def get_bin_center(self, n):
        return (n+0.5)*self.width+self.minimum

    def update_slider(self):
        i = (self.value-self.minimum)/self.width
        self.slider.setValue(int(min(max(i, 0), self.nbins-1)))

    def update_spinbox(self):
        self.spinbox.setValue(self.value)

    def set_value(self, value):
        self.value = value
        self.update_slider()
        self.update_spinbox()

    def get_value(self):
        return self.value


class DimensionMDE(Dimension):
    binningChanged = Signal()
    """
    MDEventWorkspace has additional properties for either number_of_bins or thickness

    from mantidqt.widgets.sliceviewer.dimensionwidget import DimensionMDE
    from qtpy.QtWidgets import QApplication
    app = QApplication([])
    window = DimensionMDE({'minimum':-1.1, 'number_of_bins':11, 'width':0.2, 'name':'Dim0', 'units':'A'})
    window.show()
    app.exec_()
    """
    def __init__(self, dim_info, number=0, state=State.NONE, parent=None):

        # hack in a number_of_bins for MDEventWorkspace
        dim_info['number_of_bins'] = 1000
        dim_info['width'] = (dim_info['maximum']-dim_info['minimum'])/1000

        self.spinBins = QSpinBox()
        self.spinBins.setRange(2,9999)
        self.spinBins.setValue(100)
        self.spinBins.hide()
        self.spinBins.setMinimumWidth(110)
        self.spinThick = QDoubleSpinBox()
        self.spinThick.setRange(0.001,999)
        self.spinThick.setValue(0.1)
        self.spinThick.setSingleStep(0.1)
        self.spinThick.setDecimals(3)
        self.spinThick.setMinimumWidth(110)
        self.rebinLabel = QLabel("thick")
        self.rebinLabel.setMinimumWidth(44)

        super(DimensionMDE, self).__init__(dim_info, number, state, parent)

        self.spinBins.valueChanged.connect(self.binningChanged)
        self.spinThick.valueChanged.connect(self.valueChanged)

        self.layout.addWidget(self.spinBins)
        self.layout.addWidget(self.spinThick)
        self.layout.addWidget(self.rebinLabel)

    def get_bins(self):
        return int(self.spinBins.value())

    def get_thickness(self):
        return float(self.spinThick.value())

    def set_state(self, state):
        super(DimensionMDE, self).set_state(state)
        if self.state == State.X:
            self.spinBins.show()
            self.spinThick.hide()
            self.rebinLabel.setText('bins')
        elif self.state == State.Y:
            self.spinBins.show()
            self.spinThick.hide()
            self.rebinLabel.setText('bins')
        elif self.state == State.NONE:
            self.spinBins.hide()
            self.spinThick.show()
            self.rebinLabel.setText('thick')
        else:
            self.spinBins.hide()
            self.spinThick.hide()
            self.rebinLabel.hide()
