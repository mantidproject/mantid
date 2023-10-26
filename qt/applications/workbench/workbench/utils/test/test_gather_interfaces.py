# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from unittest import TestCase
from unittest.mock import patch, Mock

from workbench.utils.gather_interfaces import gather_interface_names, get_registers_to_run


class GatherInterfacesTest(TestCase):
    @patch("workbench.utils.gather_interfaces.logger")
    @patch("os.path.exists")
    def test_python_interfaces_are_discovered_correctly(self, mock_os_path_exists, _):
        interfaces = ["Muon/Frequency_Domain_Analysis.py", "ILL/DrILL.py"]
        interfaces_str = " ".join(interfaces)  # config service returns them as a whole string.
        mock_os_path_exists.return_value = lambda path: path in interfaces

        with patch("workbench.utils.gather_interfaces.ConfigService", new={"mantidqt.python_interfaces": interfaces_str}):
            returned_interfaces = gather_interface_names("")
            registration_files = get_registers_to_run("")

        expected_interfaces = {"Muon": ["Frequency_Domain_Analysis.py"], "ILL": ["DrILL.py"]}
        self.assertDictEqual(expected_interfaces, returned_interfaces)
        self.assertDictEqual({}, registration_files)

    @patch("workbench.utils.gather_interfaces.logger")
    @patch("os.path.exists")
    def test_that_non_existent_python_interface_is_ignored_gracefully(self, mock_os_path_exists, mock_logger):
        interface_str = "fake/interface.py"
        mock_os_path_exists.return_value = False

        with patch("workbench.utils.gather_interfaces.ConfigService", new={"mantidqt.python_interfaces": interface_str}):
            returned_interfaces = gather_interface_names("")
            registration_files = get_registers_to_run("")

        self.assertDictEqual({}, returned_interfaces)
        self.assertDictEqual({}, registration_files)
        mock_logger.warning.assert_called()

    @patch("workbench.utils.gather_interfaces.UserSubWindowFactory")
    def test_cpp_interfaces_are_discovered_correctly(self, mock_UserSubWindowFactory):
        """Assuming we have already found some python interfaces, test that
        cpp interfaces are discovered correctly using the Direct interfaces as an example."""

        cpp_interface_factory = Mock()
        cpp_interface_factory.keys.return_value = ["ALFView", "TOFCalculator"]
        cpp_interface_factory.categories.side_effect = lambda name: ["Direct"] if name == "ALFView" else []
        mock_UserSubWindowFactory.Instance.return_value = cpp_interface_factory

        with patch("workbench.utils.gather_interfaces._discover_python_interfaces") as mock_python_discovery:
            mock_python_discovery.return_value = {"Direct": ["DGS_Reduction.py", "DGS_Planner.py", "PyChop.py", "MSlice.py"]}
            all_interfaces = gather_interface_names()

        expected_interfaces = {
            "Direct": ["DGS_Reduction.py", "DGS_Planner.py", "PyChop.py", "MSlice.py", "ALFView"],
            "General": ["TOFCalculator"],
        }
        self.assertDictEqual(expected_interfaces, all_interfaces)
