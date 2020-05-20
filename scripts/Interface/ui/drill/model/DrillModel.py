# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import json

from qtpy.QtCore import QObject, Signal

import mantid.simpleapi as sapi
from mantid.kernel import config, logger
from mantid.api import AlgorithmObserver

from .specifications import RundexSettings
from .DrillAlgorithmPool import DrillAlgorithmPool
from .DrillTask import DrillTask

class DrillException(Exception):
    """
    Custom exception that contains a list of invalid submitted elements.
    Each element in the list is a tuple (element number, text message).
    """

    def __init__(self, elements):
        super(DrillException, self).__init__()
        self.elements = elements


class DrillModel(QObject):

    algorithm = None
    process_started = Signal(int)
    process_done = Signal(int)
    process_error = Signal(int)
    processing_done = Signal()
    progress_update = Signal(int)

    def __init__(self):
        super(DrillModel, self).__init__()
        self.set_instrument(config['default.instrument'])
        self.samples = list()
        # read default settings
        self.settings = dict()
        for (k, v) in RundexSettings.SETTINGS[self.technique].items():
            self.settings[k] = v
        self.tasksPool = DrillAlgorithmPool()
        # setup the thread pool
        self.tasksPool.signals.taskStarted.connect(
                lambda row : self.process_started.emit(row)
                )
        self.tasksPool.signals.taskFinished.connect(
                lambda row : self.process_done.emit(row)
                )
        self.tasksPool.signals.taskError.connect(
                lambda row : self.process_error.emit(row)
                )
        self.tasksPool.signals.progressUpdate.connect(
                lambda p : self.progress_update.emit(p)
                )
        self.tasksPool.signals.processingDone.connect(
                lambda : self.processing_done.emit()
                )

    def setSettings(self, settings):
        """
        Update the settings from a dictionnary.

        Args:
            settings (dict(str: any)): settings key,value pairs. Value can be
                                       str, int, float or bool
        """
        for (k, v) in settings.items():
            if k in self.settings:
                self.settings[k] = v

    def getSettings(self):
        """
        Get the settings.

        Returns:
            dict(str, any): the settings. Value can be str, int, float or bool
        """
        return self.settings

    def getProcessingParameters(self, sample):
        """
        Get the keyword arguments to be provided to an algorithm. This will
        merge the global settings and the row specific settings in a single
        dictionnary. It will also remove all the empty parameters.

        Args:
            sample (int): sample index

        Returns:
            dict(str, any): key, value pairs of parameters. Value can be str,
                            int, float, bool
        """
        params = dict()
        params.update(self.settings)
        params.update(self.samples[sample])
        # override global params with custom ones
        if "CustomOptions" in params:
            params.update(params["CustomOptions"])
            del params["CustomOptions"]
        # remove empty params
        for (k, v) in list(params.items()):
            if v is None:
                del params[k]
        # add the output workspace param
        if "OutputWorkspace" not in params:
            params["OutputWorkspace"] = "sample_" + str(sample + 1)
        return params

    def process(self, elements):
        """
        Start samples processing.

        Args:
            elements (list(int)): list of sample indexes to be processed
        """
        # to be sure that the pool is cleared
        self.tasksPool.abortProcessing()
        # TODO: check the elements before algorithm submission
        tasks = list()
        errors = list()
        for e in elements:
            if (e >= len(self.samples)) or (not self.samples[e]):
                continue
            kwargs = self.getProcessingParameters(e)
            try:
                tasks.append(DrillTask(e, self.algorithm, **kwargs))
            except Exception as ex:
                errors.append((e, str(ex)))
        if errors:
            raise DrillException(errors)
        else:
            for t in tasks:
                self.tasksPool.addProcess(t)

    def stopProcess(self):
        """
        Stop current processing.
        """
        self.tasksPool.abortProcessing()

    def set_instrument(self, instrument):
        self.samples = list()
        if (instrument in RundexSettings.TECHNIQUES):
            config['default.instrument'] = instrument
            self.instrument = instrument
            self.set_technique(0)
        else:
            logger.error('Instrument {0} is not supported yet.'
                    .format(instrument))
            self.instrument = None
            self.technique = None
            self.columns = None
            self.algorithm = None

    def set_technique(self, technique):
        self.samples = list()
        self.technique = RundexSettings.TECHNIQUES[self.instrument][technique]
        self.columns = RundexSettings.COLUMNS[self.technique]
        self.algorithm = RundexSettings.ALGORITHMS[self.technique]

    def get_columns(self):
        return self.columns if self.columns is not None else list()

    def get_technique(self):
        return RundexSettings.TECHNIQUES[self.instrument].index(self.technique)

    def get_available_techniques(self):
        return RundexSettings.TECHNIQUES[self.instrument]

    def add_row(self, position):
        self.samples.insert(position, dict())

    def del_row(self, position):
        del self.samples[position]

    def change_data(self, row, column, contents):
        if (not contents):
            if (self.columns[column] in self.samples[row]):
                del self.samples[row][self.columns[column]]
        elif (self.columns[column] == self.columns[-1]):
            options = dict()
            for option in contents.split(","):
                # ignore bad formatted options
                if ('=' not in option):
                    continue
                options[option.split("=")[0]] = option.split("=")[1]
            self.samples[row][self.columns[column]] = options
        else:
            self.samples[row][self.columns[column]] = contents

    def set_rundex_data(self, filename):
        with open(filename) as json_file:
            json_data = json.load(json_file)

        self.samples = list()
        self.set_instrument(json_data["Instrument"])

        for sample in json_data["Samples"]:
            self.samples.append(sample)

    def export_rundex_data(self, filename):
        json_data = dict()
        # header TODO: to be continued
        json_data["Instrument"] = self.instrument
        json_data["Technique"] = self.technique

        # TODO: add settings here

        # samples
        json_data["Samples"] = list()
        for sample in self.samples:
            json_data["Samples"].append(sample)

        with open(filename, 'w') as json_file:
            json.dump(json_data, json_file, indent=4)

    def get_rows_contents(self):
        rows = list()
        for sample in self.samples:
            row = list()
            for column in self.columns[:-1]:
                if column in sample:
                    row.append(str(sample[column]))
                else:
                    row.append("")
            if self.columns[-1] in sample:
                options = list()
                for (k, v) in sample[self.columns[-1]].items():
                    options.append(str(k) + "=" + str(v))
                row.append(','.join(options))
            rows.append(row)
        return rows

    def get_supported_techniques(self):
        return [technique for technique in RundexSettings.ALGORITHMS]

