# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from mantid.kernel import (
    BoolTimeSeriesProperty,
    BoolFilteredTimeSeriesProperty,
    FloatTimeSeriesProperty,
    FloatFilteredTimeSeriesProperty,
    Int32TimeSeriesProperty,
    Int32FilteredTimeSeriesProperty,
    Int64TimeSeriesProperty,
    Int64FilteredTimeSeriesProperty,
    StringTimeSeriesProperty,
    StringFilteredTimeSeriesProperty,
    logger,
)
from mantid.api import MultipleExperimentInfos
from mantid.kernel import PropertyManager
from qtpy.QtGui import QStandardItemModel, QStandardItem, QColor
from qtpy.QtCore import Qt
import numpy as np

TimeSeriesProperties = (
    BoolTimeSeriesProperty,
    FloatTimeSeriesProperty,
    Int32TimeSeriesProperty,
    Int64TimeSeriesProperty,
    StringTimeSeriesProperty,
)
FilteredTimeSeriesProperties = (
    BoolFilteredTimeSeriesProperty,
    FloatFilteredTimeSeriesProperty,
    Int32FilteredTimeSeriesProperty,
    Int64FilteredTimeSeriesProperty,
    StringFilteredTimeSeriesProperty,
)

DEEP_RED = QColor.fromHsv(0, 180, 255)


def get_type(log):
    """Convert type to something readable"""
    dtype_map = {"i": "int", "f": "float", "s": "string", "b": "bool"}
    if isinstance(log, TimeSeriesProperties):
        return "{} series".format(dtype_map[log.dtype()[0].lower()])
    else:
        return log.type


def get_value(log):
    """Returns the either the value or the number of entries"""
    MAX_LOG_SIZE = 20  # the maximum log length to try to show in the value column

    if isinstance(log, TimeSeriesProperties):
        if log.size() == 1:
            # for logs which are filtered or not
            return "{} (1 entry)".format(log.filtered_value[0])
        else:
            entry_descr = "({} entries)".format(log.size())

            # show the value if they are all the same
            if log.size() < MAX_LOG_SIZE:
                value = set(log.value)
                if len(value) == 1:
                    return "{} {}".format(value.pop(), entry_descr)

            # otherwise just show the number of values
            return entry_descr
    else:
        # convert to numpy array to fix some issues converting _kernel.std_vector_dbl to string
        opt = np.get_printoptions()
        np.set_printoptions(threshold=500, edgeitems=50, linewidth=np.inf)
        s = str(np.array(log.value))
        np.set_printoptions(**opt)  # reset the default options
        return s


class SampleLogsModel:
    """This class stores the workspace object and return log values when
    requested
    """

    def __init__(self, ws):
        """Stores three thing:, the workspace, which experiment info number
        to use, and the run object.
        """
        self.set_ws(ws)

    def set_ws(self, ws):
        """Set the workspace"""
        self._ws = ws
        self._exp = 0
        self._set_run()
        self._workspace_name = self.get_name()

    def _set_run(self):
        """Set run depending on workspace type and experiment info number"""
        if self.isMD():
            self.run = self._ws.getExperimentInfo(self._exp).run()
        else:
            self.run = self._ws.run()

    def set_exp(self, exp):
        """Change the experiment info number"""
        self._exp = exp
        self._set_run()

    def get_exp(self):
        """Return the experiment info number"""
        return self._exp

    def get_ws(self):
        """Return the workspace"""
        return self._ws

    def set_name(self, workspace_name):
        """Set the workspace name to compare on ADS changes"""
        self._workspace_name = workspace_name

    def workspace_equals(self, workspace_name):
        return workspace_name == self._workspace_name

    def get_name(self):
        """Return the workspace name"""
        return self._ws.name()

    def getNumExperimentInfo(self):
        """Return number of experiment info's in workspace"""
        return self._ws.getNumExperimentInfo() if self.isMD() else 0

    def get_timeroi(self):
        return self.run.getTimeROI()

    def get_log(self, LogName):
        """Return log of given LogName"""
        return self.run.getLogData(LogName)

    def get_is_log_filtered(self, LogName):
        """Return if the log of given LogName is filtered"""
        log = self.get_log(LogName)
        return isinstance(log, FilteredTimeSeriesProperties)

    def get_log_names(self):
        """Returns a list of logs in workspace"""
        return self.run.keys()

    def get_hidden_logs(self):
        """Returns a list of log names that should be hidden and not displayed"""
        hidden_logs = []
        log_list = self.get_log_names()
        for log_name in log_list:
            if PropertyManager.isAnInvalidValuesFilterLog(log_name):
                hidden_logs.append(log_name)
        return hidden_logs

    def get_logs_with_invalid_data(self):
        """Returns a map of log names with invalid data, and the invalid filter logs
        The value of each log is the number of invalid entries,
         with -1 meaning all of the entries are invalid"""
        invalid_data_logs = {}
        log_list = self.get_log_names()
        for log_name in log_list:
            if PropertyManager.isAnInvalidValuesFilterLog(log_name):
                log = self.get_log(log_name)
                # determine if the entire log is invalid
                invalid_value_count = 0
                for log_value in log.value:
                    if not log_value:
                        invalid_value_count += 1
                if invalid_value_count == log.size():
                    invalid_value_count = -1

                filtered_log = PropertyManager.getLogNameFromInvalidValuesFilter(log_name)
                if filtered_log:
                    invalid_data_logs[filtered_log] = invalid_value_count
        return invalid_data_logs

    def get_log_display_values(self, LogName):
        """Return a row to display for a log (name, type, value, units)"""
        log = self.get_log(LogName)
        return log.name, get_type(log), get_value(log), log.units

    def are_any_logs_plottable(self):
        """returns true if any of the logs are plottable.
        Only Float, Int32 and Int64
        TimeSeriesProperties are plottable at this point.
        """
        log_names = self.get_log_names()
        for log_name in log_names:
            if self.is_log_plottable(log_name):
                return True
        return False

    def is_log_plottable(self, LogName):
        """Checks if logs is plottable. Only Float, Int32 and Int64
        TimeSeriesProperties are plottable at this point.
        """
        return isinstance(self.get_log(LogName), (FloatTimeSeriesProperty, Int32TimeSeriesProperty, Int64TimeSeriesProperty))

    def get_statistics(self, LogName, filtered=True):
        """Return the statistics of a particular log"""
        log = self.get_log(LogName)
        if isinstance(log, TimeSeriesProperties):
            if (not filtered) and isinstance(log, FilteredTimeSeriesProperties):
                log = log.unfiltered()
            return log.getStatistics()
        else:
            return self.run.getStatistics(LogName)

    def isMD(self):
        """Checks if workspace is a MD Workspace"""
        return isinstance(self._ws, MultipleExperimentInfos)

    def getItemModel(self, searched_key=""):
        """Return a QModel made from the current workspace. This should be set
        onto a QTableView. The searched_key allows for filtering log entries.
        """

        def create_table_item(column, itemname, invalid_value_count, log_size, callable, *args):
            item = QStandardItem()
            item.setEditable(False)
            # format if there is invalid data entries
            if invalid_value_count == -1:
                item.setData(DEEP_RED, Qt.BackgroundRole)
                item.setToolTip("All of the values in the log are marked invalid, none of them are filtered.")
            elif invalid_value_count > 0:
                saturation = 10 + (170 * (invalid_value_count / (log_size + invalid_value_count)))
                item.setData(QColor.fromHsv(0, int(saturation), 255), Qt.BackgroundRole)
                aux_verb = "is" if invalid_value_count == 1 else "are"
                item.setToolTip(
                    f"{invalid_value_count}/{log_size + invalid_value_count} of the values in the log"
                    f" {aux_verb} marked invalid, and {aux_verb} filtered."
                )
            try:
                item.setText(callable(*args))
            except Exception as exc:
                logger.warning("Error setting column {} for log {}: {}".format(column, itemname, str(exc)))

            return item

        model = QStandardItemModel()
        model.setHorizontalHeaderLabels(["Name", "Type", "Value", "Units"])
        model.setColumnCount(4)
        logs_to_highlight = self.get_logs_with_invalid_data()
        logs_to_hide = self.get_hidden_logs()
        for key in self.get_log_names():
            if key in logs_to_hide:
                continue
            if searched_key.casefold() not in key.casefold():
                continue
            invalid_value_count = 0
            if key in logs_to_highlight.keys():
                invalid_value_count = logs_to_highlight[key]
            log = self.run.getLogData(key)
            size = log.size() if hasattr(log, "size") else 0
            name = create_table_item("Name", key, invalid_value_count, size, lambda: log.name)
            log_type = create_table_item("Type", key, invalid_value_count, size, get_type, log)
            value = create_table_item("Value", key, invalid_value_count, size, lambda log: get_value(log), log)
            unit = create_table_item("Units", key, invalid_value_count, size, lambda: log.units)
            model.appendRow((name, log_type, value, unit))

        model.sort(0)
        return model
