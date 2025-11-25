# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod
from systemtesting import MantidSystemTest

from mantid.api import AlgorithmFactory, AlgorithmManager

INPUT_WS_NAME = "input_ws"


class AlgorithmValidateInputsTestBase(MantidSystemTest, metaclass=ABCMeta):
    """
    The base class for a system test which tests the Algorithm validateInputs method for a given workspace in the ADS.
    The test will only validateInputs for the most recent version of an algorithm.
    """

    def __init__(self):
        super(AlgorithmValidateInputsTestBase, self).__init__()

        algorithm_names = [algorithm.name for algorithm in AlgorithmFactory.Instance().getDescriptors(True, True)]
        # Remove duplicate algorithm names as we will only test the most recent versions
        self._unique_algorithm_names = list(dict.fromkeys(algorithm_names))

        self._setup_test()

    @abstractmethod
    def _setup_test(self):
        raise NotImplementedError("This method needs implementing in a subclass of this class.")

    def runTest(self) -> None:
        """Run the test for the provided workspace type in the ADS."""
        if len(self._unique_algorithm_names) == 0:
            self.fail("Failed to find any of the Algorithms.")

        print(f"Running the Algorithm{self._workspace_type}ValidateInputsTest with a {self._workspace_type} in the ADS")

        for algorithm_name in self._unique_algorithm_names:
            print(algorithm_name)  # Useful for debugging
            self._attempt_validate_inputs(algorithm_name)

    def _attempt_validate_inputs(self, algorithm_name: str) -> None:
        """Attempt to validate the inputs for the most recent version of the algorithm provided."""
        algorithm = AlgorithmManager.create(algorithm_name)

        properties_valid = self._set_all_workspace_properties(algorithm)
        if not properties_valid:
            return

        try:
            # This is the real test - does the validateInputs function run successfully
            algorithm.validateInputs()
        except Exception as ex:
            # Makes sure that an exception gets printed if it fails, and then re-raises the exception
            print(f"Exception for algorithm {algorithm_name}: {ex}")
            raise

    @staticmethod
    def _set_all_workspace_properties(algorithm) -> bool:
        """
        Attempt to set all algorithm properties which have the word "Workspace" in their name. They are set to
        the name of the workspace stored in the ADS. If setting the property fails, then ignore the property.
        Returns True if all properties are valid at the end of setting each property.
        """
        for property in algorithm.getProperties():
            property_name = property.name
            if "Workspace" in property_name:
                try:
                    algorithm.setPropertyValue(property_name, INPUT_WS_NAME)
                except Exception:
                    pass
            # If the property is not valid, return False
            if property.isValid != "":
                return False
        return True
