# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
from qtpy import QtWidgets, QtCore


class ShowSampleView(object):
    def add_show_button_to_table_if_shape(self, sig_view_shape_requested, table_loaded_data, ws, row, column_ind, has_shape):
        if has_shape:
            table_loaded_data.setItem(row, column_ind, QtWidgets.QTableWidgetItem(""))
            view_btn = QtWidgets.QPushButton()
            view_btn.setProperty("text", "View Shape")
            view_btn.clicked.connect(lambda _, ws_name=ws: sig_view_shape_requested.emit(ws_name))
            cell_widget = QtWidgets.QWidget()
            layout = QtWidgets.QHBoxLayout(cell_widget)
            layout.addWidget(view_btn)
            layout.setAlignment(QtCore.Qt.AlignCenter)
            layout.setContentsMargins(0, 0, 0, 0)
            table_loaded_data.setCellWidget(row, column_ind, cell_widget)
        else:
            table_loaded_data.setItem(row, column_ind, QtWidgets.QTableWidgetItem("Not set"))
