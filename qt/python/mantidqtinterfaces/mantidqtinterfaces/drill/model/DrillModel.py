# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import os
import sys

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
from .DrillSampleGroup import DrillSampleGroup


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

    """
    List of processing parameter from the current algorithm.
    """
    _parameters = None

    """
    List of samples.
    """
    _samples = None

    """
    List of sample groups.
    """
    _sampleGroups = None

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

    """
    Sent when instrument and/or acquisition mode changed.
    Args:
        str: name of the instrument
        str: name of the acquisition mode
    """
    newMode = Signal(str, str)

    def __init__(self):
        super(DrillModel, self).__init__()
        self.instrument = None
        self.acquisitionMode = None
        self.cycleNumber = None
        self.experimentId = None
        self.algorithm = None
        self._samples = list()
        self._sampleGroups = dict()
        self.controller = None
        self.visualSettings = dict()
        self.rundexIO = None
        self.exportModel = None

        self.tasksPool = DrillAlgorithmPool()

        # setup the thread pool
        self.tasksPool.signals.progressUpdate.connect(self._onProcessingProgress)
        self.tasksPool.signals.processingDone.connect(self._onProcessingDone)

    def clear(self):
        """
        Clear the sample list and the settings.
        """
        self._samples = list()
        self._sampleGroups = dict()
        self.visualSettings = dict()
        self._initController()
        self._initProcessingParameters()

    def setInstrument(self, instrument, log=True):
        """
        Set the instrument. This methods change the current instrument and all
        the associated parameters (acquisition mode, algorithm, parameters). It
        also empty the sample list and the settings.

        Args:
            instrument (str): instrument name
        """
        self._samples = list()
        self._sampleGroups = dict()
        self.visualSettings = dict()
        self.instrument = None
        self.acquisitionMode = None
        self.algorithm = None
        self.exportModel = None

        # When the user changes the facility after DrILL has been started
        if config["default.facility"] != "ILL":
            logger.error("Drill is enabled only if the facility is set to ILL.")
            return

        if instrument in RundexSettings.ACQUISITION_MODES:
            config["default.instrument"] = instrument
            self.instrument = instrument
            self.setAcquisitionMode(RundexSettings.ACQUISITION_MODES[instrument][0])
        else:
            if log:
                logger.error("Instrument {0} is not supported yet.".format(instrument))

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
        if (self.instrument is None) or (mode not in RundexSettings.ACQUISITION_MODES[self.instrument]):
            return
        self._samples = list()
        self._sampleGroups = dict()
        self.visualSettings = dict()
        if mode in RundexSettings.VISUAL_SETTINGS:
            self.visualSettings = RundexSettings.VISUAL_SETTINGS[mode]
        self.acquisitionMode = mode
        self.algorithm = RundexSettings.ALGORITHM[self.acquisitionMode]
        if self.acquisitionMode in RundexSettings.THREADS_NUMBER:
            nThreads = RundexSettings.THREADS_NUMBER[self.acquisitionMode]
        else:
            nThreads = QThread.idealThreadCount()
        self.tasksPool.setMaxThreadCount(nThreads)
        self.exportModel = DrillExportModel(self.acquisitionMode)
        self._initController()
        self._initProcessingParameters()
        self.newMode.emit(self.instrument, self.acquisitionMode)

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
        if self.instrument is None:
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
        dataDir = os.path.join(baseDir, "{0}/{1}/{2}".format(cycle, self.instrument.lower(), experiment))
        rawDataDir = os.path.join(dataDir, self.RAW_DATA_DIR)
        processedDataDir = os.path.join(dataDir, self.PROCESSED_DATA_DIR)
        if not os.path.isdir(dataDir):
            logger.warning(
                "Cycle number and experiment ID do not lead to a "
                "valid directory in the usual data directory ({0}): "
                "{1} does not exist.".format(baseDir, dataDir)
            )
            return

        # add in user directories if needed
        userDirs = ConfigService.getDataSearchDirs()
        if (os.path.isdir(rawDataDir)) and (rawDataDir not in userDirs):
            ConfigService.appendDataSearchDir(rawDataDir)
        if (os.path.isdir(processedDataDir)) and (processedDataDir not in userDirs):
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
        if not self.algorithm:
            self.controller = None
            return
        self.controller = DrillParameterController(self.algorithm)
        self.controller.start()

    def _initProcessingParameters(self):
        """
        Initialize the processing parameters from the algorithm.
        """
        self._parameters = list()
        if not self.algorithm:
            return
        alg = sapi.AlgorithmManager.createUnmanaged(self.algorithm)
        alg.initialize()

        properties = alg.getProperties()
        for p in properties:
            parameter = DrillParameter(p.name)
            parameter.setController(self.controller)
            parameter.initFromProperty(p)
            self._parameters.append(parameter)

    def getParameters(self):
        """
        Get the parameters of the current algorithm.

        Returns:
            list(DrillParameter): list of all parameters
        """
        return self._parameters

    def getSampleGroups(self):
        """
        Get the sample groups.

        Returns:
            dict(str: DrillSampleGroup): dict of groupName:group
        """
        return self._sampleGroups

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
            if name[-1] == "Z":
                name[-1] = "A"
                name += "A"
            else:
                name[-1] = chr(ord(name[-1]) + 1)
            return "".join(name)

        if not groupName:
            groupName = "A"
            while groupName in self._sampleGroups:
                groupName = incrementName(groupName)

        newGroup = DrillSampleGroup()
        newGroup.setName(groupName)
        self._sampleGroups[groupName] = newGroup

        for i in sampleIndexes:
            sample = self._samples[i]
            currentGroup = sample.getGroup()
            if currentGroup is not None:
                currentGroup.delSample(sample)
                if currentGroup.isEmpty():
                    del self._sampleGroups[currentGroup.getName()]
            newGroup.addSample(sample)

    def ungroupSamples(self, sampleIndexes):
        """
        Ungroup samples.

        Args:
            sampleIndexes (list(int)): sample indexes
        """
        for i in sampleIndexes:
            sample = self._samples[i]
            currentGroup = sample.getGroup()
            if currentGroup is not None:
                currentGroup.delSample(sample)
                if currentGroup.isEmpty():
                    del self._sampleGroups[currentGroup.getName()]

    def addToGroup(self, sampleIndexes, groupName):
        """
        Add some samples to an existing group.

        Args:
            sampleIndexes (list(int)): sample indexes
            groupName (str): name of the group
        """
        if groupName in self._sampleGroups:
            group = self._sampleGroups[groupName]
            for i in sampleIndexes:
                sample = self._samples[i]
                currentGroup = sample.getGroup()
                if currentGroup is not None:
                    currentGroup.delSample(sample)
                group.addSample(sample)

    def setGroupMaster(self, sampleIndex, state):
        """
        Set/unset the sample as master for its group.

        Args:
            sampleIndex (int): sample index
            state (bool): True to set the master sample
        """
        sample = self._samples[sampleIndex]
        group = sample.getGroup()
        if group is not None:
            if state:
                group.setMaster(sample)
            else:
                group.unsetMaster()

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
        for p in self._parameters:
            params[p.getName()] = p.getValue()

        sample = self._samples[index]
        # search for master sample
        group = sample.getGroup()
        if group:
            masterSample = group.getMaster()
            if masterSample:
                params.update(masterSample.getParameterValues())

        params.update(sample.getParameterValues())
        # remove empty params
        for k, v in list(params.items()):
            if v is None or v == "DEFAULT" or v == "":
                del params[k]
        # add the output workspace param
        if "OutputWorkspace" not in params or params["OutputWorkspace"] == "":
            params["OutputWorkspace"] = "sample_" + str(index + 1)
        sample.setOutputName(params["OutputWorkspace"])
        return params

    def processGroupByGroup(self, indexes):
        """
        Create and submit a task per group. Parameter values are appended if the
        configuration says so. Otherwise, the value from the master sample is
        taken.

        Args:
            indexes (list(int)): list of sample indexes
        """
        groups = list()
        for index in indexes:
            sample = self._samples[index]
            group = sample.getGroup()
            if group is not None and group not in groups:
                groups.append(group)
        tasks = list()
        if not groups:
            return False
        for group in groups:
            processingParams = dict()
            samples = group.getSamples()
            master = group.getMaster()
            for sample in samples:
                sParameters = sample.getParameterValues()
                for name, value in sParameters.items():
                    if name in RundexSettings.GROUPED_COLUMNS[self.acquisitionMode]:
                        if name in processingParams:
                            processingParams[name] += "," + str(value)
                        else:
                            processingParams[name] = str(value)
                    elif sample == master:
                        processingParams[name] = value
                    elif master is None:
                        processingParams[name] = value
            for p in self._parameters:
                if p.getName() not in processingParams:
                    processingParams[p.getName()] = p.getValue()
            task = DrillTask(group.getName(), self.algorithm, **processingParams)
            for sample in samples:
                task.addStartedCallback(sample.onProcessStarted)
                task.addSuccessCallback(sample.onProcessSuccess)
                task.addErrorCallback(sample.onProcessError)
                if sample == master:
                    # needed for the auto export
                    # here we need to export only one sample per group
                    sample.setOutputName(processingParams["OutputWorkspace"])

            tasks.append(task)
        self.tasksPool.addProcesses(tasks)
        return True

    def process(self, elements):
        """
        Start samples processing. The method first checks that all the samples
        are valid and if yes, submit them to the processing.

        Args:
            elements (list(int)): list of sample indexes to be processed

        Returns:
            bool: True if all samples are valid and submitted to processing
        """
        tasks = list()
        for e in elements:
            if (e >= len(self._samples)) or (not self._samples[e]):
                continue
            if not self._samples[e].isValid():
                return False
            kwargs = self.getProcessingParameters(e)
            task = DrillTask(str(e), self.algorithm, **kwargs)
            task.addStartedCallback(self._samples[e].onProcessStarted)
            task.addSuccessCallback(self._samples[e].onProcessSuccess)
            task.addErrorCallback(self._samples[e].onProcessError)
            tasks.append(task)
        self.tasksPool.addProcesses(tasks)
        return True

    def processGroup(self, elements):
        """
        Start processing of whole group(s) of samples.

        Args:
            elements (list(int)): list of sample indexes

        Returns:
            bool: True if all samples are valid and submitted to processing
        """
        sampleIndexes = []
        for e in elements:
            if (e >= len(self._samples)) or (not self._samples[e]):
                continue
            group = self._samples[e].getGroup()
            if group is not None:
                sampleIndexes += [sample.getIndex() for sample in group.getSamples()]
        return self.process(sampleIndexes)

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
            self.visualSettings = {k: v for k, v in vs.items()}
        else:
            self.visualSettings = dict()

    def getVisualSettings(self):
        """
        Get the saved visual settings

        Returns:
            dict: visual settings that the view understands
        """
        return {k: v for k, v in self.visualSettings.items()}

    def addSample(self, index):
        """
        Add a sample to the model.

        Args:
            index (int): sample index; if -1 the sample is added to the end
        """
        if (index == -1) or (index >= len(self._samples)):
            sample = DrillSample(len(self._samples))
            self._samples.append(sample)
        else:
            sample = DrillSample(index)
            self._samples.insert(index, sample)
            i = index + 1
            while i < len(self._samples):
                self._samples[i].setIndex(i)
                i += 1
        sample.setController(self.controller)
        sample.setExporter(self.exportModel)
        self.newSample.emit(sample)
        return sample

    def deleteSample(self, ref):
        """
        Remove a sample.

        Args:
            ref (int): sample index
        """
        group = self._samples[ref].getGroup()
        if group:
            group.delSample(self._samples[ref])
            if group.isEmpty():
                del self._sampleGroups[group.getName()]
        del self._samples[ref]
        i = ref
        while i < len(self._samples):
            self._samples[i].setIndex(i)
            i += 1

    def getSamples(self):
        """
        Get the list of all the samples.

        Return:
            list(dict(str:str)): samples
        """
        return self._samples
