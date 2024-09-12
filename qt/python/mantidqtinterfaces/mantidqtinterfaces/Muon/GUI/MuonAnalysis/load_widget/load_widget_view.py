# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets


class LoadWidgetView(QtWidgets.QWidget):
    def __init__(self, parent=None, load_run_view=None, load_file_view=None):
        super(LoadWidgetView, self).__init__(parent)

        self.load_run_widget = load_run_view
        self.load_file_widget = load_file_view

        self.setup_interface_layout()

    def setup_interface_layout(self):
        self.clear_button = QtWidgets.QPushButton(self)
        self.clear_button.setObjectName("clearButton")
        self.clear_button.setToolTip("Clear the currently loaded data")
        self.clear_button.setText("Clear All")

        self.multiple_loading_label = QtWidgets.QLabel(self)
        self.multiple_loading_label.setObjectName("multiple_loading_label")
        self.multiple_loading_label.setText("Co-Add : ")

        self.multiple_loading_check = QtWidgets.QCheckBox(self)
        self.multiple_loading_check.setToolTip("Enable/disable co-adding selected runs")
        self.multiple_loading_check.setChecked(False)

        # Set the layout of the tools at the bottom of the widget
        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.setObjectName("horizontalLayout")
        self.horizontal_layout.addWidget(self.clear_button)
        self.horizontal_layout.addStretch(0)
        self.horizontal_layout.addWidget(self.multiple_loading_label)
        self.horizontal_layout.addWidget(self.multiple_loading_check)

        self.horizontal_layout.setContentsMargins(0, 0, 0, 0)
        self.tool_widget = QtWidgets.QWidget(self)
        self.tool_widget.setLayout(self.horizontal_layout)

        # Set the layout vertically
        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.vertical_layout.addWidget(self.load_run_widget)
        self.vertical_layout.addWidget(self.load_file_widget)
        self.vertical_layout.addWidget(self.tool_widget)

        self.group = QtWidgets.QGroupBox("Loading")
        self.group.setFlat(False)
        self.setStyleSheet(
            "QGroupBox {border: 1px solid grey;border-radius: 10px;margin-top: 1ex; margin-right: 0ex}"
            "QGroupBox:title {"
            "subcontrol-origin: margin;"
            "padding: 0 3px;"
            "subcontrol-position: top center;"
            "padding-top: 0px;"
            "padding-bottom: 0px;"
            "padding-right: 10px;"
            " color: grey; }"
        )
        self.group.setLayout(self.vertical_layout)

        self.widget_layout = QtWidgets.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)
        self.setLayout(self.widget_layout)

    def on_multiple_load_type_changed(self, slot):
        self.multiple_loading_check.stateChanged.connect(slot)

    def get_multiple_loading_state(self):
        return self.multiple_loading_check.isChecked()

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

    def on_clear_button_clicked(self, slot):
        self.clear_button.clicked.connect(slot)

    def disable_loading(self):
        self.clear_button.setEnabled(False)
        self.multiple_loading_check.setEnabled(False)

    def enable_loading(self):
        self.clear_button.setEnabled(True)
        self.multiple_loading_check.setEnabled(True)
