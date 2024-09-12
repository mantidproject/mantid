# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import mtd, AlgorithmManager
from mantid.kernel import config, logger
from mantid.api import WorkspaceGroup

from .configurations import RundexSettings
from .DrillAlgorithmPool import DrillAlgorithmPool
from .DrillTask import DrillTask

import re
import os


class DrillExportModel:
    """
    Dictionary containing algorithm (name, extension) tuples and activation
    state.
    """

    _exportAlgorithms = None

    """
    Dictionary of export algorithm short doc.
    """
    _exportDocs = None

    """
    ThreadPool to run export algorithms asynchronously.
    """
    _pool = None

    """
    Dictionary of all the exports (dict(str:set(str))).
    """
    _exports = None

    """
    Dictionary of the successful exports (dict(str:set(str))).
    """
    _successExport = None

    def __init__(self, acquisitionMode):
        """
        Create the export model by providing an aquisition mode.

        Args:
            acquisitionMode (str): acquisition mode
        """
        self._exportAlgorithms = {k: v for k, v in RundexSettings.EXPORT_ALGORITHMS[acquisitionMode].items()}
        self._exportDocs = dict()
        for a, _ in self._exportAlgorithms.keys():
            try:
                alg = AlgorithmManager.createUnmanaged(a)
                self._exportDocs[a] = alg.summary()
            except:
                pass
        self._pool = DrillAlgorithmPool()
        self._exports = dict()
        self._successExports = dict()

    def getAlgorithms(self):
        """
        Get the list of export algorithm

        Returns:
            list(str): names of algorithms
        """
        return [(algo, ext) for (algo, ext) in self._exportAlgorithms.keys()]

    def getAlgorithmDocs(self):
        """
        Get the short documentation of each export algorithm.

        Return:
            dict(str:str): dictionary algo:doc
        """
        return {k: v for k, v in self._exportDocs.items()}

    def isAlgorithmActivated(self, algorithm):
        """
        Get the state of a specific algorithm.

        Args:
            algorithm (str, str): name of the algo and output file extension
        """
        if algorithm in self._exportAlgorithms:
            return self._exportAlgorithms[algorithm]
        else:
            return False

    def activateAlgorithm(self, algorithm):
        """
        Activate a specific algorithm.

        Args:
            algorithm list(str, str): name of the algo and output file extension
        """
        algorithm = tuple(algorithm)  # tuples are hashable and can be a key in a dictionary _exportAlgorithms
        if algorithm in self._exportAlgorithms:
            self._exportAlgorithms[algorithm] = True

    def inactivateAlgorithm(self, algorithm):
        """
        Inactivate a specific algorithm.

        Args:
            algorithm (str, str): name of the algo and output file extension
        """
        if algorithm in self._exportAlgorithms:
            self._exportAlgorithms[algorithm] = False

    def _validCriteria(self, ws, algo):
        """
        Check if the criteria of the export algorithm is valid ot not.

        Args:
            ws (name): name of the workspace on which the critaeria will be
                       tested
            algo (str): name of the export algorithm
        """
        if algo not in RundexSettings.EXPORT_ALGO_CRITERIA:
            return True
        criteria = RundexSettings.EXPORT_ALGO_CRITERIA[algo]
        if not criteria:
            return True

        try:
            processingAlgo = mtd[ws].getHistory().lastAlgorithm()
            params = re.findall("%[a-zA-Z]*%", criteria)
            for param in params:
                value = processingAlgo.getPropertyValue(param[1:-1])
                criteria = criteria.replace(param, '"' + value + '"')
            return bool(eval(criteria))
        except:
            return False

    def _onTaskSuccess(self, wsName, filename):
        """
        Triggered when the export finished with success.

        Args:
            wsName (str): name of the exported workspace
            filename (str): name of the file
        """
        if wsName not in self._successExports:
            self._successExports[wsName] = set()
        self._successExports[wsName].add(filename)

        if wsName in self._exports:
            self._exports[wsName].discard(filename)
            if not self._exports[wsName]:
                del self._exports[wsName]
                self._logSuccessExport(wsName)

    def _onTaskError(self, wsName, filename, msg):
        """
        Triggered when the export failed.

        Args:
            wsName (str): name of the exported workspace
            filename (str): name of the file
            msg (str): error msg
        """
        logger.error("Error while exporting workspace {}.".format(wsName))
        logger.error(msg)

        if wsName in self._exports:
            self._exports[wsName].discard(filename)
            if not self._exports[wsName]:
                del self._exports[wsName]
                self._logSuccessExport(wsName)

    def _logSuccessExport(self, wsName):
        """
        Log all the successful exports.

        Args:
            wsName (str): name of the concerned workspace
        """
        if wsName not in self._successExports:
            return
        filenames = ", ".join(self._successExports[wsName])
        logger.notice("Successful export of workspace {} to {}".format(wsName, filenames))
        del self._successExports[wsName]

    def run(self, sample):
        """
        Run the export algorithms on a sample. For each export algorithm, the
        function will try to validate the criteria (using _validCriteria()) on
        the output workspace that corresponds to the sample. If the criteria are
        valid, the export will be run on all workspaces whose name contains the
        sample name.

        Args:
            sample (DrillSample): sample to be exported
        """
        exportPath = config.getString("defaultsave.directory")
        if not exportPath:
            logger.warning(
                "Default save directory is not defined. Please " "specify one in the data directories dialog to " "enable exports."
            )
            return
        workspaceName = sample.getOutputName()

        try:
            outputWs = mtd[workspaceName]
            if isinstance(outputWs, WorkspaceGroup):
                names = outputWs.getNames()
                outputWs = names[0]
            else:
                outputWs = workspaceName
        except:
            return

        tasks = list()
        for (algo, ext), active in self._exportAlgorithms.items():
            if not active:
                continue
            if not self._validCriteria(outputWs, algo):
                logger.notice("Export of sample {} with {} was skipped " "because workspaces are not compatible.".format(outputWs, algo))
                continue

            for wsName in mtd.getObjectNames(contain=workspaceName):
                if isinstance(mtd[wsName], WorkspaceGroup):
                    continue
                filename = os.path.join(exportPath, wsName + ext)
                name = wsName + ":" + filename
                if wsName not in self._exports:
                    self._exports[wsName] = set()
                self._exports[wsName].add(filename)
                kwargs = {}
                if "Ascii" in algo:
                    log_list = mtd[wsName].getInstrument().getStringParameter("log_list_to_save")
                    if log_list:
                        log_list = log_list[0].split(",")
                        kwargs["LogList"] = [log.strip() for log in log_list]  # removes white spaces
                    if "Reflectometry" in algo:
                        kwargs["WriteHeader"] = True
                        kwargs["FileExtension"] = ext
                        kwargs["Theta"] = mtd[wsName].getRun().getProperty("san.value")
                    else:
                        kwargs["WriteXError"] = True
                task = DrillTask(name, algo, InputWorkspace=wsName, FileName=filename, **kwargs)
                task.addSuccessCallback(lambda wsName=wsName, filename=filename: self._onTaskSuccess(wsName, filename))
                task.addErrorCallback(lambda msg, wsName=wsName, filename=filename: self._onTaskError(wsName, filename, msg))
                tasks.append(task)

        self._pool.addProcesses(tasks)
