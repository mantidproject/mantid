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


class DirectILLAutoPANTHERPowderTest(systemtesting.MantidSystemTest):
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
        self.tolerance = 1e-2
        self.tolerance_is_rel_err = False
        self.nanEqual = True
        self.disableChecking = ["Instrument", "SpectraMap"]
        return ["He3C60", "ILL_PANTHER_Powder_Auto.nxs"]

    def runTest(self):
        EmptyRuns = "9777"
        VanaRuns = "9406"
        SampleRuns = "9738, 9740, 9744"

        EmptyName = "MTCell19meV"
        VanaName = "V19meV"
        SampleName = "He3C60"

        Ei = 19
        Elc = 104

        geometry = {"Shape": "FlatPlate", "Height": 4.0, "Width": 2.0, "Thick": 0.2, "Angle": 0.0, "Center": [0.0, 0.0, 0.0]}

        material = {
            "ChemicalFormula": "C60-(He3)0.80",
            "ZParameter": 4.0,
            "UnitCellVolume": 2744.0,  # =14A**3 for one Cell CFC C60
            "SamplePackingFraction": 0.1,
        }

        DirectILLAutoProcess(
            Runs=EmptyRuns,
            OutputWorkspace=EmptyName,
            ProcessAs="Empty",
            ReductionType="Powder",
            IncidentEnergyCalibration="Energy Calibration ON",
            IncidentEnergy=Ei,
            ElasticChannelIndex=Elc,
            EPPCreationMethod="Calculate EPP",
            SaveOutput=False,
            ClearCache=True,
        )

        # vanadium
        DirectILLAutoProcess(
            Runs=VanaRuns,
            OutputWorkspace=VanaName,
            ProcessAs="Vanadium",
            ReductionType="Powder",
            EmptyContainerWorkspace=EmptyName,
            FlatBkg="Flat Bkg ON",
            IncidentEnergyCalibration="Energy Calibration ON",
            IncidentEnergy=Ei,
            ElasticChannelIndex=Elc,
            EPPCreationMethod="Calculate EPP",
            SaveOutput=False,
            ClearCache=True,
        )

        # sample
        DirectILLAutoProcess(
            Runs=SampleRuns,
            OutputWorkspace=SampleName,
            ProcessAs="Sample",
            ReductionType="Powder",
            VanadiumWorkspace=VanaName,
            FlatBkg="Flat Bkg OFF",
            EmptyContainerWorkspace=EmptyName,
            EPPCreationMethod="Calculate EPP",
            ElasticChannelIndex=Elc,
            MaskedAngles=[0, 20],
            IncidentEnergy=Ei,
            IncidentEnergyCalibration="Energy Calibration ON",
            AbsorptionCorrection="Fast",
            SelfAttenuationMethod="MonteCarlo",
            SampleMaterial=material,
            SampleGeometry=geometry,
            SaveOutput=False,
            ClearCache=True,
        )
