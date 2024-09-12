# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import *


class DirectILLAuto_PANTHER_Powder_Test(systemtesting.MantidSystemTest):
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


class DirectILLAuto_PANTHER_SingleCrystal_Test(systemtesting.MantidSystemTest):
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


class DirectILLAuto_SHARP_Powder_Test(systemtesting.MantidSystemTest):
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
