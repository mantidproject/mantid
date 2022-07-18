# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from systemtesting import MantidSystemTest

from mantid.kernel import ConfigService
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.usersubwindowfactory import UserSubWindowFactory

# These interfaces are excluded for now as they do not have documentation
EXCLUDE_INTERFACES = ["DGS_Reduction", "Elemental_Analysis", "ORNL_SANS", "Powder_Diffraction_Reduction",
                      "Step Scan Analysis"]

# A map of interface name to documentation category and page. We should remove this once the naming of interfaces and
# their documentation page has been harmonized using a naming convention
INTERFACE_DOCS = {"ALC": ("muon", "Muon ALC"),
                  "Bayes": ("indirect", "Indirect Bayes"),
                  "Corrections": ("indirect", "Indirect Corrections"),
                  "Data Analysis": ("indirect", "Inelastic Data Analysis"),
                  "Data Reduction": ("indirect", "Indirect Data Reduction"),
                  "DGSPlanner": ("direct", "DGS Planner"),
                  "Diffraction": ("indirect", "Indirect Diffraction"),
                  "Drill": ("ILL", "DrILL"),
                  "FilterEvents": ("utility", "Filter Events"),
                  "HFIR_4Circle_Reduction": ("diffraction", "HFIR Single Crystal Reduction"),
                  "ISIS_SANS": ("isis_sans", "ISIS SANS"),
                  "Muon_Analysis": ("muon", "Muon Analysis 2"),
                  "Settings": ("indirect", "Indirect Settings"),
                  "Simulation": ("indirect", "Indirect Simulation"),
                  "TofConverter": ("utility", "TOF Converter"),
                  "Tools": ("indirect", "Indirect Tools"),
                  "QECoverage": ("utility", "QE Coverage")}


class HelpDocsExistForInterfacesTest(MantidSystemTest):
    """
    A system test for checking that the help documentation for the python and cpp interfaces can be found successfully.
    """

    def __init__(self):
        super(HelpDocsExistForInterfacesTest, self).__init__()
        self._interface_manager = InterfaceManager()
        self._sub_window_factory = UserSubWindowFactory.Instance()

        python_interfaces = [interface.split(".")[0].split("/")
                             for interface in ConfigService.getString('mantidqt.python_interfaces').split()]
        cpp_interfaces = [[self._sub_window_factory.categories(interface_name).pop(), interface_name]
                          for interface_name in self._sub_window_factory.keys()]

        self._interfaces = python_interfaces + cpp_interfaces

    def runTest(self):
        if len(self._interfaces) == 0:
            self.fail("Failed to find the names of the python interfaces.")

        for category, name in self._interfaces:
            if name not in EXCLUDE_INTERFACES:
                category = INTERFACE_DOCS[name][0] if name in INTERFACE_DOCS else category.lower()
                doc_name = INTERFACE_DOCS[name][1] if name in INTERFACE_DOCS else name.replace("_", " ")
                self._check_interface_documentation_page_exists(category,  doc_name)

    def _check_interface_documentation_page_exists(self, category: str, doc_name: str) -> None:
        """Check that the help documentation page for an interface can be found."""
        url = f"qthelp://org.mantidproject/doc/interfaces/{category}/{doc_name}.html"
        print(url)  # Useful for debugging
        if not self._interface_manager.doesHelpPageExist(url):
            self.fail(f"Missing an interface help page for '{category}/{doc_name}'. Tried the following url:\n {url}")
