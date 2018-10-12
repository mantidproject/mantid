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
from mantid.kernel import (BoolTimeSeriesProperty,
                           FloatTimeSeriesProperty, Int32TimeSeriesProperty,
                           Int64TimeSeriesProperty, StringTimeSeriesProperty)
from mantid.api import MultipleExperimentInfos
from qtpy.QtGui import QStandardItemModel, QStandardItem

TimeSeriesProperties = (BoolTimeSeriesProperty,
                        FloatTimeSeriesProperty, Int32TimeSeriesProperty,
                        Int64TimeSeriesProperty, StringTimeSeriesProperty)


def get_type(log):
    dtype_map = {'i': 'int', 'f': 'float', 's': 'string', 'b': 'bool'}
    if isinstance(log, TimeSeriesProperties):
        return "{} series".format(dtype_map[log.dtype()[0].lower()])
    else:
        return log.type


def get_value(log):
    if isinstance(log, TimeSeriesProperties):
        if log.size() > 1:
            return "({} entries)".format(log.size())
        else:
            return log.firstValue()
    else:
        return log.value


class SampleLogsModel(object):
    def __init__(self, ws):
        self._ws = ws
        self._exp = 0
        self._set_run()

    def _set_run(self):
        if self.isMD():
            self.run = self._ws.getExperimentInfo(self._exp).run()
        else:
            self.run = self._ws.run()

    def set_exp(self, exp):
        self._exp = exp
        self._set_run()

    def get_exp(self):
        return self._exp

    def get_ws(self):
        return self._ws

    def get_name(self):
        return self._ws.name()

    def getNumExperimentInfo(self):
        return self._ws.getNumExperimentInfo() if self.isMD() else 0

    def get_log(self, LogName):
        return self.run.getLogData(LogName)

    def get_log_names(self):
        return self.run.keys()

    def get_log_display_values(self, LogName):
        log = self.get_log(LogName)
        return log.name, get_type(log), get_value(log), log.units

    def is_log_plottable(self, LogName):
        return isinstance(self.get_log(LogName), (FloatTimeSeriesProperty,
                                                  Int32TimeSeriesProperty,
                                                  Int64TimeSeriesProperty))

    def get_statistics(self, LogName):
        log = self.get_log(LogName)
        if isinstance(log, TimeSeriesProperties):
            return log.getStatistics()

    def isMD(self):
        return isinstance(self._ws, MultipleExperimentInfos)

    def getItemModel(self):
        model = QStandardItemModel()
        model.setHorizontalHeaderLabels(["Name", "Type", "Value", "Units"])
        model.setColumnCount(4)
        for key in self.get_log_names():
            log = self.run.getLogData(key)
            name = QStandardItem()
            name.setText(log.name)
            name.setEditable(False)
            log_type = QStandardItem()
            log_type.setText(get_type(log))
            log_type.setEditable(False)
            value = QStandardItem()
            value.setText(str(get_value(log)))
            value.setEditable(False)
            unit = QStandardItem()
            unit.setText(log.units)
            unit.setEditable(False)
            model.appendRow((name, log_type, value, unit))
        model.sort(0)
        return model
