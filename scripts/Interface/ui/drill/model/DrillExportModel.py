# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import mtd
from mantid.kernel import config
from mantid.api import WorkspaceGroup

from .configurations import RundexSettings
from .DrillAlgorithmPool import DrillAlgorithmPool
from .DrillTask import DrillTask


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

    def run(self, workspaceName):
        """
        Run the export algorithms on a workspace. If the provided workspace is
        a groupworkspace, the export algorithms will be run on each member of
        the group.

        Args:
            workspaceName (str): name of the workspace
        """
        exportPath = config.getString("defaultsave.directory")
        tasks = list()
        ws = mtd[workspaceName]
        if isinstance(ws, WorkspaceGroup):
            workspaceNames = ws.getNames()
        else:
            workspaceNames = [workspaceName]
        for name in workspaceNames:
            for a,s in self._exportAlgorithms.items():
                if s:
                    filename = exportPath + name
                    task = DrillTask(-1, a, InputWorkspace=name,
                                     FileName=filename)
                    tasks.append(task)
        self._pool.addProcesses(tasks)
