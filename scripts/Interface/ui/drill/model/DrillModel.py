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
from .ParameterController import Parameter, ParameterController


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
    param_ok = Signal(int, str)
    param_error = Signal(int, str, str)

    def __init__(self):
        super(DrillModel, self).__init__()
        self.instrument = None
        self.acquisitionMode = None
        self.algorithm = None
        self.samples = list()
        self.settings = dict()
        self.controller = None

        # set the instrument and default acquisition mode
        self.setInstrument(config['default.instrument'])

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

    def setInstrument(self, instrument):
        """
        Set the instrument. This methods change the current instrument and all
        the associated parameters (acquisition mode, algorithm, parameters). It
        also empty the sample list and the settings.

        Args:
            instrument (str): instrument name
        """
        self.samples = list()
        self.settings = dict()

        if (instrument in RundexSettings.ACQUISITION_MODES):
            config['default.instrument'] = instrument
            self.instrument = instrument
            self.acquisitionMode = \
                RundexSettings.ACQUISITION_MODES[instrument][0]
            self.columns = RundexSettings.COLUMNS[self.acquisitionMode]
            self.algorithm = RundexSettings.ALGORITHM[self.acquisitionMode]
            self.settings.update(
                    RundexSettings.SETTINGS[self.acquisitionMode])
            self._initController()
        else:
            logger.error('Instrument {0} is not supported yet.'
                         .format(instrument))
            self.instrument = None
            self.acquisitionMode = None
            self.columns = list()
            self.algorithm = None
            self.settings = dict()

    def getInstrument(self):
        """
        Get the current instrument.

        Returns:
            str: the instrument name
        """
        return self.instrument

    def getAvailableTechniques(self):
        """
        Get the list of techniques available.

        Returns:
            list(str): list of techniques
        """
        return [technique for (instrument, technique)
                              in RundexSettings.TECHNIQUE.items()]

    def setAcquisitionMode(self, mode):
        """
        Set the acquisition mode. The acquisition mode is modified only if it
        corresponds to the current used instrument. If success, the parameters
        controller is also setup.

        Args:
            mode (str): aquisition mode name
        """
        if ((self.instrument is None)
                or (mode not in RundexSettings.ACQUISITION_MODES[
                    self.instrument])):
            return
        self.samples = list()
        self.acquisitionMode = mode
        self.columns = RundexSettings.COLUMNS[self.acquisitionMode]
        self.algorithm = RundexSettings.ALGORITHM[self.acquisitionMode]
        self.settings = dict()
        self.settings.update(RundexSettings.SETTINGS[self.acquisitionMode])
        self._initController()

    def getAcquisitionMode(self):
        """
        Get the current acquisition mode.

        Returns:
            str: the acquisition mode
        """
        return self.acquisitionMode

    def getAvailableAcquisitionModes(self):
        """
        Get the list of acquisition mode available for the current instrument.
        """
        if (self.instrument is None):
            return list()
        return RundexSettings.ACQUISITION_MODES[self.instrument]

    def _initController(self):
        """
        Initialize the parameter controller.
        """
        if (self.algorithm is None):
            return
        self.controller = ParameterController(self.algorithm)
        self.controller.signals.okParam.connect(
                lambda p : self.param_ok.emit(
                    p.sample, p.name
                    )
                )
        self.controller.signals.wrongParam.connect(
                lambda p : self.param_error.emit(
                    p.sample, p.name, p.errorMsg
                    )
                )
        self.controller.start()

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

    def importRundexData(self, filename):
        """
        Import data contained in a Json rundex file.

        Args:
            filename(str): rundex file path
        """
        with open(filename) as json_file:
            json_data = json.load(json_file)

        self.setInstrument(json_data[RundexSettings.INSTRUMENT_JSON_KEY])

        # global settings
        self.settings = json_data[RundexSettings.SETTINGS_JSON_KEY]

        # samples
        self.samples = list()
        for sample in json_data[RundexSettings.SAMPLES_JSON_KEY]:
            self.samples.append(sample)

    def exportRundexData(self, filename):
        """
        Export the data in a Json rundex file.

        Args:
            filename(str): rundex file path
        """
        json_data = dict()
        json_data[RundexSettings.INSTRUMENT_JSON_KEY] = self.instrument
        json_data[RundexSettings.TECHNIQUE_JSON_KEY] = self.acquisitionMode

        # global settings
        json_data[RundexSettings.SETTINGS_JSON_KEY] = self.settings

        # samples
        json_data[RundexSettings.SAMPLES_JSON_KEY] = list()
        for sample in self.samples:
            json_data[RundexSettings.SAMPLES_JSON_KEY].append(sample)

        with open(filename, 'w') as json_file:
            json.dump(json_data, json_file, indent=4)

    def get_columns(self):
        return self.columns if self.columns is not None else list()

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
            self.controller.addParameter(Parameter(self.columns[column],
                                                   contents,
                                                   row))

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

