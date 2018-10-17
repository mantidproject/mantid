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

from qtpy import QtGui, QtCore
from qtpy.QtCore import Qt
from qtpy.QtWidgets import (QTabWidget, QTableView,
                            QHeaderView, QAbstractItemView)

from mantidqt.widgets.matrixworkspacedisplay.table_view_model import MatrixWorkspaceTableViewModelType


class MatrixWorkspaceDisplayView(QTabWidget):
    def __init__(self, presenter, parent=None, name=''):
        super(MatrixWorkspaceDisplayView, self).__init__(parent)

        self.presenter = presenter
        # change the default color of the rows - makes them light blue
        # monitors and masked rows are colored in the table's custom model
        palette = self.palette()
        palette.setColor(QtGui.QPalette.Base, QtGui.QColor(128, 255, 255))
        self.setPalette(palette)

        self.setWindowTitle("{} - Mantid".format(name))
        self.setWindowFlags(Qt.Window)
        self.table_y = QTableView()
        self.table_y.verticalHeader().hide()

        self.table_x = QTableView()
        self.table_y.verticalHeader().show()

        self.table_e = QTableView()
        self.table_e.verticalHeader().show()

        # Create sample log table
        self.table_x.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.table_y.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.table_e.setSelectionBehavior(QAbstractItemView.SelectItems)

        self.addTab(self.table_y, "Y values")
        self.addTab(self.table_x, "X values")
        self.addTab(self.table_e, "E values")

        # self.table.clicked.connect(self.presenter.clicked)
        # self.table.doubleClicked.connect(self.presenter.doubleClicked)
        # self.addWidget(self.table)
        #
        # frame_right = QFrame()
        # layout_right = QVBoxLayout()
        #
        # Add full_time and experimentinfo options
        # layout_options = QHBoxLayout()
        #
        # if isMD:
        #     layout_options.addWidget(QLabel("Experiment Info #"))
        #     self.experimentInfo = QSpinBox()
        #     self.experimentInfo.setMaximum(noExp-1)
        #     self.experimentInfo.valueChanged.connect(self.presenter.changeExpInfo)
        #     layout_options.addWidget(self.experimentInfo)
        #
        # self.full_time = QCheckBox("Relative Time")
        # self.full_time.setChecked(True)
        # self.full_time.stateChanged.connect(self.presenter.plot_logs)
        # layout_options.addWidget(self.full_time)
        # layout_right.addLayout(layout_options)
        #
        # Sample log plot
        # self.fig = Figure()
        # self.canvas = FigureCanvas(self.fig)
        # self.canvas.setSizePolicy(QSizePolicy.Expanding,QSizePolicy.Expanding)
        # self.ax = self.fig.add_subplot(111, projection='mantid')
        # layout_right.addWidget(self.canvas)
        #
        # Sample stats
        # self.create_stats_widgets()
        # layout_stats = QFormLayout()
        # layout_stats.addRow('', QLabel("Log Statistics"))
        # layout_stats.addRow('Min:', self.stats_widgets["minimum"])
        # layout_stats.addRow('Max:', self.stats_widgets["maximum"])
        # layout_stats.addRow('Mean:', self.stats_widgets["mean"])
        # layout_stats.addRow('Median:', self.stats_widgets["median"])
        # layout_stats.addRow('Std Dev:', self.stats_widgets["standard_deviation"])
        # layout_stats.addRow('Time Avg:', self.stats_widgets["time_mean"])
        # layout_stats.addRow('Time Std Dev:', self.stats_widgets["time_standard_deviation"])
        # layout_stats.addRow('Duration:', self.stats_widgets["duration"])
        # layout_right.addLayout(layout_stats)
        # frame_right.setLayout(layout_right)
        #
        # self.addWidget(frame_right)
        # self.setStretchFactor(0,1)

        self.resize(600, 400)
        self.show()

    def set_model(self, model_x, model_y, model_e):
        self._set_table_model(self.table_x, model_x, MatrixWorkspaceTableViewModelType.x)
        self._set_table_model(self.table_y, model_y, MatrixWorkspaceTableViewModelType.y)
        self._set_table_model(self.table_e, model_e, MatrixWorkspaceTableViewModelType.e)

    @staticmethod
    def _set_table_model(table, model, expected_model_type):
        assert model.type == expected_model_type, \
            "The model for the table with {0} values has a wrong model type: {1}".format(expected_model_type.upper(),
                                                                                         model.model_type)
        table.setModel(model)
