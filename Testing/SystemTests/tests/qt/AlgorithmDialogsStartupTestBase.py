# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod
from systemtesting import MantidSystemTest

from mantid.api import AlgorithmFactory, AlgorithmManager
from mantid.simpleapi import CreateSampleWorkspace

INPUT_WS_NAME = "input_ws"
OUTPUT_WS_NAME = "test_output"
OUTPUT_WS_NAME_IN_ADS = f"{OUTPUT_WS_NAME}_ads"


class AlgorithmDialogsStartupTestBase(MantidSystemTest, metaclass=ABCMeta):
    """
    The base class for a system tests that test the Algorithm Dialogs open ok with different workspaces in the ADS.
    The test will only open the most recent version of an algorithm.
    """

    def __init__(self):
        super(AlgorithmDialogsStartupTestBase, self).__init__()

        algorithm_names = [algorithm.name for algorithm in AlgorithmFactory.Instance().getDescriptors(True, True)]
        # Remove duplicate algorithm names as we will only test the most recent versions
        self._unique_algorithm_names = list(dict.fromkeys(algorithm_names))

        # Create an OutputWorkspace stored in the ADS
        CreateSampleWorkspace(OutputWorkspace=OUTPUT_WS_NAME_IN_ADS)

        self._setup_test()

    @abstractmethod
    def _setup_test(self):
        raise NotImplementedError("This method needs implementing in a subclass of this class.")

    def runTest(self) -> None:
        """Run the test for a random workspace type as a speed-up measure whilst still having a degree of coverage"""
        if len(self._unique_algorithm_names) == 0:
            self.fail("Failed to find any of the Algorithms.")

        print(f"Running the AlgorithmDialog{self._workspace_type}StartupTest with a {self._workspace_type} in the ADS")

        for algorithm_name in self._unique_algorithm_names:
            print(algorithm_name)  # Useful for debugging when an algorithm dialog crashes
            self._attempt_validate_inputs(algorithm_name)

    def _attempt_validate_inputs(self, algorithm_name: str) -> None:
        """Attempt to open the most recent version of the algorithm provided."""
        algorithm = AlgorithmManager.create(algorithm_name)
        # Set the 'OutputWorkspace' property if it exists
        self._set_output_workspace(algorithm)

        # Attempt to set the workspace properties in this list if they exist.
        for property in ["InputWorkspace"]:
            if not self._set_property_value(algorithm, property, INPUT_WS_NAME):
                return

        # If all required properties have been provided, then try validate the inputs
        if algorithm.validateProperties():
            algorithm.validateInputs()

    def _set_output_workspace(self, algorithm) -> None:
        """If the 'OutputWorkspace' property exists, set it because it is usually a required property."""
        # Try to set the OutputWorkspace to an arbitrary string
        if not self._set_property_value(algorithm, "OutputWorkspace", OUTPUT_WS_NAME):
            # If there is an exception, then it might be an InOut property, and so the workspace must exist in the ADS
            algorithm.setPropertyValue("OutputWorkspace", OUTPUT_WS_NAME_IN_ADS)

    @staticmethod
    def _set_property_value(algorithm, property_name, value) -> bool:
        """Attempt to set the value of a property. If there is an exception setting the value, return False."""
        if not algorithm.existsProperty(property_name):
            return True

        try:
            algorithm.setPropertyValue(property_name, value)
        except Exception:
            return False
        return True
