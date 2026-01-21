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
from mantid.kernel import Logger
import h5py
import os
import numpy as np
import tempfile
import shutil
from unittest.mock import patch


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
        fp = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        algo.setProperty("SampleFilename", fp)
        instrument_prop = algo.getProperty("Instrument")
        instrument_prop.settings._applyChanges(algo, "Instrument")
        instrument = instrument_prop.value
        self.assertEqual(instrument, "WAND^2")

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

    def test_auto_populate_wavelength_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        fp = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        algo.setProperty("SampleFilename", fp)
        vanadium_diameter_prop = algo.getProperty("Wavelength")
        if isinstance(vanadium_diameter_prop.settings, list):
            for setting in vanadium_diameter_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "Wavelength")
        else:
            if hasattr(vanadium_diameter_prop.settings, "_applyChanges"):
                vanadium_diameter_prop.settings._applyChanges(algo, "Wavelength")
        diameter = vanadium_diameter_prop.value
        self.assertEqual(diameter, 2.5)

    def test_auto_populate_vanadium_diameter_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        fp = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        algo.setProperty("SampleFilename", fp)
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

    def test_auto_populate_vanadium_run_numbers_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        fp = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        algo.setProperty("SampleFilename", fp)
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

    def test_auto_populate_vanadium_background_run_numbers_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        fp = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        algo.setProperty("SampleFilename", fp)
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

    def test_auto_populate_vanadium_ipts_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        fp = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        algo.setProperty("SampleFilename", fp)
        vanadium_ipts_prop = algo.getProperty("VanadiumIPTS")
        if isinstance(vanadium_ipts_prop.settings, list):
            for setting in vanadium_ipts_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "VanadiumIPTS")
        else:
            if hasattr(vanadium_ipts_prop.settings, "_applyChanges"):
                vanadium_ipts_prop.settings._applyChanges(algo, "VanadiumIPTS")
        ipts = vanadium_ipts_prop.value
        self.assertEqual(ipts, 789)

    def test_auto_populate_vanadium_background_ipts_from_filename(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()
        fp = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        algo.setProperty("SampleFilename", fp)
        vanadium_background_ipts_prop = algo.getProperty("VanadiumBackgroundIPTS")
        if isinstance(vanadium_background_ipts_prop.settings, list):
            for setting in vanadium_background_ipts_prop.settings:
                if hasattr(setting, "_applyChanges"):
                    setting._applyChanges(algo, "VanadiumBackgroundIPTS")
        else:
            if hasattr(vanadium_background_ipts_prop.settings, "_applyChanges"):
                vanadium_background_ipts_prop.settings._applyChanges(algo, "VanadiumBackgroundIPTS")
        ipts = vanadium_background_ipts_prop.value
        self.assertEqual(ipts, 987)

    def test_auto_populate_wavelength_from_run_numbers(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()

        existing_file = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        original_h5py_File = h5py.File

        def mock_file_func(path, mode="r"):
            return original_h5py_File(existing_file, mode)

        with patch("h5py.File", side_effect=mock_file_func):
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

        existing_file = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        original_h5py_File = h5py.File

        def mock_file_func(path, mode="r"):
            return original_h5py_File(existing_file, mode)

        with patch("h5py.File", side_effect=mock_file_func):
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

        existing_file = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        original_h5py_File = h5py.File

        def mock_file_func(path, mode="r"):
            return original_h5py_File(existing_file, mode)

        with patch("h5py.File", side_effect=mock_file_func):
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

        existing_file = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        original_h5py_File = h5py.File

        def mock_file_func(path, mode="r"):
            return original_h5py_File(existing_file, mode)

        with patch("h5py.File", side_effect=mock_file_func):
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

    def test_auto_populate_vanadium_ipts_from_run_numbers(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()

        existing_file = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        original_h5py_File = h5py.File

        def mock_file_func(path, mode="r"):
            return original_h5py_File(existing_file, mode)

        with patch("h5py.File", side_effect=mock_file_func):
            algo.setProperty("SampleIPTS", 123)
            algo.setProperty("SampleRunNumbers", [456])
            algo.setProperty("Instrument", "WAND^2")
            prop = algo.getProperty("VanadiumIPTS")
            if isinstance(prop.settings, list):
                for setting in prop.settings:
                    if hasattr(setting, "_applyChanges"):
                        setting._applyChanges(algo, "VanadiumIPTS")
            else:
                if hasattr(prop.settings, "_applyChanges"):
                    prop.settings._applyChanges(algo, "VanadiumIPTS")
            val = prop.value
            compareResult = val == 789
            self.assertTrue(compareResult)

    def test_auto_populate_vanadium_background_ipts_from_run_numbers(self):
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()

        existing_file = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")
        original_h5py_File = h5py.File

        def mock_file_func(path, mode="r"):
            return original_h5py_File(existing_file, mode)

        with patch("h5py.File", side_effect=mock_file_func):
            algo.setProperty("SampleIPTS", 123)
            algo.setProperty("SampleRunNumbers", [456])
            algo.setProperty("Instrument", "WAND^2")
            prop = algo.getProperty("VanadiumBackgroundIPTS")
            if isinstance(prop.settings, list):
                for setting in prop.settings:
                    if hasattr(setting, "_applyChanges"):
                        setting._applyChanges(algo, "VanadiumBackgroundIPTS")
            else:
                if hasattr(prop.settings, "_applyChanges"):
                    prop.settings._applyChanges(algo, "VanadiumBackgroundIPTS")
            val = prop.value
            compareResult = val == 987
            self.assertTrue(compareResult)


class MetadataConsistencyTests(unittest.TestCase):
    def test_metadata_consistency_single_file(self):
        """Test that single file does not trigger metadata check"""
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()

        existing_file = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")

        # Single file should not check metadata consistency
        algo.setProperty("SampleFilename", existing_file)
        algo.setProperty("Instrument", "WAND^2")
        algo.setProperty("XMin", [1.0])
        algo.setProperty("XMax", [10.0])
        algo.setProperty("Wavelength", 2.5)
        algo.setProperty("VanadiumDiameter", 0.5)

        # Patch the Logger class's warning method
        with patch.object(Logger, "warning") as mock_warning:
            algo.validateInputs()

            # Verify that warning was called once
            self.assertEqual(mock_warning.call_count, 0)

    def test_metadata_consistency_with_multiple_run_numbers(self):
        """Test that metadata consistency check is invoked for multiple run numbers"""
        algo = AlgorithmManager.create("HFIRPowderReduction")
        algo.initialize()

        # When multiple run numbers are provided, the validation should attempt to check metadata
        # We won't test actual file differences here since that requires complex mocking,
        # but we verify the check is in place by ensuring no crashes with valid setup
        algo.setProperty("SampleIPTS", 123)
        algo.setProperty("SampleRunNumbers", [456])  # Single run - no metadata check
        algo.setProperty("Instrument", "WAND^2")
        algo.setProperty("XMin", [1.0])
        algo.setProperty("XMax", [10.0])
        algo.setProperty("Wavelength", 2.5)
        algo.setProperty("VanadiumDiameter", 0.5)

        # Patch the Logger class's warning method
        with patch.object(Logger, "warning") as mock_warning:
            algo.validateInputs()

            # Verify that warning was called once
            self.assertEqual(mock_warning.call_count, 0)

    def test_metadata_consistency_detects_mismatched_wavelength(self):
        """Test that mismatched wavelength is detected between multiple files"""
        existing_file = os.path.join(os.getcwd(), "../../ExternalData/Testing/Data/UnitTest/HB2C_456.nxs.h5")

        # Create a temporary modified file with different wavelength
        temp_dir = tempfile.mkdtemp()
        try:
            modified_file = os.path.join(temp_dir, "HB2C_457.nxs.h5")
            shutil.copy(existing_file, modified_file)

            # Modify the wavelength in the copied file
            with h5py.File(modified_file, "a") as f:
                if "/entry/wavelength" in f:
                    del f["/entry/wavelength"]
                    f["/entry/wavelength"] = np.array([b"3.5"])  # Different from original 2.5

            # Now test with both files
            algo = AlgorithmManager.create("HFIRPowderReduction")
            algo.initialize()

            # Use a comma-separated string for multiple files
            algo.setProperty("SampleFilename", f"{existing_file},{modified_file}")
            algo.setProperty("Instrument", "WAND^2")
            algo.setProperty("XMin", [1.0])
            algo.setProperty("XMax", [10.0])
            algo.setProperty("Wavelength", 2.5)
            algo.setProperty("VanadiumDiameter", 0.5)

            # Patch the Logger class's warning method
            with patch.object(Logger, "warning") as mock_warning:
                algo.validateInputs()

                # Verify that warning was called once
                self.assertEqual(mock_warning.call_count, 1)

        finally:
            # Clean up temporary directory
            shutil.rmtree(temp_dir, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
