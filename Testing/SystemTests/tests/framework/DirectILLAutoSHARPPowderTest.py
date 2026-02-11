# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import DirectILLAutoProcess


class DirectILLAutoSHARPPowderTest(systemtesting.MantidSystemTest):
    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "SHARP"
        config.appendDataSearchSubDir("ILL/SHARP/")

    @classmethod
    def tearDown(cls):
        config["default.facility"] = cls._original_facility
        config["default.instrument"] = cls._original_instrument
        config.setDataSearchDirs(cls._data_search_dirs)

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-2
        self.tolerance_is_rel_err = True
        self.nanEqual = True
        self.disableChecking = ["Instrument", "SpectraMap"]
        return ["PEO", "ILL_SHARP_Powder_Auto.nxs"]

    def runTest(self):
        ebins = "-5, 0.01, 2"
        vana_runs = "3861-3862"

        vana_geometry = {"Shape": "HollowCylinder", "OuterRadius": 0.75, "InnerRadius": 0.74, "Height": 5.0, "Center": [0.0, 0.0, 0.0]}
        vana_material = {"ChemicalFormula": "V", "SampleNumberDensity": 0.07}

        ei = 3.1695349054327178
        elp = 559.921

        DirectILLAutoProcess(
            Runs=vana_runs,
            OutputWorkspace="Vana_auto",
            ProcessAs="Vanadium",
            ReductionType="Powder",
            FlatBkg="Flat Bkg ON",
            GroupDetHorizontallyBy="6",
            GroupDetVerticallyBy="80",
            FlatBkgScaling=1.5,
            IncidentEnergyCalibration="Energy Calibration ON",
            IncidentEnergy=ei,
            ElasticChannelIndex=elp,
            EnergyRebinningParams=ebins,
            SampleMaterial=vana_material,
            SampleGeometry=vana_geometry,
            AbsorptionCorrection="Fast",
            SaveOutput=False,
            ClearCache=True,
        )

        sample = "PEO"
        sample_run = "5910-5911"

        DirectILLAutoProcess(
            Runs=sample_run,
            OutputWorkspace=sample,
            ProcessAs="Sample",
            ReductionType="Powder",
            VanadiumWorkspace="Vana_auto",
            ElasticChannelIndex=elp,
            FlatBkg="Flat Bkg ON",
            GroupDetHorizontallyBy="2",
            GroupDetVerticallyBy="16",
            MaskWithVanadium=True,
            IncidentEnergyCalibration="Energy Calibration ON",
            IncidentEnergy=ei,
            AbsorptionCorrection="None",
            EnergyRebinningParams=ebins,
            SaveOutput=False,
            ClearCache=True,
        )
