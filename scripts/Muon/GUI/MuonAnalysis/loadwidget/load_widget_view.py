from __future__ import (absolute_import, division, print_function)

from PyQt5 import QtCore, QtGui, QtWidgets


class LoadWidgetView(QtWidgets.QWidget):

    def __init__(self, parent=None, load_run_view=None, load_file_view=None):
        super(LoadWidgetView, self).__init__(parent)

        self.load_run_widget = load_run_view
        self.load_file_widget = load_file_view

        self.setupUi(self)

    def set_connections(self):
        self.on_multiple_loading_check_changed(self.change_multiple_loading_state)

    def on_multiple_loading_check_changed(self, slot):
        self.multiple_loading_check.stateChanged.connect(slot)

    def get_multiple_loading_state(self):
        return self.multiple_loading_check.isChecked()

    def change_multiple_loading_state(self):
        if self.get_multiple_loading_state():
            self.load_behaviour_combo.setEnabled(True)
        else:
            self.load_behaviour_combo.setEnabled(False)

    def on_subwidget_loading_started(self, slot):
        self.load_run_widget.loadingStarted.connect(slot)
        self.load_file_widget.loadingStarted.connect(slot)

    def on_subwidget_loading_finished(self, slot):
        self.load_run_widget.loadingFinished.connect(slot)
        self.load_file_widget.loadingFinished.connect(slot)

    def on_file_widget_data_changed(self, slot):
        self.load_file_widget.dataChanged.connect(slot)

    def on_run_widget_data_changed(self, slot):
        self.load_run_widget.dataChanged.connect(slot)

    def setupUi(self, Form):
        Form.setObjectName("Form")
        Form.resize(675, 180)
        self.verticalLayout_2 = QtWidgets.QVBoxLayout(Form)

        self.verticalLayout_2.addWidget(self.load_file_widget)
        self.verticalLayout_2.addWidget(self.load_run_widget)

        self.horizontalLayout_3 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_3.setObjectName("horizontalLayout_3")
        spacerItem4 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout_3.addItem(spacerItem4)

        self.multiple_loading_label = QtWidgets.QLabel(Form)
        self.multiple_loading_label.setObjectName("label_2")
        self.multiple_loading_label.setText("Multiple loading : ")
        self.horizontalLayout_3.addWidget(self.multiple_loading_label)

        self.multiple_loading_check = QtWidgets.QCheckBox()
        self.multiple_loading_check.setChecked(False)
        # self.multiple_loading_check.stateChanged.connect(lambda: self.btnstate(self.b1))
        self.horizontalLayout_3.addWidget(self.multiple_loading_check)

        self.label_2 = QtWidgets.QLabel(Form)
        self.label_2.setObjectName("label_2")
        self.label_2.setText("Load Behaviour : ")
        self.horizontalLayout_3.addWidget(self.label_2)
        self.load_behaviour_combo = QtWidgets.QComboBox(Form)
        self.load_behaviour_combo.setObjectName("load_behaviour_combo")
        self.load_behaviour_combo.addItem("Co-Add")
        self.load_behaviour_combo.addItem("Simultaneous")
        self.load_behaviour_combo.setEnabled(False)
        self.horizontalLayout_3.addWidget(self.load_behaviour_combo)
        spacerItem5 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout_3.addItem(spacerItem5)

        self.verticalLayout_2.addLayout(self.horizontalLayout_3)

        self.set_connections()
        QtCore.QMetaObject.connectSlotsByName(Form)

    def disable_loading(self):
        self.load_file_widget.disable_load_buttons()
        self.load_run_widget.disable_load_buttons()

    def enable_loading(self):
        self.load_file_widget.ensable_load_buttons()
        self.load_run_widget.ensable_load_buttons()
