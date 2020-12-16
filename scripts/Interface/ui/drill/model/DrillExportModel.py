# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from .configurations import RundexSettings


class DrillExportModel:

    """
    Dictionnary containing algorithms and activation state.
    """
    _exportAlgorithms = None

    def __init__(self, acquisitionMode):
        """
        Create the export model by providing an aquisition mode.

        Args:
            acquisitionMode (str): acquisition mode
        """
        self._exportAlgorithms = {k:v
                for k,v
                in RundexSettings.EXPORT_ALGORITHMS[acquisitionMode].items()}

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
