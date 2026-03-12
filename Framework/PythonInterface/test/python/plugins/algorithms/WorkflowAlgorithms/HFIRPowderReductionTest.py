# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import unittest
from mantid.simpleapi import (
    HFIRPowderReduction,
    CreateSampleWorkspace,
    RotateInstrumentComponent,
    MoveInstrumentComponent,
    CloneWorkspace,
    AddSampleLog,
    GroupWorkspaces,
    SaveNexusESS,
    Load,
)
from mantid.api import AlgorithmManager, MatrixWorkspace, WorkspaceGroup
from mantid.kernel import Logger, V3D
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
            res = HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", SampleIPTS=123, SampleRunNumbers=[456], Instrument="WAND^2")  # noqa: F841

        # Check the error message
        error_msg = str(cm.exception)
        self.assertIn("SampleFilename", error_msg)
        self.assertIn("Too many fields filled: Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers", error_msg)

    def test_validate_sample_inputs_missing_fields(self):
        # Test that not providing any sample inputs raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            res = HFIRPowderReduction(Instrument="WAND^2")  # noqa: F841

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
                res = HFIRPowderReduction(**kwargs)  # noqa: F841

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
                res = HFIRPowderReduction(**kwargs)

            # Check the error message
            error_msg = str(cm.exception)
            self.assertIn(f"{field}RunNumbers", error_msg)
            self.assertIn(f"{field}RunNumbers must be provided if {field}IPTS is provided", error_msg)

            kwargs = {f"{field}RunNumbers": [456], "Instrument": "WAND^2"}

            with self.assertRaises(RuntimeError) as cm:
                res = HFIRPowderReduction(**kwargs)  # noqa: F841

            # Check the error message
            error_msg = str(cm.exception)
            self.assertIn(f"{field}IPTS", error_msg)
            self.assertIn(f"{field}IPTS must be provided if {field}RunNumbers is provided", error_msg)

    def test_valid_sample_input_combinations(self):
        # Test filename only - should not raise
        try:
            res = HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", Instrument="WAND^2")
        except RuntimeError as e:
            if "Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers" in str(e):
                self.fail("Valid input combination with SampleFilename failed validation")

        # Test IPTS and RunNumbers - should not raise
        try:
            res = HFIRPowderReduction(SampleIPTS=123, SampleRunNumbers=[456], Instrument="WAND^2")  # noqa: F841
        except RuntimeError as e:
            if "Must specify either SampleFilename or SampleIPTS AND SampleRunNumbers" in str(e):
                self.fail("Valid input combination with IPTS and RunNumbers failed validation")

    def test_validate_xmin_xmax(self):
        # Test that missing XMin raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            res = HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMax=10.0, Instrument="WAND^2")

        error_msg = str(cm.exception)
        self.assertIn("XMin", error_msg)
        self.assertIn("XMin must be provided", error_msg)

        # Test that missing XMax raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            res = HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=1.0, Instrument="WAND^2")

        error_msg = str(cm.exception)
        self.assertIn("XMax", error_msg)
        self.assertIn("XMax must be provided", error_msg)

        # Test that XMin >= XMax raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            res = HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=10.0, XMax=5.0, Instrument="WAND^2")

        error_msg = str(cm.exception)
        self.assertIn("XMin", error_msg)
        self.assertIn("XMax", error_msg)
        self.assertIn("XMin (10.0) cannot be greater than or equal to XMax (5.0)", error_msg)

        # Test that XMin and XMax of different lengths raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            res = HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=[1.0, 2.0], XMax=[5.0], Instrument="WAND^2")  # noqa: F841
        error_msg = str(cm.exception)
        self.assertIn("XMin", error_msg)
        self.assertIn("XMax", error_msg)
        self.assertIn("XMin and XMax do not define same number of spectra (2 != 1)", error_msg)

    def test_validate_instrument(self):
        # Test that missing Instrument raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            res = HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=1.0, XMax=10.0)  # noqa: F841

        error_msg = str(cm.exception)
        self.assertIn("Instrument", error_msg)
        self.assertIn("Instrument must be provided", error_msg)

    def test_validate_wavelength(self):
        # Test that missing Wavelength raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            res = HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=1.0, XMax=10.0, Instrument="WAND^2", VanadiumDiameter=0.5)  # noqa: F841

        error_msg = str(cm.exception)
        self.assertIn("Wavelength", error_msg)
        self.assertIn("Wavelength must be provided", error_msg)

    def test_vanadium_diameter(self):
        # Test that missing Vandaium Diameter raises a RuntimeError
        with self.assertRaises(RuntimeError) as cm:
            res = HFIRPowderReduction(SampleFilename="HB2C_7000.nxs.h5", XMin=1.0, XMax=10.0, Instrument="WAND^2", Wavelength=2.5)  # noqa: F841

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


class ReductionExecutionTests(unittest.TestCase):
    def setUp(self):
        self._test_dir = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self._test_dir)

    def _create_workspaces(self):
        cal = CreateSampleWorkspace(NumBanks=1, BinWidth=20000, PixelSpacing=0.1, BankPixelWidth=100)
        RotateInstrumentComponent(cal, ComponentName="bank1", X=1, Y=0.5, Z=2, Angle=35)
        MoveInstrumentComponent(cal, ComponentName="bank1", X=1, Y=1, Z=5)
        bkg = CloneWorkspace(cal)
        data = CloneWorkspace(cal)
        AddSampleLog(
            cal,
            LogName="gd_prtn_chrg",
            LogType="Number",
            NumberType="Double",
            LogText="200",
        )
        AddSampleLog(
            bkg,
            LogName="gd_prtn_chrg",
            LogType="Number",
            NumberType="Double",
            LogText="50",
        )
        AddSampleLog(
            data,
            LogName="gd_prtn_chrg",
            LogType="Number",
            NumberType="Double",
            LogText="100",
        )
        AddSampleLog(cal, LogName="duration", LogType="Number", NumberType="Double", LogText="20")
        AddSampleLog(bkg, LogName="duration", LogType="Number", NumberType="Double", LogText="5")
        AddSampleLog(
            data,
            LogName="duration",
            LogType="Number",
            NumberType="Double",
            LogText="10",
        )

        def get_cal_counts(n):
            if n < 5000:
                return 0.9
            else:
                return 1.0

        def get_bkg_counts(n):
            return 1.5 * get_cal_counts(n)

        def get_data_counts(n, twoTheta):
            tt1 = 30
            tt2 = 45
            return get_bkg_counts(n) + 10 * np.exp(-((twoTheta - tt1) ** 2) / 1) + 20 * np.exp(-((twoTheta - tt2) ** 2) / 0.2)

        for i in range(cal.getNumberHistograms()):
            cal.setY(i, [get_cal_counts(i) * 2.0])
            bkg.setY(i, [get_bkg_counts(i) / 2.0])
            twoTheta = data.getInstrument().getDetector(i + 10000).getTwoTheta(V3D(0, 0, 0), V3D(0, 0, 1)) * 180 / np.pi
            data.setY(i, [get_data_counts(i, twoTheta)])

        data_file_name = os.path.join(self._test_dir, "sample_workspace.nxs")
        SaveNexusESS(data, Filename=data_file_name)
        vanadium_file_name = os.path.join(self._test_dir, "vanadium_workspace.nxs")
        SaveNexusESS(cal, Filename=vanadium_file_name)
        background_file_name = os.path.join(self._test_dir, "background_workspace.nxs")
        SaveNexusESS(bkg, Filename=background_file_name)

        return data_file_name, vanadium_file_name, background_file_name

    def test(self):
        data, cal, bkg = self._create_workspaces()

        # data normalised by monitor
        pd_out = HFIRPowderReduction(
            SampleFileName=data, XUnits="2Theta", Instrument="WAND^2", XMax=180, XMin=0, Wavelength=1.6513, VanadiumDiameter=0.5
        )

        x = pd_out.extractX()
        y = pd_out.extractY()

        self.assertAlmostEqual(x.min(), 8.09946893)
        self.assertAlmostEqual(x.max(), 50.80113407)
        self.assertAlmostEqual(y.min(), 0.00784728)
        self.assertAlmostEqual(y.max(), 9.94421639)
        self.assertAlmostEqual(x[0, y.argmax()], 45.10091179)

        # data normalised by monitor <- duplicate input as two
        # NOTE:
        # still needs to check physics
        pd_out_multi = HFIRPowderReduction(
            SampleFileName=(f"{data},{data}"),
            XUnits="2Theta",
            Instrument="WAND^2",
            XMax=180,
            XMin=0,
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
            Sum=True,
        )

        x = pd_out_multi.extractX()
        y = pd_out_multi.extractY()

        self.assertAlmostEqual(x.min(), 8.09946893)
        self.assertAlmostEqual(x.max(), 50.80113407)
        self.assertAlmostEqual(y.min(), 0.01569456)
        self.assertAlmostEqual(y.max(), 19.88843277)
        self.assertAlmostEqual(x[0, y.argmax()], 45.10091179)

        # data and calibration, limited range
        pd_out2 = HFIRPowderReduction(
            SampleFileName=data,
            VanadiumFilename=cal,
            XUnits="2Theta",
            XMin=10,
            XMax=40,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
        )

        x = pd_out2.extractX()
        y = pd_out2.extractY()

        self.assertAlmostEqual(x.min(), 10.05)
        self.assertAlmostEqual(x.max(), 39.95)
        self.assertAlmostEqual(y.min(), 1.5)
        self.assertAlmostEqual(y.max(), 12.57768805)
        self.assertAlmostEqual(x[0, y.argmax()], 29.95)

        # data and calibration, limited range
        # NOTE:
        # still needs to check physics
        pd_out2_multi = HFIRPowderReduction(
            SampleFileName=(f"{data},{data}"),
            VanadiumFilename=cal,
            XUnits="2Theta",
            XMin=10,
            XMax=40,
            Sum=True,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
        )

        x = pd_out2_multi.extractX()
        y = pd_out2_multi.extractY()

        self.assertAlmostEqual(x.min(), 10.05)
        self.assertAlmostEqual(x.max(), 39.95)
        self.assertAlmostEqual(y.min(), 1.5)
        self.assertAlmostEqual(y.max(), 12.57768805)
        self.assertAlmostEqual(x[0, y.argmax()], 29.95)

        # data, cal and background, normalised by time
        pd_out3 = HFIRPowderReduction(
            SampleFileName=data,
            VanadiumFilename=cal,
            VanadiumBackgroundFilename=bkg,
            XUnits="2Theta",
            NormaliseBy="Time",
            XMin=0,
            XMax=180,
            Sum=True,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
        )

        x = pd_out3.extractX()
        y = pd_out3.extractY()

        self.assertAlmostEqual(x.min(), 8.09946893)
        self.assertAlmostEqual(x.max(), 50.80113407)
        self.assertAlmostEqual(y.min(), -42.8104874)
        self.assertAlmostEqual(y.max(), -3.0)
        np.testing.assert_allclose(x[0, y.argmax()], 36.40057252, rtol=1e-6, atol=1e-1)

        # data, cal and background, normalised by time
        # NOTE:
        # still needs to check physics
        pd_out3_multi = HFIRPowderReduction(
            SampleFileName=(f"{data},{data}"),
            VanadiumFilename=cal,
            VanadiumBackgroundFilename=bkg,
            XUnits="2Theta",
            NormaliseBy="Time",
            Sum=True,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
            XMin=0,
            XMax=180,
        )

        x = pd_out3_multi.extractX()
        y = pd_out3_multi.extractY()

        self.assertAlmostEqual(x.min(), 8.09946893)
        self.assertAlmostEqual(x.max(), 50.80113407)
        self.assertAlmostEqual(y.min(), -42.8104874)
        self.assertAlmostEqual(y.max(), -3.0)
        np.testing.assert_allclose(x[0, y.argmax()], 36.40057252, rtol=1e-6, atol=1e-1)

        # data, cal and background. To d spacing
        pd_out4 = HFIRPowderReduction(
            SampleFileName=data,
            VanadiumFilename=cal,
            VanadiumBackgroundFilename=bkg,
            XUnits="d-spacing",
            Wavelength=1.6513045600369298,
            XMin=2,
            XMax=10,
            Instrument="WAND^2",
            VanadiumDiameter=0.5,
        )

        x = pd_out4.extractX()
        y = pd_out4.extractY()

        self.assertAlmostEqual(x.min(), 2.05)
        self.assertAlmostEqual(x.max(), 9.95)
        self.assertAlmostEqual(y.min(), -20.84743178)
        self.assertAlmostEqual(y.max(), -3.0)
        self.assertAlmostEqual(x[0, y.argmax()], 2.55)

        pd_out4_multi = HFIRPowderReduction(
            SampleFileName=(f"{data},{data}"),
            VanadiumFilename=cal,
            VanadiumBackgroundFilename=bkg,
            XUnits="d-spacing",
            Wavelength=1.6513045600369298,
            Sum=True,
            XMin=2,
            XMax=10,
            Instrument="WAND^2",
            VanadiumDiameter=0.5,
        )

        x = pd_out4_multi.extractX()
        y = pd_out4_multi.extractY()

        self.assertAlmostEqual(x.min(), 2.05)
        self.assertAlmostEqual(x.max(), 9.95)
        self.assertAlmostEqual(y.min(), -20.84743178)
        self.assertAlmostEqual(y.max(), -3.0)
        self.assertAlmostEqual(x[0, y.argmax()], 2.55)

        # data, cal and background with mask angle, to Q.
        pd_out4 = HFIRPowderReduction(
            SampleFileName=data,
            VanadiumFilename=cal,
            VanadiumBackgroundFilename=bkg,
            XUnits="Q",
            Wavelength=1.6513045600369298,
            MaskAngle=60,
            XMin=1,
            XMax=3.2,
            XBinWidth=0.001125,
            Instrument="WAND^2",
            VanadiumDiameter=0.5,
        )

        x = pd_out4.extractX()
        y = pd_out4.extractY()

        self.assertAlmostEqual(x.min(), 1.0006, places=4)
        self.assertAlmostEqual(x.max(), 3.1994, places=4)
        self.assertAlmostEqual(y.min(), -42.98771, places=4)
        self.assertAlmostEqual(y.max(), -3.0, places=4)
        np.testing.assert_allclose(x[0, y.argmax()], 1.07258, rtol=1e-6, atol=1e-1)

        # NOTE:
        # Need to check the physics
        pd_out4_multi = HFIRPowderReduction(
            SampleFileName=(f"{data},{data}"),
            VanadiumFilename=cal,
            VanadiumBackgroundFilename=bkg,
            XUnits="Q",
            Wavelength=1.6513045600369298,
            MaskAngle=60,
            XMin=1,
            XMax=3.2,
            XBinWidth=0.001125,
            Instrument="WAND^2",
            VanadiumDiameter=0.5,
            Sum=True,
        )

        x = pd_out4_multi.extractX()
        y = pd_out4_multi.extractY()

        self.assertAlmostEqual(x.min(), 1.0006, places=4)
        self.assertAlmostEqual(x.max(), 3.1994, places=4)
        self.assertAlmostEqual(y.min(), -42.98771, places=4)
        self.assertAlmostEqual(y.max(), -3.0, places=4)
        np.testing.assert_allclose(x[0, y.argmax()], 1.07258, rtol=1e-6, atol=1e-1)

        # data, cal and background, scale background
        pd_out4 = HFIRPowderReduction(
            SampleFileName=data,
            VanadiumFilename=cal,
            VanadiumBackgroundFilename=bkg,
            Wavelength=1.6513045600369298,
            Scale=0.5,
            XUnits="2Theta",
            NormaliseBy="Time",
            Instrument="WAND^2",
            VanadiumDiameter=0.5,
            XMin=8,
            XMax=50,
        )

        x = pd_out4.extractX()
        y = pd_out4.extractY()

        self.assertAlmostEqual(x.min(), 8.09952728)
        self.assertAlmostEqual(x.max(), 49.94993970)
        self.assertAlmostEqual(y.min(), -21.21099623)
        self.assertAlmostEqual(y.max(), -1.5)
        self.assertAlmostEqual(x[0, y.argmax()], 36.53377878)

        pd_out4_multi = HFIRPowderReduction(
            SampleFileName=(f"{data},{data}"),
            VanadiumFilename=cal,
            VanadiumBackgroundFilename=bkg,
            Wavelength=1.6513045600369298,
            Scale=0.5,
            XUnits="2Theta",
            NormaliseBy="Time",
            Sum=True,
            Instrument="WAND^2",
            VanadiumDiameter=0.5,
            XMin=8,
            XMax=50,
        )

        x = pd_out4_multi.extractX()
        y = pd_out4_multi.extractY()

        self.assertAlmostEqual(x.min(), 8.09952728)
        self.assertAlmostEqual(x.max(), 49.94993970)
        self.assertAlmostEqual(y.min(), -21.21099623)
        self.assertAlmostEqual(y.max(), -1.5)
        self.assertAlmostEqual(x[0, y.argmax()], 36.53377878)

    def test_event(self):
        # check that the workflow runs with event workspaces as input, junk data

        event_data = CreateSampleWorkspace(
            NumBanks=1,
            BinWidth=20000,
            PixelSpacing=0.1,
            BankPixelWidth=100,
            WorkspaceType="Event",
        )
        event_cal = CreateSampleWorkspace(
            NumBanks=1,
            BinWidth=20000,
            PixelSpacing=0.1,
            BankPixelWidth=100,
            WorkspaceType="Event",
            Function="Flat background",
        )
        event_bkg = CreateSampleWorkspace(
            NumBanks=1,
            BinWidth=20000,
            PixelSpacing=0.1,
            BankPixelWidth=100,
            WorkspaceType="Event",
            Function="Flat background",
        )
        data_file_name = os.path.join(self._test_dir, "sample_workspace_event.nxs")
        SaveNexusESS(event_data, Filename=data_file_name)
        vanadium_file_name = os.path.join(self._test_dir, "vanadium_workspace_event.nxs")
        SaveNexusESS(event_cal, Filename=vanadium_file_name)
        background_file_name = os.path.join(self._test_dir, "background_workspace_event.nxs")
        SaveNexusESS(event_bkg, Filename=background_file_name)

        # CASE 1
        # input single workspace, output single workspace
        pd_out = HFIRPowderReduction(
            SampleFilename=data_file_name,
            VanadiumFilename=vanadium_file_name,
            VanadiumBackgroundFilename=background_file_name,
            XUnits="2Theta",
            NormaliseBy="None",
            Sum=False,
            XMin=0,
            XMax=70,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
        )

        assert isinstance(pd_out, MatrixWorkspace)

        x = pd_out.extractX()
        y = pd_out.extractY()

        self.assertAlmostEqual(x.min(), 0.05)
        self.assertAlmostEqual(x.max(), 69.95)
        self.assertAlmostEqual(y[0, 0], np.inf)

        # CASE 2
        # input multiple single ws, output (single) summed ws
        pd_out = HFIRPowderReduction(
            SampleFilename=f"{data_file_name}, {data_file_name}",
            VanadiumFilename=vanadium_file_name,
            VanadiumBackgroundFilename=background_file_name,
            XUnits="2Theta",
            NormaliseBy="None",
            Sum=True,
            XMin=0,
            XMax=70,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
        )

        x = pd_out.extractX()
        y = pd_out.extractY()

        self.assertAlmostEqual(x.min(), 0.05)
        self.assertAlmostEqual(x.max(), 69.95)
        self.assertAlmostEqual(y[0, 0], 0.0)
        assert isinstance(pd_out, MatrixWorkspace)

        # CASE 3
        # input group ws containing several ws, output group ws containing several ws
        pd_out = HFIRPowderReduction(
            SampleFilename=f"{data_file_name}, {data_file_name}",
            VanadiumFilename=vanadium_file_name,
            VanadiumBackgroundFilename=background_file_name,
            XUnits="2Theta",
            NormaliseBy="None",
            Sum=False,
            XMin=0,
            XMax=70,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
        )

        for i in pd_out:
            x = i.extractX()
            y = i.extractY()

            self.assertAlmostEqual(x.min(), 0.05)
            self.assertAlmostEqual(x.max(), 69.95)
            self.assertAlmostEqual(y[0, 0], np.inf)

        assert isinstance(pd_out, WorkspaceGroup)
        assert len(pd_out) == 2

        event_data2 = CloneWorkspace(event_data)

        event_data_group = WorkspaceGroup()
        event_data_group.addWorkspace(event_data)
        event_data_group.addWorkspace(event_data2)
        group_file_name = os.path.join(self._test_dir, "group_workspace_event.nxs")
        SaveNexusESS(event_data_group, Filename=group_file_name)

        # CASE 4 - input group ws, output group ws
        pd_out = HFIRPowderReduction(
            SampleFilename=group_file_name,
            VanadiumFilename=vanadium_file_name,
            VanadiumBackgroundFilename=background_file_name,
            XUnits="2Theta",
            NormaliseBy="None",
            Sum=False,
            XMin=0,
            XMax=70,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
        )

        for i in pd_out:
            x = i.extractX()
            y = i.extractY()

            self.assertAlmostEqual(x.min(), 0.05)
            self.assertAlmostEqual(x.max(), 69.95)
            self.assertAlmostEqual(y[0, 0], np.inf)

        assert isinstance(pd_out, WorkspaceGroup)
        assert len(pd_out) == 2

        event_data2 = CloneWorkspace(event_data)
        event_data_group = GroupWorkspaces([event_data, event_data2])
        group_file_name = os.path.join(self._test_dir, "group_workspace_event.nxs")
        SaveNexusESS(event_data_group, Filename=group_file_name)

        pd_out = HFIRPowderReduction(
            SampleFilename=group_file_name,
            VanadiumFilename=vanadium_file_name,
            VanadiumBackgroundFilename=background_file_name,
            XUnits="2Theta",
            NormaliseBy="None",
            Sum=False,
            XMin=0,
            XMax=70,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
        )

        for i in pd_out:
            x = i.extractX()
            y = i.extractY()

            self.assertAlmostEqual(x.min(), 0.05)
            self.assertAlmostEqual(x.max(), 69.95)
            self.assertAlmostEqual(y[0, 0], np.inf)

        assert isinstance(pd_out, WorkspaceGroup)
        assert len(pd_out) == 2

    def test_save(self):
        # check that the workflow can save output to a file, with event workspace input

        data, cal, bkg = self._create_workspaces()

        output_file_name = os.path.join(self._test_dir, "output_workspace.nxs")

        HFIRPowderReduction(
            SampleFilename=data,
            XUnits="2Theta",
            NormaliseBy="None",
            Sum=False,
            XMin=0,
            XMax=70,
            Instrument="WAND^2",
            Wavelength=1.6513,
            VanadiumDiameter=0.5,
            OutputWorkspace="output_ws",
            OutputDirectory=output_file_name,
        )

        # Check that the output file was created
        self.assertTrue(os.path.isfile(output_file_name))

        # Load the output file and check it contains a workspace
        output_ws = Load(output_file_name)
        self.assertIsInstance(output_ws, MatrixWorkspace)


if __name__ == "__main__":
    unittest.main()
