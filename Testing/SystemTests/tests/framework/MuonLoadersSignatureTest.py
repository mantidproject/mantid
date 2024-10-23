# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.api import AlgorithmManager


class MuonLoadersSignatureTest(systemtesting.MantidSystemTest):
    def runTest(self):
        # Check signatures for LoadMuonNexus, LoadMuonNexusV2 and LoadPSIMuonBinData are the same
        load_muon_nexus = AlgorithmManager.create("LoadMuonNexus")
        load_muon_nexus_v2 = AlgorithmManager.create("LoadMuonNexusV2")
        load_psi_muon_bin = AlgorithmManager.create("LoadPSIMuonBin")

        self.assertTrue(load_muon_nexus.existsProperty("DeadTimeTable"))
        self.assertTrue(load_muon_nexus_v2.existsProperty("DeadTimeTable"))
        self.assertTrue(load_psi_muon_bin.existsProperty("DeadTimeTable"))

        self.assertTrue(load_muon_nexus.existsProperty("DetectorGroupingTable"))
        self.assertTrue(load_muon_nexus_v2.existsProperty("DetectorGroupingTable"))
        self.assertTrue(load_psi_muon_bin.existsProperty("DetectorGroupingTable"))

        self.assertTrue(load_muon_nexus.existsProperty("TimeZeroTable"))
        self.assertTrue(load_muon_nexus_v2.existsProperty("TimeZeroTable"))
        self.assertTrue(load_psi_muon_bin.existsProperty("TimeZeroTable"))

        self.assertTrue(load_muon_nexus.existsProperty("MainFieldDirection"))
        self.assertTrue(load_muon_nexus_v2.existsProperty("MainFieldDirection"))
        self.assertTrue(load_psi_muon_bin.existsProperty("MainFieldDirection"))

        self.assertTrue(load_muon_nexus.existsProperty("TimeZero"))
        self.assertTrue(load_muon_nexus_v2.existsProperty("TimeZero"))
        self.assertTrue(load_psi_muon_bin.existsProperty("TImeZero"))

        self.assertTrue(load_muon_nexus.existsProperty("TimeZeroList"))
        self.assertTrue(load_muon_nexus_v2.existsProperty("TimeZeroList"))
        self.assertTrue(load_psi_muon_bin.existsProperty("TimeZeroList"))

        self.assertTrue(load_muon_nexus.existsProperty("FirstGoodData"))
        self.assertTrue(load_muon_nexus_v2.existsProperty("FirstGoodData"))
        self.assertTrue(load_psi_muon_bin.existsProperty("FirstGoodData"))

        self.assertTrue(load_muon_nexus.existsProperty("LastGoodData"))
        self.assertTrue(load_muon_nexus_v2.existsProperty("LastGoodData"))
        self.assertTrue(load_psi_muon_bin.existsProperty("LastGoodData"))
