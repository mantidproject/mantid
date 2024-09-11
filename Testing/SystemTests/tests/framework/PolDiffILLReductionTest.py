# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import MatrixWorkspace, WorkspaceGroup, Run
from mantid.simpleapi import CloneWorkspace, config, CreateEmptyTableWorkspace, D7AbsoluteCrossSections, Load, mtd, PolDiffILLReduction
from mantid.geometry import Instrument


class PolDiffILLReductionTest(systemtesting.MantidSystemTest):
    _pixels_per_bank = 44
    _sampleProperties = None

    def __init__(self):
        super(PolDiffILLReductionTest, self).__init__()
        self.setUp()

    def setUp(self):
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D7"
        config.appendDataSearchSubDir("ILL/D7/")
        Load("numerical_attenuation.nxs", OutputWorkspace="numerical_attenuation_ws")

        self._sampleProperties = {
            "SampleChemicalFormula": "V",
            "SampleMass": 8.54,
            "FormulaUnitMass": 50.94,
            "SampleInnerRadius": 2,
            "SampleOuterRadius": 2.5,
            "SampleRadius": 2.5,
            "Height": 2,
            "SampleThickness": 0.5,
            "SampleDensity": 0.1,
            "SampleAngle": 0,
            "SampleWidth": 2.5,
            "BeamWidth": 3.0,
            "BeamHeight": 3.0,
            "ContainerRadius": 2.7,
            "ContainerInnerRadius": 1.99,
            "ContainerOuterRadius": 2.7,
            "ContainerFrontThickness": 0.2,
            "ContainerBackThickness": 0.2,
            "ContainerChemicalFormula": "Al",
            "ContainerDensity": 0.01,
            "EventsPerPoint": 100,
            "ElementSize": 1.0,
            "IncoherentCrossSection": 0.1,
            "SampleSpin": 1.5,
            "EPCentre": 1650,
        }

    def cleanup(self):
        mtd.clear()

    def d7_reduction_test_vanadium_individual(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_individual",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_individual"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_individual"], "Vanadium")

    def d7_reduction_test_vanadium_sum(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_sum",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Sum",
        )
        self._check_output(mtd["vanadium_sum"], 1, 132, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_sum"], "Vanadium")

    def d7_reduction_test_vanadium_average(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_average",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="AveragePol",
        )
        self._check_output(mtd["vanadium_average"], 1, 132, 6, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_average"], "Vanadium")

    def d7_reduction_test_vanadium_average_2theta(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_average",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="AverageTwoTheta",
        )
        self._check_output(mtd["vanadium_average"], 1, 132, 6, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_average"], "Vanadium")

    def d7_reduction_test_vanadium_scatter(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_scatter",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="IndividualXY",
        )
        self._check_output(mtd["vanadium_full"], 264, 1, 1, "Momentum transfer", "Momentum transfer", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_scatter"], "Vanadium")

    def d7_reduction_test_vanadium_full_reduction(self):
        PolDiffILLReduction(Run="396983", ProcessAs="EmptyBeam", OutputWorkspace="beam_ws")
        PolDiffILLReduction(Run="396985", ProcessAs="Transmission", OutputWorkspace="quartz_transmission", EmptyBeamWorkspace="beam_ws")
        PolDiffILLReduction(Run="396917,396918", ProcessAs="Empty", OutputWorkspace="container_ws")
        PolDiffILLReduction(Run="396928,396929", ProcessAs="Cadmium", OutputWorkspace="absorber_ws")
        PolDiffILLReduction(Run="396991", ProcessAs="BeamWithCadmium", OutputWorkspace="cadmium_ws")
        PolDiffILLReduction(
            Run="396939,397000",
            ProcessAs="Quartz",
            OutputWorkspace="pol_corrections",
            Transmission="quartz_transmission",
            CadmiumTransmissionWorkspace="cadmium_ws",
            OutputTreatment="AveragePol",
        )
        PolDiffILLReduction(
            Run="396990",
            ProcessAs="Transmission",
            OutputWorkspace="vanadium_transmission",
            CadmiumTransmissionWorkspace="cadmium_ws",
            EmptyBeamWorkspace="beam_ws",
        )
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_full",
            CadmiumWorkspace="absorber_ws",
            EmptyContainerWorkspace="container_ws",
            Transmission="vanadium_transmission",
            QuartzWorkspace="pol_corrections",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Sum",
            MaskDetectors=[30, 50, 127],
        )
        self._check_output(mtd["vanadium_full"], 1, 132, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_full"], "Vanadium")

    def d7_reduction_test_vanadium_individual_transmission_attenuation(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_ind_transmission_att",
            SelfAttenuationMethod="Transmission",
            SampleGeometry="None",
            Transmission="0.95",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_ind_transmission_att"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_ind_transmission_att"], "Vanadium")

    def d7_reduction_test_vanadium_individual_flat_plate_numerical(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_ind_num_flat_plate",
            SelfAttenuationMethod="Numerical",
            SampleGeometry="FlatPlate",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_ind_num_flat_plate"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_ind_num_flat_plate"], "Vanadium")

    def d7_reduction_test_vanadium_individual_flat_plate_monte_carlo(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_ind_mc_flat_plate",
            SelfAttenuationMethod="MonteCarlo",
            SampleGeometry="FlatPlate",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_ind_mc_flat_plate"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_ind_mc_flat_plate"], "Vanadium")

    def d7_reduction_test_vanadium_individual_cylinder_numerical(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_ind_num_cylinder",
            SelfAttenuationMethod="Numerical",
            SampleGeometry="Cylinder",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_ind_num_cylinder"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_ind_num_cylinder"], "Vanadium")

    def d7_reduction_test_vanadium_individual_cylinder_monte_carlo(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_ind_mc_cylinder",
            SelfAttenuationMethod="MonteCarlo",
            SampleGeometry="Cylinder",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_ind_mc_cylinder"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_ind_mc_cylinder"], "Vanadium")

    def d7_reduction_test_vanadium_individual_annulus_numerical(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_ind_num_annulus",
            SelfAttenuationMethod="Numerical",
            SampleGeometry="Annulus",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_ind_num_annulus"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_ind_num_annulus"], "Vanadium")

    def d7_reduction_test_vanadium_individual_annulus_monte_carlo(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_ind_mc_annulus",
            SelfAttenuationMethod="MonteCarlo",
            SampleGeometry="Annulus",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_ind_mc_annulus"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_ind_mc_annulus"], "Vanadium")

    def d7_reduction_test_vanadium_individual_user(self):
        CloneWorkspace(InputWorkspace="numerical_attenuation_ws", OutputWorkspace="numerical_attenuation_ws_clone")
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_ind_cylinder",
            SelfAttenuationMethod="User",
            SampleGeometry="Custom",
            SampleSelfAttenuationFactors="numerical_attenuation_ws_clone",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["vanadium_ind_cylinder"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_ind_cylinder"], "Vanadium")

    def d7_reduction_test_vanadium_sum_flat_plate(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_sum_num_flat_plate",
            SelfAttenuationMethod="Numerical",
            SampleGeometry="FlatPlate",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Sum",
        )
        self._check_output(mtd["vanadium_sum_num_flat_plate"], 1, 132, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_sum_num_flat_plate"], "Vanadium")

    def d7_reduction_test_vanadium_sum_cylinder(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_sum_num_cylinder",
            SelfAttenuationMethod="Numerical",
            SampleGeometry="Cylinder",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Sum",
        )
        self._check_output(mtd["vanadium_sum_num_cylinder"], 1, 132, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_sum_num_cylinder"], "Vanadium")

    def d7_reduction_test_vanadium_sum_annulus(self):
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_sum_mc_annulus",
            SelfAttenuationMethod="MonteCarlo",
            SampleGeometry="Annulus",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Sum",
        )
        self._check_output(mtd["vanadium_sum_mc_annulus"], 1, 132, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_sum_mc_annulus"], "Vanadium")

    def d7_reduction_test_vanadium_sum_user(self):
        CloneWorkspace(InputWorkspace="numerical_attenuation_ws", OutputWorkspace="numerical_attenuation_ws_clone")
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_sum_user",
            SelfAttenuationMethod="User",
            SampleGeometry="Custom",
            SampleSelfAttenuationFactors="numerical_attenuation_ws_clone",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Sum",
        )
        self._check_output(mtd["vanadium_sum_user"], 1, 132, 1, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["vanadium_sum_user"], "Vanadium")

    def d7_reduction_test_sample_individual(self):
        PolDiffILLReduction(
            Run="397004,397005",
            ProcessAs="Sample",
            OutputWorkspace="sample_individual",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["sample_individual"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["sample_individual"], "Sample")
        D7AbsoluteCrossSections(
            InputWorkspace="sample_individual",
            OutputWorkspace="sample_individual_not_normalised",
            CrossSectionSeparationMethod="XYZ",
            NormalisationMethod="None",
            OutputTreatment="Merge",
            OutputUnits="Q",
            SampleAndEnvironmentProperties=self._sampleProperties,
        )
        self._check_output(mtd["sample_individual_not_normalised"], 263, 1, 6, "q", "MomentumTransfer", "Height", "Label")

    def d7_reduction_test_sample_individual_incoherent(self):
        PolDiffILLReduction(
            Run="397004,397005",
            ProcessAs="Sample",
            OutputWorkspace="sample_individual",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["sample_individual"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["sample_individual"], "Sample")
        D7AbsoluteCrossSections(
            InputWorkspace="sample_individual",
            OutputWorkspace="sample_individual_incoherent",
            CrossSectionSeparationMethod="XYZ",
            NormalisationMethod="Incoherent",
            OutputTreatment="Merge",
            OutputUnits="TwoTheta",
            SampleAndEnvironmentProperties=self._sampleProperties,
        )
        self._check_output(mtd["sample_individual_incoherent"], 263, 1, 6, "Scattering Angle", "Label", "Height", "Label")

    def d7_reduction_test_sample_individual_paramagnetic(self):
        PolDiffILLReduction(
            Run="397004,397005",
            ProcessAs="Sample",
            OutputWorkspace="sample_individual",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["sample_individual"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["sample_individual"], "Sample")
        D7AbsoluteCrossSections(
            InputWorkspace="sample_individual",
            OutputWorkspace="sample_individual_paramagnetic",
            CrossSectionSeparationMethod="XYZ",
            NormalisationMethod="Paramagnetic",
            OutputTreatment="Individual",
            OutputUnits="Q",
            SampleAndEnvironmentProperties=self._sampleProperties,
        )
        self._check_output(
            mtd["sample_individual_paramagnetic"],
            132,
            1,
            12,
            "q",
            "MomentumTransfer",
            "Wavelength",
            "Wavelength",
            normalised_individually=True,
        )

    def d7_reduction_test_sample_sum(self):
        PolDiffILLReduction(
            Run="397004,397005",
            ProcessAs="Sample",
            OutputWorkspace="sample_sum",
            SampleAndEnvironmentProperties=self._sampleProperties,
            ScatteringAngleBinSize=1.0,
            OutputTreatment="Sum",
        )
        self._check_output(mtd["sample_sum"], 132, 1, 1, "Scattering Angle", "Label", "Height", "Label")
        self._check_process_flag(mtd["sample_sum"], "Sample")

    def d7_reduction_test_sample_crop_tof_axis(self):
        # creates table workspace with mock elastic peak positions and widths:
        table_ws = CreateEmptyTableWorkspace()
        table_ws.addColumn("float", "PeakCentre")
        table_ws.addColumn("float", "Sigma")
        for row in range(132):
            table_ws.addRow([1645.2, 15.0])

        sampleProperties = {"SampleMass": 2.93, "FormulaUnitMass": 50.94}
        yig_calibration_file = "D7_YIG_calibration_TOF.xml"
        PolDiffILLReduction(
            Run="395639",
            ProcessAs="Sample",
            OutputWorkspace="sample_tof",
            SampleAndEnvironmentProperties=sampleProperties,
            SampleGeometry="None",
            OutputTreatment="Individual",
            InstrumentCalibration=yig_calibration_file,
            ElasticChannelsWorkspace=table_ws,
            MeasurementTechnique="TOF",
            ConvertToEnergy=False,
            MaxTOFChannel=500,
        )
        self._check_output(mtd["sample_tof"], 500, 132, 2, "Time-of-flight", "TOF", "Spectrum", "Label")
        self._check_process_flag(mtd["sample_tof"], "Sample")

    def d7_reduction_test_sample_tof_data_bckg_subtraction(self):
        yig_calibration_file = "D7_YIG_calibration_TOF.xml"
        # uses TOF vanadium sample as the source of background
        PolDiffILLReduction(Run="396016", ProcessAs="Empty", OutputWorkspace="container_ws")
        PolDiffILLReduction(
            Run="395639",
            ProcessAs="Sample",
            OutputWorkspace="sample_tof_data_bckg",
            SampleAndEnvironmentProperties=self._sampleProperties,
            SampleGeometry="None",
            OutputTreatment="Individual",
            Transmission="0.95",
            EmptyContainerWorkspace="container_ws",
            InstrumentCalibration=yig_calibration_file,
            MeasurementTechnique="TOF",
            ConvertToEnergy=False,
            SubtractTOFBackgroundMethod="Data",
        )
        self._check_output(mtd["sample_tof_data_bckg"], 512, 132, 2, "Time-of-flight", "TOF", "Spectrum", "Label")
        self._check_process_flag(mtd["sample_tof_data_bckg"], "Sample")

    def d7_reduction_test_sample_tof_gauss_bckg_subtraction(self):
        yig_calibration_file = "D7_YIG_calibration_TOF.xml"
        # uses TOF vanadium sample as the source of background
        PolDiffILLReduction(Run="396016", ProcessAs="Empty", OutputWorkspace="container_ws")
        PolDiffILLReduction(
            Run="395639",
            ProcessAs="Sample",
            OutputWorkspace="sample_tof_data_bckg",
            SampleAndEnvironmentProperties=self._sampleProperties,
            SampleGeometry="None",
            OutputTreatment="Individual",
            Transmission="0.95",
            EmptyContainerWorkspace="container_ws",
            InstrumentCalibration=yig_calibration_file,
            MeasurementTechnique="TOF",
            ConvertToEnergy=False,
            SubtractTOFBackgroundMethod="Gaussian",
        )
        self._check_output(mtd["sample_tof_data_bckg"], 512, 132, 2, "Time-of-flight", "TOF", "Spectrum", "Label")
        self._check_process_flag(mtd["sample_tof_data_bckg"], "Sample")

    def d7_reduction_test_sample_full_reduction(self):
        PolDiffILLReduction(Run="396983", ProcessAs="EmptyBeam", OutputWorkspace="beam_ws")
        PolDiffILLReduction(Run="396985", ProcessAs="Transmission", OutputWorkspace="quartz_transmission", EmptyBeamWorkspace="beam_ws")
        PolDiffILLReduction(Run="396917,396918", ProcessAs="Empty", OutputWorkspace="container_ws")
        PolDiffILLReduction(Run="396928,396929", ProcessAs="Cadmium", OutputWorkspace="absorber_ws")
        PolDiffILLReduction(Run="396991", ProcessAs="BeamWithCadmium", OutputWorkspace="cadmium_ws")
        PolDiffILLReduction(
            Run="396939,397000",
            ProcessAs="Quartz",
            OutputWorkspace="pol_corrections",
            Transmission="quartz_transmission",
            CadmiumTransmissionWorkspace="cadmium_ws",
            OutputTreatment="AveragePol",
        )
        PolDiffILLReduction(
            Run="396990",
            ProcessAs="Transmission",
            OutputWorkspace="vanadium_transmission",
            CadmiumTransmissionWorkspace="cadmium_ws",
            EmptyBeamWorkspace="beam_ws",
        )
        PolDiffILLReduction(
            Run="396993,396994",
            ProcessAs="Vanadium",
            OutputWorkspace="vanadium_full",
            CadmiumWorkspace="absorber_ws",
            EmptyContainerWorkspace="container_ws",
            Transmission="vanadium_transmission",
            QuartzWorkspace="pol_corrections",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Sum",
        )
        PolDiffILLReduction(
            Run="396986,396987",
            ProcessAs="Transmission",
            OutputWorkspace="sample_transmission",
            CadmiumTransmissionWorkspace="cadmium_ws",
            EmptyBeamWorkspace="beam_ws",
        )
        PolDiffILLReduction(
            Run="397004,397005",
            ProcessAs="Sample",
            OutputWorkspace="sample_full",
            CadmiumWorkspace="absorber_ws",
            EmptyContainerWorkspace="container_ws",
            Transmission="vanadium_transmission",
            QuartzWorkspace="pol_corrections",
            SampleAndEnvironmentProperties=self._sampleProperties,
            OutputTreatment="Individual",
        )
        self._check_output(mtd["sample_full"], 1, 132, 12, "Wavelength", "Wavelength", "Spectrum", "Label")
        self._check_process_flag(mtd["sample_full"], "Sample")

        D7AbsoluteCrossSections(
            InputWorkspace="sample_full",
            OutputWorkspace="sample_full_normalised",
            CrossSectionSeparationMethod="XYZ",
            VanadiumInputWorkspace="vanadium_full",
            NormalisationMethod="Vanadium",
            OutputTreatment="Merge",
            OutputUnits="Q",
            SampleAndEnvironmentProperties=self._sampleProperties,
            AbsoluteUnitsNormalisation=True,
        )
        self._check_output(mtd["sample_full_normalised"], 263, 1, 6, "q", "MomentumTransfer", "Height", "Label")

    def _check_process_flag(self, ws, value):
        self.assertTrue(ws[0].getRun().getLogData("ProcessedAs").value, value)

    def _check_output(self, ws, blocksize, spectra, nEntries, x_unit, x_unit_id, y_unit, y_unit_id, normalised_individually=False):
        self.assertTrue(ws)
        self.assertTrue(isinstance(ws, WorkspaceGroup))
        self.assertEqual(ws.getNumberOfEntries(), nEntries)
        for entry in ws:
            self.assertTrue(isinstance(entry, MatrixWorkspace))
            self.assertTrue(not entry.isDistribution())
            if normalised_individually:
                self.assertTrue(not entry.isHistogramData())
            else:
                self.assertTrue(entry.isHistogramData())
            self.assertEqual(entry.getAxis(0).getUnit().caption(), x_unit)
            self.assertEqual(entry.getAxis(0).getUnit().unitID(), x_unit_id)
            self.assertEqual(entry.getAxis(1).getUnit().caption(), y_unit)
            self.assertEqual(entry.getAxis(1).getUnit().unitID(), y_unit_id)
            self.assertEqual(entry.blocksize(), blocksize)
            self.assertEqual(entry.getNumberHistograms(), spectra)
            self.assertTrue(isinstance(entry.getInstrument(), Instrument))
            self.assertTrue(isinstance(entry.getRun(), Run))
            self.assertTrue(entry.getHistory())

    def runTest(self):
        self.d7_reduction_test_vanadium_individual()
        self.d7_reduction_test_vanadium_average()
        self.d7_reduction_test_vanadium_sum()
        self.d7_reduction_test_vanadium_full_reduction()
        self.d7_reduction_test_vanadium_individual_transmission_attenuation()
        self.d7_reduction_test_vanadium_individual_flat_plate_numerical()
        self.d7_reduction_test_vanadium_individual_flat_plate_monte_carlo()
        self.d7_reduction_test_vanadium_individual_cylinder_numerical()
        self.d7_reduction_test_vanadium_individual_cylinder_monte_carlo()
        self.d7_reduction_test_vanadium_individual_annulus_numerical()
        self.d7_reduction_test_vanadium_individual_annulus_monte_carlo()
        self.d7_reduction_test_vanadium_individual_user()
        self.d7_reduction_test_vanadium_sum_flat_plate()
        self.d7_reduction_test_vanadium_sum_cylinder()
        self.d7_reduction_test_vanadium_sum_annulus()
        self.d7_reduction_test_vanadium_sum_user()
        self.d7_reduction_test_sample_individual()
        self.d7_reduction_test_sample_individual_incoherent()
        self.d7_reduction_test_sample_individual_paramagnetic()
        self.d7_reduction_test_sample_sum()
        self.d7_reduction_test_sample_crop_tof_axis()
        self.d7_reduction_test_sample_tof_data_bckg_subtraction()
        self.d7_reduction_test_sample_tof_gauss_bckg_subtraction()
        self.d7_reduction_test_sample_full_reduction()
