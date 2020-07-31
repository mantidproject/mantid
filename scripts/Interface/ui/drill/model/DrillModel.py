# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import json

from qtpy.QtCore import QObject, Signal, QThread

import mantid.simpleapi as sapi
from mantid.kernel import *
from mantid.api import *

from .specifications import RundexSettings
from .DrillAlgorithmPool import DrillAlgorithmPool
from .DrillTask import DrillTask
from .ParameterController import Parameter, ParameterController


class DrillModel(QObject):

    ###########################################################################
    # signals                                                                 #
    ###########################################################################

    """
    Raised when a process is started.
    Args:
        (int): sample index
    """
    processStarted = Signal(int)

    """
    Raised when a process finished with success.
    Args:
        (int): sample index
    """
    processSuccess = Signal(int)

    """
    Raised when a process failed.
    Args:
        (int): sample index
    """
    processError = Signal(int)

    """
    Raised when all the processing are done.
    """
    processingDone = Signal()

    """
    Raised each time the processing progress is updated.
    Args:
        (int): sample index
    """
    progressUpdate = Signal(int)

    """
    Raised when a new param is ok.
    Args:
        (int): sample index
        (str): parameter name
    """
    paramOk = Signal(int, str)

    """
    Raised when a new parameter is wrong.
    Args:
        (int): sample index
        (str): parameter name
    """
    paramError = Signal(int, str, str)

    def __init__(self):
        super(DrillModel, self).__init__()
        self.instrument = None
        self.acquisitionMode = None
        self.cycleNumber = None
        self.experimentId = None
        self.algorithm = None
        self.samples = list()
        self.settings = dict()
        self.controller = None
        self.rundexFile = None
        self.visualSettings = None

        # set the instrument and default acquisition mode
        self.setInstrument(config['default.instrument'])

        self.tasksPool = DrillAlgorithmPool()
        # setup the thread pool
        self.tasksPool.signals.taskStarted.connect(self._onTaskStarted)
        self.tasksPool.signals.taskSuccess.connect(self._onTaskSuccess)
        self.tasksPool.signals.taskError.connect(self._onTaskError)
        self.tasksPool.signals.progressUpdate.connect(self._onProcessingProgress)
        self.tasksPool.signals.processingDone.connect(self._onProcessingDone)

    def setInstrument(self, instrument):
        """
        Set the instrument. This methods change the current instrument and all
        the associated parameters (acquisition mode, algorithm, parameters). It
        also empty the sample list and the settings.

        Args:
            instrument (str): instrument name
        """
        self.rundexFile = None
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
        self.rundexFile = None
        self.samples = list()
        self.acquisitionMode = mode
        self.columns = RundexSettings.COLUMNS[self.acquisitionMode]
        self.algorithm = RundexSettings.ALGORITHM[self.acquisitionMode]
        if self.algorithm in RundexSettings.THREADS_NUMBER:
            nThreads = RundexSettings.THREADS_NUMBER[self.algorithm]
        else:
            nThreads = QThread.idealThreadCount()
        self.tasksPool.setMaxThreadCount(nThreads)
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

    def setCycleAndExperiment(self, cycle, experiment):
        """
        Set the cycle number and the experiment ID.

        Args:
            cycle (str): cycle number
            experiment (str): experiment ID
        """
        self.cycleNumber = cycle
        self.experimentId = experiment

    def getCycleAndExperiment(self):
        """
        Get the cycle number and the experiment ID.

        Returns:
            str, str: cycle number, experiment ID
        """
        return self.cycleNumber, self.experimentId

    def _initController(self):
        """
        Initialize the parameter controller.
        """
        def onParamOk(p):
            if ((p.sample != -1) and (p.name not in self.columns)):
                self.paramOk.emit(p.sample, RundexSettings.CUSTOM_OPT_JSON_KEY)
            elif ((p.name in self.columns) or (p.name in self.settings)):
                self.paramOk.emit(p.sample, p.name)

        def onParamError(p):
            if ((p.sample != -1) and (p.name not in self.columns)):
                self.paramError.emit(p.sample,
                                     RundexSettings.CUSTOM_OPT_JSON_KEY,
                                     p.errorMsg)
            elif ((p.name in self.columns) or (p.name in self.settings)):
                self.paramError.emit(p.sample, p.name, p.errorMsg)

        if (self.algorithm is None):
            return
        self.controller = ParameterController(self.algorithm)
        self.controller.signals.okParam.connect(onParamOk)
        self.controller.signals.wrongParam.connect(onParamError)
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

    def getSettingsTypes(self):
        """
        Get informations about the algorithm settings. For all settings that
        should appear in the settings dialog, the function will return their
        type, allowed values and documentation. This method is used to generate
        the settings dialog automatically.

        Returns:
            tuple(dict(str: str), dict(str: list(str)), dict(str: str)): three
                dictionnaries for type, allowed values and documentation. Each
                of them uses the setting name as key. The type is a str:
                "file", "workspace", "combobox", "bool" or "string".
        """
        alg = sapi.AlgorithmManager.createUnmanaged(self.algorithm)
        alg.initialize()

        types = dict()
        values = dict()
        docs = dict()
        for s in self.settings:
            p = alg.getProperty(s)
            if (isinstance(p, FileProperty)):
                t = "file"
            elif (isinstance(p, MultipleFileProperty)):
                t = "files"
            elif ((isinstance(p, WorkspaceGroupProperty))
                  or (isinstance(p, MatrixWorkspaceProperty))):
                t = "workspace"
            elif (isinstance(p, StringPropertyWithValue)):
                if (p.allowedValues):
                    t = "combobox"
                else:
                    t = "string"
            elif (isinstance(p, BoolPropertyWithValue)):
                t = "bool"
            else:
                t = "string"

            types[s] = t
            values[s] = p.allowedValues
            docs[s] = p.documentation

        return (types, values, docs)

    def checkParameter(self, param, value, sample=-1):
        """
        Check a parameter by giving it name and value. The sample index is a
        facultative parameter. This method pushes the parameter on the
        controller queue.

        Args:
            param (str): parameter name
            value (any): parameter value. Can be str, bool
            sample (int): sample index if it is a sample specific parameter
        """
        self.controller.addParameter(Parameter(param, value, sample))

    def checkAllParameters(self):
        """
        Check all the parameters.
        """
        for i in range(len(self.samples)):
            for (n, v) in self.samples[i].items():
                if n == "CustomOptions":
                    for (nn, vv) in v.items():
                        self.controller.addParameter(Parameter(nn, vv, i))
                else:
                    self.controller.addParameter(Parameter(n, v, i))
        for (n, v) in self.settings.items():
            self.controller.addParameter(Parameter(n, v, -1))

    def changeParameter(self, row, column, contents):
        """
        Change parameter value and update the model samples. The method is able
        to parse usual parameters (present in self.columns) and those coming
        from the custom options. It submits the new value to the parameters
        controller to check it. In case of empty value, the paramOk signal is
        sent without any submission to the controller.

        Args:
            row (int): index of the sample in self.samples
            column (int): index of the parameter in self.columns
            contents (str): new value
        """
        if (not contents):
            self.paramOk.emit(row, self.columns[column])
            if (self.columns[column] in self.samples[row]):
                del self.samples[row][self.columns[column]]
            return

        if (self.columns[column] == RundexSettings.CUSTOM_OPT_JSON_KEY):
            options = dict()
            for option in contents.split(';'):
                if ('=' not in option):
                    self.paramError.emit(row, self.columns[column],
                                         "Badly formatted custom options")
                    return
                name = option.split("=")[0]
                value = option.split("=")[1]
                # everything is a str, we try to find bool
                if value in ['true', 'True', 'TRUE']:
                    value = True
                if value in ['false', 'False', 'FALSE']:
                    value = False
                options[name] = value
            for (k, v) in options.items():
                self.checkParameter(k, v, row)
            self.samples[row][RundexSettings.CUSTOM_OPT_JSON_KEY] = options
        else:
            self.samples[row][self.columns[column]] = contents
            self.checkParameter(self.columns[column], contents, row)

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
        tasks = list()
        for e in elements:
            if (e >= len(self.samples)) or (not self.samples[e]):
                continue
            kwargs = self.getProcessingParameters(e)
            tasks.append(DrillTask(e, self.algorithm, **kwargs))
        self.tasksPool.addProcesses(tasks)

    def _onTaskStarted(self, ref):
        """
        Called each time a task starts.

        Args:
            ref (int): sample index
        """
        name = str(ref + 1)
        logger.information("Starting of sample {0} processing"
                           .format(name))
        self.processStarted.emit(ref)

    def _onTaskSuccess(self, ref):
        """
        Called when a task finished with success.

        Args:
            ref (int): sample index
        """
        name = str(ref + 1)
        logger.information("Processing of sample {0} finished with sucess"
                           .format(name))
        self.processSuccess.emit(ref)

    def _onTaskError(self, ref, msg):
        """
        Called when a processing fails. This method logs a message and fires the
        corresponding signal.

        Args:
            ref (int): sample index
            msg (str): error msg
        """
        name = str(ref + 1)
        logger.error("Error while processing sample {0}: {1}"
                     .format(name, msg))
        self.processError.emit(ref)

    def _onProcessingProgress(self, progress):
        """
        Called each time a processing updates its progress.

        Args:
            ref (int): sample index
            progress (float): progress value (between 0.0 and 1.0)
        """
        self.progressUpdate.emit(progress)

    def _onProcessingDone(self):
        """
        Called when the processing is done.
        """
        self.processingDone.emit()

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
            try:
                json_data = json.load(json_file)
            except Exception as ex:
                logger.error("Wrong file format for: {0}".format(filename))
                logger.error(str(ex))
                raise ex

        if ((RundexSettings.MODE_JSON_KEY not in json_data)
                or (RundexSettings.INSTRUMENT_JSON_KEY not in json_data)):
            logger.error("Unable to load {0}".format(filename))
            raise ValueError("Json mandatory fields '{0}' or '{1}' not found."
                             .format(RundexSettings.CYCLE_JSON_KEY,
                                     RundexSettings.INSTRUMENT_JSON_KEY))

        self.setInstrument(json_data[RundexSettings.INSTRUMENT_JSON_KEY])
        self.setAcquisitionMode(json_data[RundexSettings.MODE_JSON_KEY])

        # cycle number and experiment id
        if ((RundexSettings.CYCLE_JSON_KEY in json_data)
                and (RundexSettings.EXPERIMENT_JSON_KEY in json_data)):
            self.cycleNumber = json_data[RundexSettings.CYCLE_JSON_KEY]
            self.experimentId = json_data[RundexSettings.EXPERIMENT_JSON_KEY]

        # visual setings
        if RundexSettings.VISUAL_SETTINGS_JSON_KEY in json_data:
            self.visualSettings = json_data[
                    RundexSettings.VISUAL_SETTINGS_JSON_KEY]
        else:
            self.visualSettings = None

        # global settings
        if (RundexSettings.SETTINGS_JSON_KEY in json_data):
            self.settings.update(json_data[RundexSettings.SETTINGS_JSON_KEY])
        else:
            logger.warning("No global settings found when importing {0}. "
                           "Default settings will be used."
                           .format(filename))

        # samples
        self.samples = list()
        if ((RundexSettings.SAMPLES_JSON_KEY in json_data)
                and (json_data[RundexSettings.SAMPLES_JSON_KEY])):
            for sample in json_data[RundexSettings.SAMPLES_JSON_KEY]:
                self.samples.append(sample)
        else:
            logger.warning("No sample found when importing {0}."
                           .format(filename))

        self.rundexFile = filename

    def exportRundexData(self, filename, visualSettings=None):
        """
        Export the data in a Json rundex file.

        Args:
            filename (str): rundex file path
            visualSettings (dict): settings that the view produced and can read
        """
        json_data = dict()
        json_data[RundexSettings.INSTRUMENT_JSON_KEY] = self.instrument
        json_data[RundexSettings.MODE_JSON_KEY] = self.acquisitionMode

        # experiment
        if self.cycleNumber:
            json_data[RundexSettings.CYCLE_JSON_KEY] = self.cycleNumber
        if self.experimentId:
            json_data[RundexSettings.EXPERIMENT_JSON_KEY] = self.experimentId

        # visual setings
        if visualSettings:
            json_data[RundexSettings.VISUAL_SETTINGS_JSON_KEY] = visualSettings

        # global settings
        json_data[RundexSettings.SETTINGS_JSON_KEY] = self.settings

        # samples
        json_data[RundexSettings.SAMPLES_JSON_KEY] = list()
        for sample in self.samples:
            json_data[RundexSettings.SAMPLES_JSON_KEY].append(sample)

        with open(filename, 'w') as json_file:
            json.dump(json_data, json_file, indent=4)
        self.rundexFile = filename

    def getRundexFile(self):
        """
        Get the current rundex file.

        Returns:
            str: rundex file
        """
        return self.rundexFile

    def getVisualSettings(self):
        """
        Get the saved visual settings

        Returns:
            dict: visual settings that the view understands
        """
        return self.visualSettings

    def getColumnHeaderData(self):
        """
        Get the column names and tooltips in two lists with same length.

        Returns:
            list(str): list of column names
            list(str): list of column tooltips
        """
        if not self.columns:
            return [], []

        alg = sapi.AlgorithmManager.createUnmanaged(self.algorithm)
        alg.initialize()

        tooltips = list()
        for c in self.columns:
            try:
                p = alg.getProperty(c)
                tooltips.append(p.documentation)
            except:
                tooltips.append(c)

        return self.columns, tooltips

    def addSample(self, ref):
        """
        Add an empty sample. This method adds an empty space for a new sample.

        Args:
            ref (int): sample index
        """
        self.samples.insert(ref, dict())

    def deleteSample(self, ref):
        """
        Remove a sample.

        Args:
            ref (int): sample index
        """
        del self.samples[ref]

    def getRowsContents(self):
        """
        Get all the samples as a table, 1 sample per row.

        Returns:
            list(list(str)): table contents
        """
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
                row.append(';'.join(options))
            rows.append(row)
        return rows
