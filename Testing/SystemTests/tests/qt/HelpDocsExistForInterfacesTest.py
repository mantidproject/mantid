# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from systemtesting import MantidSystemTest

from mantid.kernel import ConfigService
from mantidqt.interfacemanager import InterfaceManager
#from mantidqt.usersubwindowfactory import UserSubWindowFactory
from mantidqt.utils.qt.testing import get_application

from qtpy.QtWidgets import QWidget

# These interfaces are excluded for now as they do not have documentation
EXCLUDE_INTERFACES = ["DGS_Reduction", "Elemental_Analysis", "ORNL_SANS", "Powder_Diffraction_Reduction"]

# A map of interface name to documentation category and page. We should remove this once the naming of interfaces and
# their documentation page has been harmonized using a naming convention
INTERFACE_DOCS = {"DGSPlanner": ("direct", "DGS Planner"),
                  "Drill": ("ILL", "DrILL"),
                  "FilterEvents": ("utility", "Filter Events"),
                  "HFIR_4Circle_Reduction": ("diffraction", "HFIR Single Crystal Reduction"),
                  "ISIS_SANS": ("isis_sans", "ISIS SANS"),
                  "TofConverter": ("utility", "TOF Converter"),
                  "QECoverage": ("utility", "QE Coverage")}


class HelpDocsExistForInterfacesTest(MantidSystemTest):
    """
    A system test for checking that the help documentation for the python and cpp interfaces can be found successfully.
    """

    def __init__(self):
        super(HelpDocsExistForInterfacesTest, self).__init__()
        python_interfaces = [interface.split(".")[0].split("/")
                             for interface in ConfigService.getString('mantidqt.python_interfaces').split()]
        cpp_interfaces = []

        self._interface_manager = InterfaceManager()
        self._interfaces = python_interfaces + cpp_interfaces
        #self._cpp_interfaces = UserSubWindowFactory.Instance().keys()

    def runTest(self):
        if len(self._interfaces) == 0:
            self.fail("Failed to find the names of the python interfaces.")

        self._qapp = get_application()

        for category, name in self._interfaces:
            if name not in EXCLUDE_INTERFACES:
                category = INTERFACE_DOCS[name][0] if name in INTERFACE_DOCS else category.lower()
                doc_name = INTERFACE_DOCS[name][1] if name in INTERFACE_DOCS else name.replace("_", " ")
                self._attempt_to_open_python_help_window(category,  doc_name)
        self._qapp = None

    def _attempt_to_open_python_help_window(self, category: str, doc_name: str) -> None:
        """Attempt to open the help documentation for a python interface."""
        url = f"qthelp://org.mantidproject/doc/interfaces/{category}/{doc_name}.html"
        print(url)  # Useful for debugging
        parent = QWidget()

        self._interface_manager.showHelpPage(url, parent)
        if not self._interface_manager.isHelpPageFound(url):
            self.fail(f"Missing an interface help page for '{category}/{doc_name}'. Tried the following url:\n {url}")

        parent.close()
        self._qapp.closeAllWindows()

    def _attempt_to_open_cpp_help_window(self, interface_name):
        """Attempt to open the help documentation for a cpp interface."""
        print(interface_name)
        interface = self._interface_manager.createSubWindow(interface_name)
        self._interface_manager.showCustomInterfaceHelp(interface_name, interface.categoryInfo())
