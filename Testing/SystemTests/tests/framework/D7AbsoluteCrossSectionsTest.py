# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import mtd
from mantid.kernel import config
from mantid.simpleapi import AppendSpectra, D7AbsoluteCrossSections, PolDiffILLReduction, RenameWorkspace
import numpy as np


class ILL_D7_Powder_Test(systemtesting.MantidSystemTest):
    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D7"
        config.appendDataSearchSubDir("ILL/D7/")

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
        self.disableChecking = ["Instrument"]
        return ["normalized_powder_XYZ", "ILL_D7_Powder_XYZ.nxs"]

    def runTest(self):
        vanadium_mass = 8.535
        sample_formula_mass = 137.33 * 2.0 + 54.93 + 127.6 + 15.999 * 6.0
        sample_mass = 7.83

        vanadium_dictionary = {
            "SampleMass": vanadium_mass,
            "FormulaUnitMass": 50.9412,
            "SampleChemicalFormula": "V",
            "SampleDensity": 6.1,
            "Height": 1.0,
            "ElementSize": 0.5,
            "SampleInnerRadius": 0.91,
            "SampleOuterRadius": 0.99,
            "ContainerChemicalFormula": "Al",
            "ContainerDensity": 2.7,
            "ContainerOuterRadius": 1.0,
            "ContainerInnerRadius": 0.9,
        }

        sample_dictionary = {"SampleMass": sample_mass, "FormulaUnitMass": sample_formula_mass}

        calibration_file = "D7_YIG_calibration.xml"

        # Empty container
        PolDiffILLReduction(Run="450747:450748", OutputWorkspace="container_ws", ProcessAs="Empty")

        # Absorber
        PolDiffILLReduction(Run="450758:450759", OutputWorkspace="absorber_ws", ProcessAs="Cadmium")

        PolDiffILLReduction(
            Run="450769:450770",
            OutputWorkspace="pol_corrections",
            CadmiumWorkspace="absorber_ws",
            EmptyContainerWorkspace="container_ws",
            Transmission="0.9",
            OutputTreatment="AveragePol",
            ProcessAs="Quartz",
        )

        PolDiffILLReduction(
            Run="450835:450836",
            OutputWorkspace="vanadium_ws",
            CadmiumWorkspace="absorber_ws",
            EmptyContainerWorkspace="container_ws",
            Transmission="0.89",
            QuartzWorkspace="pol_corrections",
            OutputTreatment="Sum",
            SampleGeometry="None",
            SampleAndEnvironmentProperties=vanadium_dictionary,
            AbsoluteNormalisation=True,
            InstrumentCalibration=calibration_file,
            ProcessAs="Vanadium",
        )

        PolDiffILLReduction(
            Run="451235:451236",
            OutputWorkspace="sample_ws",
            CadmiumWorkspace="absorber_ws",
            EmptyContainerWorkspace="container_ws",
            Transmission="0.91",
            QuartzWorkspace="pol_corrections",
            SelfAttenuationMethod="Transmission",
            OutputTreatment="Individual",
            SampleGeometry="None",
            SampleAndEnvironmentProperties=sample_dictionary,
            InstrumentCalibration=calibration_file,
            ProcessAs="Sample",
        )

        D7AbsoluteCrossSections(
            InputWorkspace="sample_ws",
            OutputWorkspace="normalized_powder_XYZ",
            CrossSectionSeparationMethod="XYZ",
            NormalisationMethod="Vanadium",
            VanadiumInputWorkspace="vanadium_ws",
            OutputTreatment="Merge",
            OutputUnits="TwoTheta",
            ScatteringAngleBinSize=1.5,  # degrees
            SampleAndEnvironmentProperties=sample_dictionary,
            AbsoluteUnitsNormalisation=True,
            ClearCache=True,
        )


class ILL_D7_SingleCrystal_Test(systemtesting.MantidSystemTest):
    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D7"
        config.appendDataSearchSubDir("ILL/D7/")

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
        self.disableChecking = ["Instrument"]
        return ["normalized_single_crystal_XYZ", "ILL_D7_SingleCrystal_XYZ.nxs"]

    def runTest(self):
        vanadium_mass = 8.535
        sample_formula_mass = 137.33 * 2.0 + 54.93 + 127.6 + 15.999 * 6.0
        sample_mass = 7.83
        vanadium_dictionary = {"SampleMass": vanadium_mass, "FormulaUnits": 1, "FormulaUnitMass": 50.942}
        sample_dictionary = {
            "SampleMass": sample_mass,
            "FormulaUnits": 1,
            "FormulaUnitMass": sample_formula_mass,
            "KiXAngle": 45.0,
            "OmegaShift": 52.5,
        }
        calibration_file = "D7_YIG_calibration.xml"

        # Empty container for quartz and vanadium
        PolDiffILLReduction(Run="450747:450748", OutputWorkspace="container_ws", ProcessAs="Empty")

        # Empty container for bank position 1 (bt1), tth=79.5
        PolDiffILLReduction(Run="397406:397407", OutputTreatment="AveragePol", OutputWorkspace="container_bt1_ws", ProcessAs="Empty")
        # empty container for bt2, tth=75
        PolDiffILLReduction(Run="397397:397398", OutputTreatment="AveragePol", OutputWorkspace="container_bt2_ws", ProcessAs="Empty")

        PolDiffILLReduction(
            Run="450769:450770",
            OutputWorkspace="pol_corrections",
            EmptyContainerWorkspace="container_ws",
            Transmission="0.9",
            OutputTreatment="AveragePol",
            ProcessAs="Quartz",
        )

        PolDiffILLReduction(
            Run="450835:450836",
            OutputWorkspace="vanadium_ws",
            EmptyContainerWorkspace="container_ws",
            Transmission="0.89",
            QuartzWorkspace="pol_corrections",
            OutputTreatment="Sum",
            SampleGeometry="None",
            SelfAttenuationMethod="Transmission",
            SampleAndEnvironmentProperties=vanadium_dictionary,
            AbsoluteNormalisation=True,
            InstrumentCalibration=calibration_file,
            ProcessAs="Vanadium",
        )

        # bank position 1, tth=79.5
        PolDiffILLReduction(
            Run="399451:399452",
            OutputWorkspace="bt1",
            EmptyContainerWorkspace="container_bt1_ws",
            Transmission="0.95",
            QuartzWorkspace="pol_corrections",
            OutputTreatment="Individual",
            SampleGeometry="None",
            SampleAndEnvironmentProperties=sample_dictionary,
            MeasurementTechnique="SingleCrystal",
            InstrumentCalibration=calibration_file,
            ProcessAs="Sample",
        )
        # bank position 2, tth=75
        PolDiffILLReduction(
            Run="400287:400288",
            OutputWorkspace="bt2",
            EmptyContainerWorkspace="container_bt2_ws",
            Transmission="0.95",
            QuartzWorkspace="pol_corrections",
            OutputTreatment="Individual",
            SampleGeometry="None",
            SampleAndEnvironmentProperties=sample_dictionary,
            MeasurementTechnique="SingleCrystal",
            InstrumentCalibration=calibration_file,
            ProcessAs="Sample",
        )
        appended_ws = "appended_ws"
        AppendSpectra(InputWorkspace1="bt1", InputWorkspace2="bt2", OutputWorkspace=appended_ws)
        # names need to be re-set, AppendSpectra just concatenates them
        possible_polarisations = ["ZPO_ON", "ZPO_OFF", "XPO_ON", "XPO_OFF", "YPO_ON", "YPO_OFF"]
        polarisation = ""
        for entry in mtd[appended_ws]:
            entry_name = entry.name()
            for polarisation in possible_polarisations:
                if polarisation in entry_name:
                    break
            RenameWorkspace(InputWorkspace=entry, OutputWorkspace="{}_{}".format(appended_ws, polarisation))

        D7AbsoluteCrossSections(
            InputWorkspace=appended_ws,
            OutputWorkspace="normalized_single_crystal_XYZ",
            CrossSectionSeparationMethod="XYZ",
            NormalisationMethod="Vanadium",
            VanadiumInputWorkspace="vanadium_ws",
            OutputUnits="Qxy",
            SampleAndEnvironmentProperties=sample_dictionary,
            AbsoluteUnitsNormalisation=False,
            IsotropicMagnetism=False,
            MeasurementTechnique="SingleCrystal",
            ClearCache=True,
        )


class ILL_D7_TimeOfFlight_Test(systemtesting.MantidSystemTest):
    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D7"
        config.appendDataSearchSubDir("ILL/D7/")

    @classmethod
    def tearDown(cls):
        config["default.facility"] = cls._original_facility
        config["default.instrument"] = cls._original_instrument
        config.setDataSearchDirs(cls._data_search_dirs)

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e4
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument"]
        self.nanEqual = True
        return ["h2O_reduced_norm", "ILL_D7_TOF_Z.nxs"]

    def runTest(self):
        vanadium_mass = 6.11 * 4.0 * np.pi * (0.6**2 - 0.4**2)  # 15.356 g
        formula_weight_H2O = 1.008 * 2 + 15.999  # NIST
        sample_mass_H2O = 0.874
        sample_formula_H2O = "H2O"
        max_tof_channel = 500
        sample_dictionary_H2O = {
            "SampleMass": sample_mass_H2O,
            "FormulaUnitMass": formula_weight_H2O,
            "SampleChemicalFormula": sample_formula_H2O,
            "EPCentre": 1645.0,
        }
        yig_calibration_file = "D7_YIG_calibration_TOF.xml"

        # empty container
        PolDiffILLReduction(
            Run="396016:396017",
            OutputTreatment="AveragePol",
            OutputWorkspace="container_vana_ws",
            ProcessAs="Empty",
            MeasurementTechnique="TOF",
            MaxTOFChannel=max_tof_channel,
        )
        # Quartz has not been measured

        vanadium_dictionary = {
            "SampleMass": vanadium_mass,
            "FormulaUnitMass": 50.942,
            "EPCentre": 1645.0,
            "EPWidth": 54.0,  # TOF units
            "EPNSigmasBckg": 3.0,
            "EPNSigmasVana": 3.0,
        }

        # Vanadium reduction
        PolDiffILLReduction(
            Run="396016:396017",
            OutputWorkspace="vanadium_ws",
            EmptyContainerWorkspace="container_vana_ws",
            Transmission="0.9",
            OutputTreatment="Sum",
            SelfAttenuationMethod="None",
            SampleGeometry="None",
            AbsoluteNormalisation=False,
            SampleAndEnvironmentProperties=vanadium_dictionary,
            MeasurementTechnique="TOF",
            ProcessAs="Vanadium",
            InstrumentCalibration=yig_calibration_file,
            FrameOverlapCorrection=True,
            DetectorEnergyEfficiencyCorrection=True,
            ConvertToEnergy=True,
            EnergyBinning="-10,0.02,2",
            MaxTOFChannel=max_tof_channel,
            ClearCache=True,
        )

        # water reduction
        PolDiffILLReduction(
            Run="395639:395640",
            OutputWorkspace="h2O_ws",
            EmptyContainerWorkspace="container_vana_ws",
            Transmission="0.86",
            OutputTreatment="AveragePol",
            SampleGeometry="None",
            SampleAndEnvironmentProperties=sample_dictionary_H2O,
            MeasurementTechnique="TOF",
            InstrumentCalibration=yig_calibration_file,
            ConvertToEnergy=True,
            FrameOverlapCorrection=True,
            DetectorEnergyEfficiencyCorrection=True,
            MaxTOFChannel=max_tof_channel,
            ProcessAs="Sample",
            EnergyBinning="-10,0.02,2",
        )

        D7AbsoluteCrossSections(
            InputWorkspace="h2O_ws",
            OutputWorkspace="h2O_reduced_norm",
            CrossSectionSeparationMethod="Z",
            NormalisationMethod="Vanadium",
            VanadiumInputWorkspace="vanadium_ws",
            OutputUnits="Default",
            SampleAndEnvironmentProperties=sample_dictionary_H2O,
            AbsoluteUnitsNormalisation=True,
            MeasurementTechnique="TOF",
            ClearCache=True,
        )
