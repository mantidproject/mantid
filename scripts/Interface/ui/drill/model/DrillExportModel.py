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

    def _onTaskError(self, ref, msg):
        """
        Triggered when the export failed.

        Args:
            ref (int): task reference
            msg (str): error msg
        """
        sampleName = str(ref)
        logger.error("Error while exporting sample {}.".format(sampleName))
        logger.error(msg)

    def run(self, sample, ref):
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
                    if s and self._validCriteria(wsName, a):
                        filename = exportPath + wsName \
                                   + RundexSettings.EXPORT_ALGO_EXTENSION[a]
                        task = DrillTask(ref, a, InputWorkspace=wsName,
                                         FileName=filename)
                        tasks.append(task)
        self._pool.addProcesses(tasks)
