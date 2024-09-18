# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from typing import Dict, List

from DocumentationTest import PATH_TO_DOCS, DocumentationTestBase
from mantid.kernel import ConfigService
from workbench.utils.gather_interfaces import gather_interface_names

INTERFACES_DOCS_PATH = PATH_TO_DOCS / "interfaces"

# This should be empty
INTERFACES_WITH_NO_DOCS = [
    "Shiver.rst",  # externally imported
    "SNAPRed.rst",  # externally imported
    "ORNL SANS.rst",  # information is in concepts/ORNL_SANS_Reduction.rst
    "Powder Diffraction Reduction.rst",  # reduction application
    "DGS Reduction.rst",  # reduction application
    "HFIR 4Circle Reduction.rst",
    "Garnet.rst",  # externally imported
]


def _get_interface_names() -> Dict[str, List[str]]:
    interfaces = gather_interface_names()
    hidden_interfaces = ConfigService["interfaces.categories.hidden"].split(";")
    interface_names = {key: names for key, names in interfaces.items() if key not in hidden_interfaces}
    return interface_names


class InterfaceDocumentationTest(DocumentationTestBase):
    """
    This test checks that all interfaces have a documentation page
    """

    def _setup_test(self):
        self._known_no_docs = INTERFACES_WITH_NO_DOCS
        self._docs_path = INTERFACES_DOCS_PATH
        for category, interfaces in _get_interface_names().items():
            for interface_name in interfaces:
                if interface_name.endswith(".py"):
                    interface_name = interface_name.replace(".py", "").replace("_", " ")

                self._files.append([f"{category} {interface_name}.rst", f"{interface_name}.rst"])

        self._test_type = "Interface"
