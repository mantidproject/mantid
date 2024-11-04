# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction
import mantid.simpleapi


class LoadVisionInelastic(PythonAlgorithm):
    __forward = "bank1,bank2,bank3,bank4,bank5,bank6,bank7"
    __backward = "bank8,bank9,bank10,bank11,bank12,bank13,bank14"

    def category(self):
        return "DataHandling\\Nexus"

    def seeAlso(self):
        return ["LoadVisionElasticBS", "LoadVisionElasticEQ"]

    def name(self):
        return "LoadVisionInelastic"

    def summary(self):
        return "This algorithm loads only the inelastic detectors on VISION."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", action=FileAction.Load, extensions=[".nxs.h5"]))
        self.declareProperty("Banks", "all")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output))

    def PyExec(self):
        filename = self.getProperty("Filename").value
        banks = self.getProperty("Banks").value

        # First lets replace 'All' with 'forward,backward'
        banks = banks.lower().replace("all", "forward,backward")
        banks = banks.lower().replace("forward", self.__forward)
        banks = banks.lower().replace("backward", self.__backward)
        banks = banks.lower()

        # Let's make sure we have a unique and sorted list
        banks_integers = banks.replace("bank", "")
        banks_list_integers = banks_integers.split(",")
        banks_list_integers = list(set(banks_list_integers))
        banks_list_integers.sort(key=int)
        banks_list = ["bank{0}".format(i) for i in banks_list_integers]
        banks = ",".join(banks_list)

        wksp_name = "__tmp"
        ws = mantid.simpleapi.LoadEventNexus(Filename=filename, BankName=banks, OutputWorkspace=wksp_name)

        self.setProperty("OutputWorkspace", ws)
        mantid.simpleapi.DeleteWorkspace(wksp_name)


# Register
AlgorithmFactory.subscribe(LoadVisionInelastic)
