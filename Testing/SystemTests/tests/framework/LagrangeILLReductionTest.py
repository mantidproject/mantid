# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import LagrangeILLReduction


class ILL_Lagrange_ASCII_Test(systemtesting.MantidSystemTest):
    _facility = None
    _instrument = None
    _data_search_dirs = None

    def __init__(self):
        super(ILL_Lagrange_ASCII_Test, self).__init__()
        self.setUp()

    def setUp(self):
        self._facility = config["default.facility"]
        self._instrument = config["default.instrument"]
        self._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "Lagrange"
        config.appendDataSearchSubDir("ILL/LAGRANGE/")

    def cleanup(self):
        mtd.clear()
        config["default.facility"] = self._facility
        config["default.instrument"] = self._instrument
        config.setDataSearchDirs(self._data_search_dirs)

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        return ["lagrange_ascii", "ILL_LAGRANGE_ASCII.nxs"]

    def runTest(self):
        LagrangeILLReduction(
            SampleRuns="012869:012871",
            ContainerRuns="012882:012884",
            CorrectionFile="correction-water-cu220-2020.txt",
            OutputWorkspace="lagrange_ascii",
            UseIncidentEnergy=True,
            NexusInput=False,
        )
