# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from qtpy.QtWidgets import (QTableView, QHBoxLayout, QVBoxLayout,
                            QAbstractItemView, QFormLayout, QLineEdit,
                            QHeaderView, QLabel, QCheckBox, QMenu,
                            QSizePolicy, QSpinBox, QSplitter, QFrame,
                            QSpacerItem)
from qtpy.QtCore import QItemSelectionModel, Qt
from mantidqt.MPLwidgets import FigureCanvas
from matplotlib.figure import Figure
import matplotlib.pyplot as plt


class SampleLogsView(QSplitter):
    """Sample Logs View

    This contains a table of the logs, a plot of the currently
    selected logs, and the statistics of the selected log.
    """

    def __init__(self, presenter, parent=None, window_flags=Qt.Window, name='', isMD=False, noExp = 0):
        super(SampleLogsView, self).__init__(parent)

        self.presenter = presenter

        self.setWindowTitle("{} sample logs".format(name))
        self.setWindowFlags(window_flags)
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        # left hand side
        self.frame_left = QFrame()
        layout_left = QVBoxLayout()

        # add a spin box for MD workspaces
        if isMD:
            layout_mult_expt_info = QHBoxLayout()
            layout_mult_expt_info.addWidget(QLabel("Experiment Info #"))
            self.experimentInfo = QSpinBox()
            self.experimentInfo.setMaximum(noExp-1)
            self.experimentInfo.valueChanged.connect(self.presenter.changeExpInfo)
            layout_mult_expt_info.addWidget(self.experimentInfo)
            layout_mult_expt_info.addSpacerItem(QSpacerItem(10, 10, QSizePolicy.Expanding))
            layout_left.addLayout(layout_mult_expt_info)

        # create a line edit to allow for filtering keys
        self.line_edit = QLineEdit()
        self.line_edit.setClearButtonEnabled(True)
        self.line_edit.setToolTip("Type here to filter the logs")
        self.line_edit.setPlaceholderText("Search the logs")
        self.line_edit.textEdited.connect(self.presenter.search_key_changed)
        layout_left.addWidget(self.line_edit)
        # Create sample log table
        self.table = QTableView()
        self.table.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.table.doubleClicked.connect(self.presenter.doubleClicked)
        self.table.contextMenuEvent = self.tableMenu
        layout_left.addWidget(self.table)
        self.frame_left.setLayout(layout_left)
        self.addWidget(self.frame_left)

        #right hand side
        self.frame_right = QFrame()
        layout_right = QVBoxLayout()

        #Add full_time and experimentinfo options
        layout_options = QHBoxLayout()

        if isMD:
            layout_options.addWidget(QLabel("Experiment Info #"))
            self.experimentInfo = QSpinBox()
            self.experimentInfo.setMaximum(noExp-1)
            self.experimentInfo.valueChanged.connect(self.presenter.changeExpInfo)
            layout_options.addWidget(self.experimentInfo)

        #check boxes
        self.full_time = QCheckBox("Relative Time")
        self.full_time.setToolTip(
            "Shows relative time in seconds from the start of the run.")
        self.full_time.setChecked(True)
        self.full_time.stateChanged.connect(self.presenter.plot_logs)
        layout_options.addWidget(self.full_time)
        self.show_filtered = QCheckBox("Filtered Data")
        self.show_filtered.setToolTip(
            "Filtered data only shows data while running and in this period.\nInvalid values are also filtered.")
        self.show_filtered.setChecked(True)
        self.show_filtered.stateChanged.connect(self.presenter.filtered_changed)
        layout_options.addWidget(self.show_filtered)
        self.spaceItem = QSpacerItem(10, 10, QSizePolicy.Expanding)
        layout_options.addSpacerItem(self.spaceItem)
        layout_right.addLayout(layout_options)

        # Sample log plot
        self.fig = Figure()
        self.canvas = FigureCanvas(self.fig)
        self.canvas.setSizePolicy(QSizePolicy.Expanding,QSizePolicy.Expanding)
        self.canvas.mpl_connect('button_press_event', self.presenter.plot_clicked)
        self.ax = self.fig.add_subplot(111, projection='mantid')
        layout_right.addWidget(self.canvas)

        # Sample stats
        self.create_stats_widgets()
        layout_stats = QFormLayout()
        layout_stats.addRow('', QLabel("Log Statistics"))
        layout_stats.addRow('Min:', self.stats_widgets["minimum"])
        layout_stats.addRow('Max:', self.stats_widgets["maximum"])
        layout_stats.addRow('Time Avg:', self.stats_widgets["time_mean"])
        layout_stats.addRow('Time Std Dev:', self.stats_widgets["time_standard_deviation"])
        layout_stats.addRow('Mean (unweighted):', self.stats_widgets["mean"])
        layout_stats.addRow('Median (unweighted):', self.stats_widgets["median"])
        layout_stats.addRow('Std Dev:', self.stats_widgets["standard_deviation"])
        layout_stats.addRow('Duration:', self.stats_widgets["duration"])
        layout_right.addLayout(layout_stats)
        self.frame_right.setLayout(layout_right)

        self.addWidget(self.frame_right)
        self.setStretchFactor(0,1)

        self.resize(1200,800)
        self.show()

    def closeEvent(self, event):
        self.deleteLater()
        super(SampleLogsView, self).closeEvent(event)

    def tableMenu(self, event):
        """Right click menu for table, can plot or print selected logs"""
        menu = QMenu(self)
        plotAction = menu.addAction("Plot selected")
        plotAction.triggered.connect(self.presenter.new_plot_logs)
        plotAction = menu.addAction("Print selected")
        plotAction.triggered.connect(self.presenter.print_selected_logs)
        menu.exec_(event.globalPos())

    def set_model(self, model):
        """Set the model onto the table"""
        self.model = model
        self.table.setModel(self.model)
        self.table.resizeColumnsToContents()
        self.table.horizontalHeader().setSectionResizeMode(2, QHeaderView.Stretch)
        self.table.selectionModel().selectionChanged.connect(self.presenter.update)

    def show_plot_and_stats(self, show_plot_and_stats):
        """sets wether the plot and stats section should be visible"""
        if self.frame_right.isVisible() != show_plot_and_stats:
            # the desired state is nor the current state
            self.setUpdatesEnabled(False)
            current_width = self.frame_right.width()
            if current_width:
                self.last_width = current_width
            else:
                current_width = self.last_width

            if show_plot_and_stats:
                self.resize(self.width() + current_width, self.height())
            else:
                self.resize(self.width() - current_width, self.height())
            self.frame_right.setVisible(show_plot_and_stats)
            self.setUpdatesEnabled(True)

    def plot_selected_logs(self, ws, exp, rows):
        """Update the plot with the selected rows"""
        if self.frame_right.isVisible():
            self.ax.clear()
            self.create_ax_by_rows(self.ax, ws, exp, rows)
            try:
                self.fig.canvas.draw()
            except ValueError as ve:
                #this can throw an error if the plot has recently been hidden, but the error does not matter
                if not str(ve).startswith("Image size of"):
                    raise

    def new_plot_selected_logs(self, ws, exp, rows):
        """Create a new plot, in a separate window for selected rows"""
        fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})
        self.create_ax_by_rows(ax, ws, exp, rows)
        fig.show()

    def create_ax_by_rows(self, ax, ws, exp, rows):
        """Creates the plots for given rows onto axis ax"""
        for row in rows:
            log_text = self.get_row_log_name(row)
            ax.plot(ws,
                    LogName=log_text,
                    label=log_text,
                    FullTime=not self.full_time.isChecked(),
                    Filtered=self.show_filtered.isChecked(),
                    ExperimentInfo=exp)

        ax.set_ylabel('')
        if ax.get_legend_handles_labels()[0]:
            ax.legend()

    def set_log_controls(self,are_logs_filtered):
        """Sets log specific settings based on the log clicked on"""
        self.show_filtered.setEnabled(are_logs_filtered)

    def get_row_log_name(self, i):
        """Returns the log name of particular row"""
        return str(self.model.item(i, 0).text())

    def get_exp(self):
        """Get set experiment info number"""
        return self.experimentInfo.value()

    def get_selected_row_indexes(self):
        """Return a list of selected row from table"""
        return [row.row() for row in self.table.selectionModel().selectedRows()]

    def set_selected_rows(self, rows):
        """Set seleceted rows in table"""
        mode = QItemSelectionModel.Select | QItemSelectionModel.Rows
        for row in rows:
            self.table.selectionModel().select(self.model.index(row, 0), mode)

    def create_stats_widgets(self):
        """Creates the statistics widgets"""
        self.stats_widgets = {"minimum": QLineEdit(),
                              "maximum": QLineEdit(),
                              "mean": QLineEdit(),
                              "median": QLineEdit(),
                              "standard_deviation": QLineEdit(),
                              "time_mean": QLineEdit(),
                              "time_standard_deviation": QLineEdit(),
                              "duration": QLineEdit()}
        for widget in self.stats_widgets.values():
            widget.setReadOnly(True)

    def set_statistics(self, stats):
        """Updates the statistics widgets from stats dictionary"""
        for param in self.stats_widgets.keys():
            self.stats_widgets[param].setText('{:.6}'.format(getattr(stats, param)))

    def clear_statistics(self):
        """Clears the values in statistics widgets"""
        for widget in self.stats_widgets.values():
            widget.clear()
