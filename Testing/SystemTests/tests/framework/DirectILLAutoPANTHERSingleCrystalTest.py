# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import DirectILLAutoProcess, GroupWorkspaces, Load


class DirectILLAutoPANTHERSingleCrystalTest(systemtesting.MantidSystemTest):
    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "PANTHER"
        config.appendDataSearchSubDir("ILL/PANTHER/")

    @classmethod
    def tearDown(cls):
        config["default.facility"] = cls._original_facility
        config["default.instrument"] = cls._original_instrument
        config.setDataSearchDirs(cls._data_search_dirs)

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument", "Sample"]
        return ["panther_236", "ILL_PANTHER_SingleCrystal_Auto.nxs"]

    def runTest(self):
        ei = 19.03
        elp = 104
        Load(Filename="Vnorm012315.nxs", OutputWorkspace="Norm")
        GroupWorkspaces(InputWorkspaces="Norm", OutputWorkspace="NormVana")
        eBins = [-2.0, 0.19, 16]  # inelastic
        group_by = 4

        output_name = "panther_236"
        DirectILLAutoProcess(
            Runs="13806, 13807",
            OutputWorkspace=output_name,
            ProcessAs="Sample",
            ReductionType="SingleCrystal",
            VanadiumWorkspace="NormVana",
            MaskWorkspace="Mask212.xml",
            IncidentEnergyCalibration="Energy Calibration ON",
            IncidentEnergy=ei,
            ElasticChannelIndex=elp,
            EnergyRebinningParams=eBins,
            FlatBkg="Flat Bkg ON",
            GroupDetBy=group_by,
            ClearCache=True,
            SaveOutput=False,
        )
