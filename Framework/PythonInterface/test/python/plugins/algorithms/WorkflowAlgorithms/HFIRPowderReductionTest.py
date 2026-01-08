# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import unittest
from mantid.simpleapi import HFIRPowderReduction
from mantid.api import AlgorithmManager
import h5py
import os
import numpy as np


class LoadInputErrorMessages(unittest.TestCase):
    def test_validate_sample_inputs_too_many_fields(self):
        # Test that providing both filename and IPTS/RunNumbers raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", SampleIPTS=123, SampleRunNumbers=[456], Instrument="WAND^2")

        # Check the error message
        error_msg = str(cm.exception)
        self.assertIn("SampleFilename", error_msg)
        self.assertIn("Too many fields filled: Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers", error_msg)

    def test_validate_sample_inputs_missing_fields(self):
        # Test that not providing any sample inputs raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            HFIRPowderReduction(Instrument="WAND^2")

        # Check the error message
        error_msg = str(cm.exception)
        self.assertIn("SampleFilename", error_msg)
        self.assertIn("Missing required field: Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers", error_msg)

    def test_validate_optional_fields_too_many(self):
        # Test each optional field (Vanadium, VanadiumBackground, SampleBackground)
        all_fields = ["Sample", "Vanadium", "VanadiumBackground", "SampleBackground"]

        for field in all_fields:
            # Test that providing both filename and IPTS/RunNumbers for optional fields raises a RuntimeError
            kwargs = {
                "SampleFilename": "HB2C_7000.nxs.h5",  # Valid sample input
                f"{field}Filename": "HB2C_7000.nxs.h5",
                f"{field}IPTS": 123,
                f"{field}RunNumbers": [456],
                "Instrument": "WAND^2",
            }

            with self.assertRaises(RuntimeError) as cm:
                HFIRPowderReduction(**kwargs)

            # Check the error message
            error_msg = str(cm.exception)
            self.assertIn(f"{field}Filename", error_msg)
            self.assertIn(f"Too many fields filled: Must specify either {field}Filename or {field}IPTS AND {field}RunNumbers", error_msg)

    def test_validate_missing_ipts_or_runnumbers(self):
        # Test each optional field (Vanadium, VanadiumBackground, SampleBackground)
        all_fields = ["Sample", "Vanadium", "VanadiumBackground", "SampleBackground"]

        for field in all_fields:
            # Test that missing IPTS or RunNumbers when the other is filled out for optional fields raises a RuntimeError
            kwargs = {f"{field}IPTS": 123, "Instrument": "WAND^2"}

            with self.assertRaises(RuntimeError) as cm:
                HFIRPowderReduction(**kwargs)

            # Check the error message
            error_msg = str(cm.exception)
            self.assertIn(f"{field}RunNumbers", error_msg)
            self.assertIn(f"{field}RunNumbers must be provided if {field}IPTS is provided", error_msg)

            kwargs = {f"{field}RunNumbers": [456], "Instrument": "WAND^2"}

            with self.assertRaises(RuntimeError) as cm:
                HFIRPowderReduction(**kwargs)

            # Check the error message
            error_msg = str(cm.exception)
            self.assertIn(f"{field}IPTS", error_msg)
            self.assertIn(f"{field}IPTS must be provided if {field}RunNumbers is provided", error_msg)

    def test_valid_sample_input_combinations(self):
        # Test filename only - should not raise
        try:
            HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", Instrument="WAND^2")
        except RuntimeError as e:
            if "Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers" in str(e):
                self.fail("Valid input combination with SampleFilename failed validation")

        # Test IPTS and RunNumbers - should not raise
        try:
            HFIRPowderReduction(SampleIPTS=123, SampleRunNumbers=[456], Instrument="WAND^2")
        except RuntimeError as e:
            if "Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers" in str(e):
                self.fail("Valid input combination with IPTS and RunNumbers failed validation")

    def test_validate_xmin_xmax(self):
        # Test that missing XMin raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMax=10.0, Instrument="WAND^2")

        error_msg = str(cm.exception)
        self.assertIn("XMin", error_msg)
        self.assertIn("XMin must be provided", error_msg)

        # Test that missing XMax raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=1.0, Instrument="WAND^2")

        error_msg = str(cm.exception)
        self.assertIn("XMax", error_msg)
        self.assertIn("XMax must be provided", error_msg)

        # Test that XMin >= XMax raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=10.0, XMax=5.0, Instrument="WAND^2")

        error_msg = str(cm.exception)
        self.assertIn("XMin", error_msg)
        self.assertIn("XMax", error_msg)
        self.assertIn("XMin (10.0) cannot be greater than or equal to XMax (5.0)", error_msg)

        # Test that XMin and XMax of different lengths raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=[1.0, 2.0], XMax=[5.0], Instrument="WAND^2")
        error_msg = str(cm.exception)
        self.assertIn("XMin", error_msg)
        self.assertIn("XMax", error_msg)
        self.assertIn("XMin and XMax do not define same number of spectra (2 != 1)", error_msg)

    def test_validate_instrument(self):
        # Test that missing Instrument raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=1.0, XMax=10.0)

        error_msg = str(cm.exception)
        self.assertIn("Instrument", error_msg)
        self.assertIn("Instrument must be provided", error_msg)

    def test_validate_wavelength(self):
        # Test that missing Wavelength raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=1.0, XMax=10.0, Instrument="WAND^2", VanadiumDiameter=0.5)

        error_msg = str(cm.exception)
        self.assertIn("Wavelength", error_msg)
        self.assertIn("Wavelength must be provided", error_msg)

    def test_vanadium_diameter(self):
        # Test that missing Vandaium Diameter raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=1.0, XMax=10.0, Instrument="WAND^2", Wavelength=2.5)

        error_msg = str(cm.exception)
        self.assertIn("VanadiumDiameter", error_msg)
        self.assertIn("VanadiumDiameter must be provided", error_msg)


class AutoPopulateTests(unittest.TestCase):
    def test_auto_populate_instrument_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        tmp_file = os.path.join(os.getcwd(), "test_instrument.h5")
        with h5py.File(tmp_file, "w") as f:
            f.create_dataset("/entry/instrument/name", data=[b"WAND^2"])
        algo.setProperty("SampleFilename", tmp_file)
        instrument_prop = algo.getProperty("Instrument")
        instrument_prop.settings._applyChanges(algo, "Instrument")
        instrument = instrument_prop.value
        self.assertEqual(instrument, "WAND^2")
        os.remove(tmp_file)

    def test_auto_populate_normalise_by_from_instrument(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        # Test WAND^2 sets NormaliseBy to Monitor
        algo.setProperty("Instrument", "WAND^2")
        normalise_by_prop = algo.getProperty("NormaliseBy")
        normalise_by_prop.settings._applyChanges(algo, "NormaliseBy")
        normalise_by = normalise_by_prop.value
        self.assertEqual(normalise_by, "Monitor")
        # Test MIDAS sets NormaliseBy to Time
        algo.setProperty("Instrument", "MIDAS")
        normalise_by_prop.settings._applyChanges(algo, "NormaliseBy")
        normalise_by = algo.getProperty("NormaliseBy").value
        self.assertEqual(normalise_by, "Time")

    def test_auto_populate_vanadium_diameter_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        tmp_file = os.path.join(os.getcwd(), "test_vanadium_diameter.h5")
        with h5py.File(tmp_file, "w") as f:
            f.create_dataset("/entry/vanadium_diameter", data=[b"0.5"])
        algo.setProperty("SampleFilename", tmp_file)
        vanadium_diameter_prop = algo.getProperty("VanadiumDiameter")
        if isinstance(vanadium_diameter_prop.settings, list):
            for setting in vanadium_diameter_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "VanadiumDiameter")
        else:
            if hasattr(vanadium_diameter_prop.settings, "_applyChanges"):
                vanadium_diameter_prop.settings._applyChanges(algo, "VanadiumDiameter")
        diameter = vanadium_diameter_prop.value
        self.assertEqual(diameter, 0.5)
        os.remove(tmp_file)

    def test_auto_populate_vanadium_run_numbers_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        tmp_file = os.path.join(os.getcwd(), "test_vanadium_run_numbers.h5")
        with h5py.File(tmp_file, "w") as f:
            f.create_dataset("/entry/vanadium_run_numbers", data=[7001, 7002])
        algo.setProperty("SampleFilename", tmp_file)
        vanadium_run_numbers_prop = algo.getProperty("VanadiumRunNumbers")
        if isinstance(vanadium_run_numbers_prop.settings, list):
            for setting in vanadium_run_numbers_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "VanadiumRunNumbers")
        else:
            if hasattr(vanadium_run_numbers_prop.settings, "_applyChanges"):
                vanadium_run_numbers_prop.settings._applyChanges(algo, "VanadiumRunNumbers")
        run_numbers = vanadium_run_numbers_prop.value
        compareResult = (run_numbers == np.array([7001, 7002])).all()
        self.assertTrue(compareResult)
        os.remove(tmp_file)

    def test_auto_populate_vanadium_background_run_numbers_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        tmp_file = os.path.join(os.getcwd(), "test_vanadium_background_run_numbers.h5")
        with h5py.File(tmp_file, "w") as f:
            f.create_dataset("/entry/vanadium_background_run_numbers", data=[7003, 7004])
        algo.setProperty("SampleFilename", tmp_file)
        vanadium_background_run_numbers_prop = algo.getProperty("VanadiumBackgroundRunNumbers")
        if isinstance(vanadium_background_run_numbers_prop.settings, list):
            for setting in vanadium_background_run_numbers_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "VanadiumBackgroundRunNumbers")
        else:
            if hasattr(vanadium_background_run_numbers_prop.settings, "_applyChanges"):
                vanadium_background_run_numbers_prop.settings._applyChanges(algo, "VanadiumBackgroundRunNumbers")
        run_numbers = vanadium_background_run_numbers_prop.value
        compareResult = (run_numbers == np.array([7003, 7004])).all()
        self.assertTrue(compareResult)
        os.remove(tmp_file)

    def test_auto_populate_wavelength_from_run_numbers(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        algo.setProperty("SampleIPTS", 123)
        algo.setProperty("SampleRunNumbers", [456])
        algo.setProperty("Instrument", "WAND^2")
        wavelength_prop = algo.getProperty("Wavelength")
        if isinstance(wavelength_prop.settings, list):
            for setting in wavelength_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "Wavelength")
        else:
            if hasattr(wavelength_prop.settings, "_applyChanges"):
                wavelength_prop.settings._applyChanges(algo, "Wavelength")
        wavelength = wavelength_prop.value
        self.assertEqual(wavelength, 2.5)

    def test_auto_populate_vanadium_diameter_from_run_numbers(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        algo.setProperty("SampleIPTS", 123)
        algo.setProperty("SampleRunNumbers", [456])
        algo.setProperty("Instrument", "WAND^2")
        vanadium_diameter_prop = algo.getProperty("VanadiumDiameter")
        if isinstance(vanadium_diameter_prop.settings, list):
            for setting in vanadium_diameter_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "VanadiumDiameter")
        else:
            if hasattr(vanadium_diameter_prop.settings, "_applyChanges"):
                vanadium_diameter_prop.settings._applyChanges(algo, "VanadiumDiameter")
        diameter = vanadium_diameter_prop.value
        self.assertEqual(diameter, 0.5)

    def test_auto_populate_vanadium_run_numbers_from_run_numbers(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        algo.setProperty("SampleIPTS", 123)
        algo.setProperty("SampleRunNumbers", [456])
        algo.setProperty("Instrument", "WAND^2")
        vanadium_run_numbers_prop = algo.getProperty("VanadiumRunNumbers")
        if isinstance(vanadium_run_numbers_prop.settings, list):
            for setting in vanadium_run_numbers_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "VanadiumRunNumbers")
        else:
            if hasattr(vanadium_run_numbers_prop.settings, "_applyChanges"):
                vanadium_run_numbers_prop.settings._applyChanges(algo, "VanadiumRunNumbers")
        run_numbers = vanadium_run_numbers_prop.value
        compareResult = (run_numbers == np.array([7001, 7002])).all()
        self.assertTrue(compareResult)

    def test_auto_populate_vanadium_background_run_numbers_from_run_numbers(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        algo.setProperty("SampleIPTS", 123)
        algo.setProperty("SampleRunNumbers", [456])
        algo.setProperty("Instrument", "WAND^2")
        vanadium_background_run_numbers_prop = algo.getProperty("VanadiumBackgroundRunNumbers")
        if isinstance(vanadium_background_run_numbers_prop.settings, list):
            for setting in vanadium_background_run_numbers_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "VanadiumBackgroundRunNumbers")
        else:
            if hasattr(vanadium_background_run_numbers_prop.settings, "_applyChanges"):
                vanadium_background_run_numbers_prop.settings._applyChanges(algo, "VanadiumBackgroundRunNumbers")
        run_numbers = vanadium_background_run_numbers_prop.value
        compareResult = (run_numbers == np.array([7003, 7004])).all()
        self.assertTrue(compareResult)


if __name__ == "__main__":
    unittest.main()
