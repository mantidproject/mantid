# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

import os
import mantid
import unittest
import systemtesting

from sans.common.general_functions import create_unmanaged_algorithm
from sans.common.constants import EMPTY_NAME


# -----------------------------------------------
# Tests for the SANSSave algorithm
# -----------------------------------------------
class SANSSaveTest(unittest.TestCase):
    @staticmethod
    def _get_sample_workspace(with_zero_errors, convert_to_numeric_axis=False):
        create_name = "CreateSimulationWorkspace"
        create_options = {"Instrument": "LARMOR", "BinParams": "1,10,1000", "UnitX": "MomentumTransfer", "OutputWorkspace": EMPTY_NAME}
        create_alg = create_unmanaged_algorithm(create_name, **create_options)
        create_alg.execute()
        workspace = create_alg.getProperty("OutputWorkspace").value

        crop_name = "CropWorkspace"
        crop_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME, "EndWorkspaceIndex": 0}
        crop_alg = create_unmanaged_algorithm(crop_name, **crop_options)
        crop_alg.execute()
        workspace = crop_alg.getProperty("OutputWorkspace").value

        if convert_to_numeric_axis:
            convert_name = "ConvertSpectrumAxis"
            convert_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME, "Target": "ElasticQ", "EFixed": 1}
            convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
            convert_alg.execute()
            workspace = convert_alg.getProperty("OutputWorkspace").value

        if with_zero_errors:
            errors = workspace.dataE(0)
            errors[0] = 0.0
            errors[14] = 0.0
            errors[45] = 0.0
        return workspace

    def _assert_that_file_exists(self, file_name):
        self.assertTrue(os.path.exists(file_name))

    def _remove_file(self, file_name):
        if os.path.exists(file_name):
            os.remove(file_name)

    def test_that_run_number_properties_can_be_set(self):
        # Arrange
        workspace = SANSSaveTest._get_sample_workspace(with_zero_errors=False, convert_to_numeric_axis=True)
        file_name = os.path.join(mantid.config.getString("defaultsave.directory"), "sample_sans_save_file")
        save_name = "SANSSave"
        save_options = {
            "InputWorkspace": workspace,
            "Filename": file_name,
            "UseZeroErrorFree": False,
            "Nexus": False,
            "CanSAS": False,
            "NXCanSAS": True,
            "NistQxy": False,
            "RKH": False,
            "CSV": False,
            "SampleTransmissionRunNumber": "5",
            "SampleDirectRunNumber": "6",
            "CanScatterRunNumber": "7",
            "CanDirectRunNumber": "8",
        }
        try:
            create_unmanaged_algorithm(save_name, **save_options)
        except RuntimeError:
            self.fail("Unable to set properties for SANSSave.")

    def test_that_workspace_can_be_saved_without_zero_error_free_option(self):
        # Arrange
        workspace = SANSSaveTest._get_sample_workspace(with_zero_errors=False, convert_to_numeric_axis=True)
        file_name = os.path.join(mantid.config.getString("defaultsave.directory"), "sample_sans_save_file")
        use_zero_errors_free = False
        save_name = "SANSSave"
        save_options = {
            "InputWorkspace": workspace,
            "Filename": file_name,
            "UseZeroErrorFree": use_zero_errors_free,
            "Nexus": True,
            "CanSAS": True,
            "NXCanSAS": True,
            "NistQxy": True,
            "RKH": True,
            "CSV": True,
        }
        save_alg = create_unmanaged_algorithm(save_name, **save_options)

        # Act
        save_alg.execute()
        self.assertTrue(save_alg.isExecuted())

        # Assert
        expected_files = [
            "sample_sans_save_file.xml",
            "sample_sans_save_file.txt",
            "sample_sans_save_file_nistqxy.dat",
            "sample_sans_save_file.h5",
            "sample_sans_save_file.nxs",
            "sample_sans_save_file.csv",
        ]
        expected_full_file_names = [os.path.join(mantid.config.getString("defaultsave.directory"), elem) for elem in expected_files]
        for file_name in expected_full_file_names:
            self._assert_that_file_exists(file_name)

        # Clean up
        for file_name in expected_full_file_names:
            self._remove_file(file_name)

    def test_that_nistqxy_cannot_be_saved_if_axis_is_spectra_axis(self):
        # Arrange
        workspace = SANSSaveTest._get_sample_workspace(with_zero_errors=False, convert_to_numeric_axis=False)
        file_name = os.path.join(mantid.config.getString("defaultsave.directory"), "sample_sans_save_file")
        use_zero_errors_free = False
        save_name = "SANSSave"
        save_options = {
            "InputWorkspace": workspace,
            "Filename": file_name,
            "UseZeroErrorFree": use_zero_errors_free,
            "Nexus": False,
            "CanSAS": False,
            "NXCanSAS": False,
            "NistQxy": True,
            "RKH": False,
            "CSV": False,
        }
        save_alg = create_unmanaged_algorithm(save_name, **save_options)
        save_alg.setRethrows(True)
        # Act
        try:
            save_alg.execute()
            did_raise = False
        except RuntimeError:
            did_raise = True
        self.assertTrue(did_raise)

    def test_that_if_no_format_is_selected_raises(self):
        # Arrange
        workspace = SANSSaveTest._get_sample_workspace(with_zero_errors=False, convert_to_numeric_axis=True)
        file_name = os.path.join(mantid.config.getString("defaultsave.directory"), "sample_sans_save_file")
        use_zero_errors_free = True
        save_name = "SANSSave"
        save_options = {
            "InputWorkspace": workspace,
            "Filename": file_name,
            "UseZeroErrorFree": use_zero_errors_free,
            "Nexus": False,
            "CanSAS": False,
            "NXCanSAS": False,
            "NistQxy": False,
            "RKH": False,
            "CSV": False,
        }
        save_alg = create_unmanaged_algorithm(save_name, **save_options)
        save_alg.setRethrows(True)
        # Act
        try:
            save_alg.execute()
            did_raise = False
        except RuntimeError:
            did_raise = True
        self.assertTrue(did_raise)

    def test_that_zero_error_is_removed(self):
        # Arrange
        workspace = SANSSaveTest._get_sample_workspace(with_zero_errors=True, convert_to_numeric_axis=True)
        file_name = os.path.join(mantid.config.getString("defaultsave.directory"), "sample_sans_save_file")
        use_zero_errors_free = True
        save_name = "SANSSave"
        save_options = {
            "InputWorkspace": workspace,
            "Filename": file_name,
            "UseZeroErrorFree": use_zero_errors_free,
            "Nexus": True,
            "CanSAS": False,
            "NXCanSAS": False,
            "NistQxy": False,
            "RKH": False,
            "CSV": False,
        }
        save_alg = create_unmanaged_algorithm(save_name, **save_options)

        # Act
        save_alg.execute()
        self.assertTrue(save_alg.isExecuted())
        file_name = os.path.join(mantid.config.getString("defaultsave.directory"), "sample_sans_save_file.nxs")

        load_name = "LoadNexusProcessed"
        load_options = {"Filename": file_name, "OutputWorkspace": EMPTY_NAME}
        load_alg = create_unmanaged_algorithm(load_name, **load_options)
        load_alg.execute()
        reloaded_workspace = load_alg.getProperty("OutputWorkspace").value
        errors = reloaded_workspace.dataE(0)
        # Make sure that the errors are not zero
        self.assertGreater(errors[0], 1.0)
        self.assertGreater(errors[14], 1.0)
        self.assertGreater(errors[45], 1.0)

        # Clean up
        self._remove_file(file_name)


class SANSSaveRunnerTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSSaveTest, "test"))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 1000

    def validate(self):
        return self._success
