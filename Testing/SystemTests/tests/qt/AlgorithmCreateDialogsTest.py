# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from systemtesting import MantidSystemTest

from mantid.api import AlgorithmFactory
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.interfacemanager import InterfaceManager


class AlgorithmCreateDialogsTest(MantidSystemTest):
    """
    A system test which tests the Algorithm dialogs can be created successfully.
    The test will only createDialogFromName for the most recent version of an algorithm.
    """

    def __init__(self):
        super(AlgorithmCreateDialogsTest, self).__init__()

        algorithm_names = [algorithm.name for algorithm in AlgorithmFactory.Instance().getDescriptors(True, True)]
        # Remove duplicate algorithm names as we will only test the most recent versions
        self._unique_algorithm_names = list(dict.fromkeys(algorithm_names))
        CreateSampleWorkspace(OutputWorkspace="name")

    def runTest(self) -> None:
        """Run the test for the provided workspace type in the ADS."""
        if len(self._unique_algorithm_names) == 0:
            self.fail("Failed to find any of the Algorithms.")

        for algorithm_name in self._unique_algorithm_names:
            print(algorithm_name)  # Useful for debugging
            presets = {"InputWorkspace": "1", "OutputWorkspace": "out"}
            manager = InterfaceManager()
            manager.createDialogFromName(algorithm_name, -1, None, False, presets, "")