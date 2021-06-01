# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os
import sys
import numpy

from qtpy.QtCore import QObject, Signal, QThread

import mantid.simpleapi as sapi
from mantid.kernel import *
from mantid.api import *

from .configurations import RundexSettings
from .DrillAlgorithmPool import DrillAlgorithmPool
from .DrillTask import DrillTask
from .DrillParameterController import DrillParameterController
from .DrillExportModel import DrillExportModel
from .DrillRundexIO import DrillRundexIO
from .DrillSample import DrillSample
from .DrillParameter import DrillParameter


class DrillModel(QObject):

    """
    Data directory on Linux.
    """
    LINUX_BASE_DATA_PATH = "/net4/serdon/illdata/"

    """
    Data directory on MacOS.
    """
    MACOS_BASE_DATA_PATH = "/Volumes/illdata/"

    """
    Data directory on Windows.
    """
    WIN_BASE_DATA_PATH = "Z:\\"

    """
    Raw data directory name.
    """
    RAW_DATA_DIR = "rawdata"

    """
    Processed data directory name.
    """
    PROCESSED_DATA_DIR = "processed"

    ###########################################################################
    # signals                                                                 #
    ###########################################################################

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
    Sent when the model get a new sample.
    Args:
        DrillSample: the new sample
    """
    newSample = Signal(DrillSample)

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
        self.visualSettings = dict()
        self.rundexIO = None
        self.exportModel = None

        # set the instrument and default acquisition mode
        self.tasksPool = DrillAlgorithmPool()
        self.setInstrument(config['default.instrument'], log=False)

        # setup the thread pool
        self.tasksPool.signals.taskStarted.connect(self._onTaskStarted)
        self.tasksPool.signals.taskSuccess.connect(self._onTaskSuccess)
        self.tasksPool.signals.taskError.connect(self._onTaskError)
        self.tasksPool.signals.progressUpdate.connect(self._onProcessingProgress)
        self.tasksPool.signals.processingDone.connect(self._onProcessingDone)

    def clear(self):
        """
        Clear the sample list and the settings.
        """
        self.samples = list()
        self.visualSettings = dict()
        self._setDefaultSettings()

    def setInstrument(self, instrument, log=True):
        """
        Set the instrument. This methods change the current instrument and all
        the associated parameters (acquisition mode, algorithm, parameters). It
        also empty the sample list and the settings.

        Args:
            instrument (str): instrument name
        """
        self.samples = list()
        self.settings = dict()
        self.columns = list()
        self.visualSettings = dict()
        self.instrument = None
        self.acquisitionMode = None
        self.algorithm = None
        self.exportModel = None

        # When the user changes the facility after DrILL has been started
        if config['default.facility'] != 'ILL':
            logger.error('Drill is enabled only if the facility is set to ILL.')
            return

        if (instrument in RundexSettings.ACQUISITION_MODES):
            config['default.instrument'] = instrument
            self.instrument = instrument
            self.setAcquisitionMode(RundexSettings.ACQUISITION_MODES[instrument][0])
        else:
            if log:
                logger.error('Instrument {0} is not supported yet.'
                             .format(instrument))

    def getInstrument(self):
        """
        Get the current instrument.

        Returns:
            str: the instrument name
        """
        return self.instrument

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
        self.visualSettings = dict()
        if mode in RundexSettings.VISUAL_SETTINGS:
            self.visualSettings = RundexSettings.VISUAL_SETTINGS[mode]
        self.acquisitionMode = mode
        self.columns = RundexSettings.COLUMNS[self.acquisitionMode]
        self.algorithm = RundexSettings.ALGORITHM[self.acquisitionMode]
        if self.acquisitionMode in RundexSettings.THREADS_NUMBER:
            nThreads = RundexSettings.THREADS_NUMBER[self.acquisitionMode]
        else:
            nThreads = QThread.idealThreadCount()
        self.tasksPool.setMaxThreadCount(nThreads)
        self.settings = dict.fromkeys(
                RundexSettings.SETTINGS[self.acquisitionMode])
        self._setDefaultSettings()
        self.exportModel = DrillExportModel(self.acquisitionMode)
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

    def getExportModel(self):
        """
        Get the export model.

        Returns:
            DrillExportModel: export model
        """
        return self.exportModel

    def setCycleAndExperiment(self, cycle, experiment):
        """
        Set the cycle number and the experiment ID. This method tries to add the
        usual data directory to the user directories list if it exists.

        Args:
            cycle (str): cycle number
            experiment (str): experiment ID
        """
        self.cycleNumber = cycle
        self.experimentId = experiment

        # platform
        if sys.platform.startswith("linux"):
            baseDir = self.LINUX_BASE_DATA_PATH
        elif sys.platform.startswith("darwin"):
            baseDir = self.MACOS_BASE_DATA_PATH
        elif sys.platform.startswith("win32"):
            baseDir = self.WIN_BASE_DATA_PATH
        else:
            return

        if not os.path.isdir(baseDir):
            return

        # data
        dataDir = os.path.join(baseDir, "{0}/{1}/{2}"
                                        .format(cycle,
                                                self.instrument.lower(),
                                                experiment))
        rawDataDir = os.path.join(dataDir, self.RAW_DATA_DIR)
        processedDataDir = os.path.join(dataDir, self.PROCESSED_DATA_DIR)
        if not os.path.isdir(dataDir):
            logger.warning("Cycle number and experiment ID do not lead to a "
                           "valid directory in the usual data directory ({0}): "
                           "{1} does not exist."
                           .format(baseDir, dataDir))
            return

        # add in user directories if needed
        userDirs = ConfigService.getDataSearchDirs()
        if ((os.path.isdir(rawDataDir)) and (rawDataDir not in userDirs)):
            ConfigService.appendDataSearchDir(rawDataDir)
        if ((os.path.isdir(processedDataDir))
                and (processedDataDir not in userDirs)):
            ConfigService.appendDataSearchDir(processedDataDir)

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
        self.controller = DrillParameterController(self.algorithm)
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
        types = dict()
        values = dict()
        docs = dict()
        if not self.algorithm:
            return types, values, docs

        alg = sapi.AlgorithmManager.createUnmanaged(self.algorithm)
        alg.initialize()
        for s in self.settings:
            p = alg.getProperty(s)
            if (isinstance(p, FileProperty)):
                t = "file"
            elif (isinstance(p, MultipleFileProperty)):
                t = "files"
            elif (isinstance(p, (WorkspaceGroupProperty,
                                 MatrixWorkspaceProperty))):
                t = "workspace"
            elif (isinstance(p, StringPropertyWithValue)):
                if (p.allowedValues):
                    t = "combobox"
                else:
                    t = "string"
            elif (isinstance(p, BoolPropertyWithValue)):
                t = "bool"
            elif (isinstance(p, FloatArrayProperty)):
                t = "floatArray"
            elif (isinstance(p, IntArrayProperty)):
                t = "intArray"
            else:
                t = "string"

            types[s] = t
            values[s] = p.allowedValues
            docs[s] = p.documentation

        return (types, values, docs)

    def _setDefaultSettings(self):
        """
        Set the settings to their defautl values. This method takes the default
        values directly from the algorithm.
        """
        if not self.algorithm:
            return
        alg = sapi.AlgorithmManager.createUnmanaged(self.algorithm)
        alg.initialize()

        for s in self.settings:
            p = alg.getProperty(s)
            v = p.value
            if isinstance(v, numpy.ndarray):
                self.settings[s] = v.tolist()
            elif v is None:
                self.settings[s] = ""
            else:
                self.settings[s] = v

    def _getSamplesFromGroup(self, groupName):
        """
        Get the list of samples that belong to a group.

        Args:
            groupName (str): name of the group

        Returns:
            list(DrillSample): samples that belong to the group
        """
        out = []
        for sample in self.samples:
            if sample.getGroupName() == groupName:
                out.append(sample)
        return out

    def groupSamples(self, sampleIndexes, groupName=None):
        """
        Group samples.

        Args:
            sampleIndexes (list(int)): sample indexes
            groupName (str): optional name for the new group
        """
        def incrementName(name):
            """
            Increment the group name from A to Z, AA to AZ, ...

            Args:
                name (str): group name
            """
            name = list(name)
            if name[-1] == 'Z':
                name[-1] = 'A'
                name += 'A'
            else:
                name[-1] = chr(ord(name[-1]) + 1)
            return ''.join(name)

        if not groupName:
            groupName = 'A'
            while self._getSamplesFromGroup(groupName):
                groupName = incrementName(groupName)

        i = 0
        modifiedGroups = list()
        for s in sampleIndexes:
            sample = self.samples[s]
            currentGroup = sample.getGroupName()
            if currentGroup:
                modifiedGroups.append(currentGroup)
            sample.setGroup(groupName, i)
            i += 1
        for group in modifiedGroups:
            i = 0
            for s in self._getSamplesFromGroup(group):
                s.setGroupName(group, i)
                i += 1

    def ungroupSamples(self, sampleIndexes):
        """
        Ungroup samples.

        Args:
            sampleIndexes (list(int)): sample indexes
        """
        for s in sampleIndexes:
            self.samples[s].setGroup(None)

    def setGroupMaster(self, sampleIndex):
        """
        Set the sample as master for its group.

        Args:
            sampleIndex (int): sample index
        """
        sample = self.samples[sampleIndex]
        groupName = sample.getGroupName()
        for s in self._getSamplesFromGroup(groupName):
            if s.isMaster():
                s.setMaster(False)
        self.samples[sampleIndex].setMaster(True)

    def getProcessingParameters(self, index):
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
        if self.acquisitionMode in RundexSettings.FLAGS:
            params.update(RundexSettings.FLAGS[self.acquisitionMode])
        params.update(self.settings)

        sample = self.samples[index]
        # search for master sample
        groupName = sample.getGroupName()
        if groupName:
            samplesFromGroup = self._getSamplesFromGroup(groupName)
            for s in samplesFromGroup:
                if s.isMaster():
                    params.update(s.getParameterValues())

        params.update(sample.getParameterValues())
        # remove empty params
        for (k, v) in list(params.items()):
            if v is None or v == "DEFAULT":
                del params[k]
        # add the output workspace param
        if "OutputWorkspace" not in params:
            params["OutputWorkspace"] = "sample_" + str(index + 1)
        sample.setOutputName(params["OutputWorkspace"])
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
            tasks.append(DrillTask(str(e), self.algorithm, **kwargs))
        self.tasksPool.addProcesses(tasks)

    def _onTaskStarted(self, ref):
        """
        Called each time a task starts.

        Args:
            ref (int): sample index
        """
        self.samples[ref].onProcessStarted()

    def _onTaskSuccess(self, ref):
        """
        Called when a task finished with success.

        Args:
            ref (int): sample index
        """
        self.samples[ref].onProcessSuccess()
        self.exportModel.run(self.samples[ref])

    def _onTaskError(self, ref, msg):
        """
        Called when a processing fails. This method logs a message and fires the
        corresponding signal.

        Args:
            ref (int): sample index
            msg (str): error msg
        """
        self.samples[ref].onProcessError(msg)

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

    def setIOFile(self, filename):
        """
        Setup the io service on a provided filename.

        Args:
            filename (str): the filename
        """
        self.rundexIO = DrillRundexIO(filename, self)

    def resetIOFile(self):
        """
        Reset the IO service.
        """
        self.rundexIO = None

    def getIOFile(self):
        """
        Get the filename used by the IO service.
        """
        if self.rundexIO:
            return self.rundexIO.getFilename()
        return None

    def importRundexData(self):
        """
        Import data.
        """
        if self.rundexIO:
            self.rundexIO.load()

    def exportRundexData(self):
        """
        Export data.
        """
        if self.rundexIO:
            self.rundexIO.save()

    def setVisualSettings(self, vs):
        """
        Set the visual settings.

        Args:
            vs (dict(str:str)): visual settings
        """
        if vs:
            self.visualSettings = {k:v for k,v in vs.items()}
        else:
            self.visualSettings = dict()

    def getVisualSettings(self):
        """
        Get the saved visual settings

        Returns:
            dict: visual settings that the view understands
        """
        return {k:v for k,v in self.visualSettings.items()}

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

    def addSample(self, index):
        """
        Add a sample to the model.

        Args:
            index (int): sample index; if -1 the sample is added to the end
        """
        if (index == -1) or (index >= len(self.samples)):
            sample = DrillSample(len(self.samples))
            self.samples.append(sample)
        else:
            sample = DrillSample(index)
            self.samples.insert(index, sample)
            i = index + 1
            while i < len(self.samples):
                self.samples[i].setIndex(i)
                i += 1
        sample.setController(self.controller)
        self.newSample.emit(sample)
        return sample

    def deleteSample(self, ref):
        """
        Remove a sample.

        Args:
            ref (int): sample index
        """
        sample = self.samples[ref]
        del self.samples[ref]
        i = ref
        while i < len(self.samples):
            self.samples[i].setIndex(i)
            i += 1

    def getSamples(self):
        """
        Get the list of all the samples.

        Return:
            list(dict(str:str)): samples
        """
        return self.samples

    def getRowsContents(self):
        """
        Get all the samples as a table, 1 sample per row.

        Returns:
            list(list(str)): table contents
        """
        rows = list()
        for sample in self.samples:
            params = sample.getParameters()
            row = list()
            for column in self.columns[:-1]:
                if column in params:
                    row.append(str(params[column]))
                else:
                    row.append("")
            if self.columns[-1] in params:
                options = list()
                for (k, v) in params[self.columns[-1]].items():
                    options.append(str(k) + "=" + str(v))
                row.append(';'.join(options))
            rows.append(row)
        return rows
