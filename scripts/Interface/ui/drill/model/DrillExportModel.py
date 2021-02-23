# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import mtd
from mantid.kernel import config, logger
from mantid.api import WorkspaceGroup

from .configurations import RundexSettings
from .DrillAlgorithmPool import DrillAlgorithmPool
from .DrillTask import DrillTask

import re


class DrillExportModel:

    """
    Dictionnary containing algorithms and activation state.
    """
    _exportAlgorithms = None

    """
    ThreadPool to run export algorithms asynchronously.
    """
    _pool = None

    """
    Dictionnary of all the exports (dict(str:set(str))).
    """
    _exports = None

    """
    Dictionnary of the successful exports (dict(str:set(str))).
    """
    _successExport = None

    def __init__(self, acquisitionMode):
        """
        Create the export model by providing an aquisition mode.

        Args:
            acquisitionMode (str): acquisition mode
        """
        self._exportAlgorithms = {k:v
                for k,v
                in RundexSettings.EXPORT_ALGORITHMS[acquisitionMode].items()}
        self._pool = DrillAlgorithmPool()
        self._pool.signals.taskError.connect(self._onTaskError)
        self._pool.signals.taskSuccess.connect(self._onTaskSuccess)
        self._exports = dict()
        self._successExports = dict()

    def getAlgorithms(self):
        """
        Get the list of export algorithm

        Returns:
            list(str): names of algorithms
        """
        return [algo for algo in self._exportAlgorithms.keys()]

    def isAlgorithmActivated(self, algorithm):
        """
        Get the state of a specific algorithm.

        Args:
            algorithm: name of the algo
        """
        if algorithm in self._exportAlgorithms:
            return self._exportAlgorithms[algorithm]
        else:
            return False

    def activateAlgorithm(self, algorithm):
        """
        Activate a spefific algorithm.

        Args:
            algorithm (str): name of the algo
        """
        if algorithm in self._exportAlgorithms:
            self._exportAlgorithms[algorithm] = True

    def inactivateAlgorithm(self, algorithm):
        """
        Inactivate a specific algorithm.

        Args:
            algorithm (str): name of the algo
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

        processingAlgo = mtd[ws].getHistory().lastAlgorithm()
        try:
            params = re.findall("%[a-zA-Z]*%", criteria)
            for param in params:
                value = processingAlgo.getPropertyValue(param[1:-1])
                criteria = criteria.replace(param, '"' + value + '"')
            return bool(eval(criteria))
        except:
            return False

    def _onTaskSuccess(self, name):
        """
        Triggered when the export finished with success.

        Args:
            name (str): the task name
        """
        name = name.split(':')
        wsName = name[0]
        filename = name[1]

        if wsName not in self._successExports:
            self._successExports[wsName] = set()
        self._successExports[wsName].add(filename)

        if wsName in self._exports:
            self._exports[wsName].discard(filename)
            if not self._exports[wsName]:
                del self._exports[wsName]
                self._logSuccessExport(wsName)

    def _onTaskError(self, name, msg):
        """
        Triggered when the export failed.

        Args:
            name (str): the task name
            msg (str): error msg
        """
        name = name.split(':')
        wsName = name[0]
        filename = name[1]

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
        logger.notice("Successful export of workspace {} to {}"
                      .format(wsName, filenames))
        del self._successExports[wsName]

    def run(self, sample):
        """
        Run the export algorithms on a workspace. If the provided workspace is
        a groupworkspace, the export algorithms will be run on each member of
        the group.

        Args:
            workspaceName (str): name of the workspace
        """
        exportPath = config.getString("defaultsave.directory")
        workspaceName = sample.getOutputName()

        tasks = list()
        for wsName in mtd.getObjectNames():
            if ((workspaceName in wsName)
                    and (not isinstance(mtd[wsName], WorkspaceGroup))):
                for a,s in self._exportAlgorithms.items():
                    if s:
                        if not self._validCriteria(wsName, a):
                            logger.notice("Export of {} with {} was skipped "
                                          "because the workspace is not "
                                          "compatible.".format(wsName, a))
                            continue
                        filename = exportPath + wsName \
                                   + RundexSettings.EXPORT_ALGO_EXTENSION[a]
                        name = wsName + ":" + filename
                        if wsName not in self._exports:
                            self._exports[wsName] = set()
                        self._exports[wsName].add(filename)
                        kwargs = {}
                        if 'Ascii' in a:
                            log_list = (mtd[wsName].getInstrument().getStringParameter('log_list_to_save')[0]).split(',')
                            kwargs['LogList'] = [log.strip() for log in log_list] # removes white spaces
                            if 'Reflectometry' in a:
                                kwargs['WriteHeader'] = True
                                kwargs['FileExtension'] = 'custom'
                        task = DrillTask(name, a, InputWorkspace=wsName,
                                         FileName=filename, **kwargs)
                        tasks.append(task)
        self._pool.addProcesses(tasks)
