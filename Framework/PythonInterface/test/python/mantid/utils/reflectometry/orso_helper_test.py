# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import unittest
from unittest import mock
from mantid.kernel import version
from datetime import datetime, timezone, date, time

from mantid.simpleapi import CreateSampleWorkspace
from mantid.utils.reflectometry.orso_helper import MantidORSODataColumns, MantidORSODataset, MantidORSOSaver
from orsopy.fileio.base import Column, ErrorColumn


class MantidORSODataColumnsTest(unittest.TestCase):
    def test_four_columns_created_if_all_data_supplied(self):
        col_length = 5
        col_values = [1, 2, 3, 4]

        columns = MantidORSODataColumns(
            np.full(col_length, col_values[0]),
            np.full(col_length, col_values[1]),
            np.full(col_length, col_values[2]),
            np.full(col_length, col_values[3]),
        )

        self._check_default_header(columns, len(col_values))
        self._check_column_data(columns, col_values, col_length)

    def test_three_columns_created_if_resolution_not_supplied(self):
        col_length = 5
        col_values = [1, 2, 3]

        columns = MantidORSODataColumns(
            np.full(col_length, col_values[0]), np.full(col_length, col_values[1]), np.full(col_length, col_values[2])
        )

        self._check_default_header(columns, len(col_values))
        self._check_column_data(columns, col_values, col_length)

    def test_columns_with_different_units_and_error_value_types(self):
        col_length = 5
        col_values = [1, 2, 3, 4]

        columns = MantidORSODataColumns(
            np.full(col_length, col_values[0]),
            np.full(col_length, col_values[1]),
            np.full(col_length, col_values[2]),
            np.full(col_length, col_values[3]),
            q_unit=MantidORSODataColumns.Unit.InverseNm,
            r_error_value_is=MantidORSODataColumns.ErrorValue.FWHM,
            q_error_value_is=MantidORSODataColumns.ErrorValue.FWHM,
        )

        self._check_default_header(
            columns,
            len(col_values),
            MantidORSODataColumns.Unit.InverseNm,
            MantidORSODataColumns.ErrorValue.FWHM,
            MantidORSODataColumns.ErrorValue.FWHM,
        )
        self._check_column_data(columns, col_values, col_length)

    def test_error_columns_with_empty_units(self):
        col_length = 5
        col_values = [1, 2, 3, 4]

        columns = MantidORSODataColumns(
            np.full(col_length, col_values[0]),
            np.full(col_length, col_values[1]),
            np.full(col_length, col_values[2]),
            np.full(col_length, col_values[3]),
            q_unit=MantidORSODataColumns.Unit.InverseNm,
            r_error_value_is=None,
            q_error_value_is=None,
        )

        self._check_default_header(
            columns,
            len(col_values),
            MantidORSODataColumns.Unit.InverseNm,
            None,
            None,
        )
        self._check_column_data(columns, col_values, col_length)

    def test_adding_additional_columns(self):
        col_length = 5
        col_values = [1, 2, 3, 4, 5, 6]

        extra_col = ["test_1", "degree", "incident_angle"]
        extra_error_col = ["test_error", MantidORSODataColumns.ErrorType.Uncertainty, MantidORSODataColumns.ErrorValue.FWHM]

        columns = MantidORSODataColumns(
            np.full(col_length, col_values[0]),
            np.full(col_length, col_values[1]),
            np.full(col_length, col_values[2]),
            np.full(col_length, col_values[3]),
        )
        columns.add_column(extra_col[0], extra_col[1], extra_col[2], np.full(col_length, col_values[4]))
        columns.add_error_column(extra_error_col[0], extra_error_col[1], extra_error_col[2], np.full(col_length, col_values[5]))

        self._check_default_header(columns, len(col_values))
        header = columns.header_info
        self._check_column_header(header[4], extra_col[0], extra_col[1], extra_col[2])
        self._check_error_column_header(header[5], extra_error_col[0], extra_error_col[1], extra_error_col[2])
        self._check_column_data(columns, col_values, col_length)

    def test_adding_additional_column_using_unit_enum(self):
        col_length = 5

        extra_col = ["test_1", MantidORSODataColumns.Unit.InverseAngstrom, "angstrom_unit_test"]
        columns = MantidORSODataColumns(
            np.full(col_length, 1),
            np.full(col_length, 1),
            np.full(col_length, 1),
            np.full(col_length, 1),
        )
        columns.add_column(extra_col[0], extra_col[1], extra_col[2], np.full(col_length, 1))

        header = columns.header_info
        self._check_column_header(header[4], extra_col[0], extra_col[1].value, extra_col[2])

    def test_adding_additional_column_with_no_resolution(self):
        col_length = 5
        col_values = [1, 2, 3, np.nan, 5]

        extra_col = ["test_1", "degree", "incident_angle"]

        columns = MantidORSODataColumns(
            np.full(col_length, col_values[0]), np.full(col_length, col_values[1]), np.full(col_length, col_values[2])
        )
        columns.add_column(extra_col[0], extra_col[1], extra_col[2], np.full(col_length, col_values[4]))

        self._check_default_header(columns, len(col_values))
        header = columns.header_info
        self._check_column_header(header[4], extra_col[0], extra_col[1], extra_col[2])
        self._check_column_data(columns, col_values, col_length)

    def test_adding_additional_column_with_no_reflectivity_error(self):
        col_length = 5
        col_values = [1, 2, np.nan, np.nan, 5]

        extra_col = ["test_1", "degree", "incident_angle"]

        columns = MantidORSODataColumns(np.full(col_length, col_values[0]), np.full(col_length, col_values[1]))
        columns.add_column(extra_col[0], extra_col[1], extra_col[2], np.full(col_length, col_values[4]))

        self._check_default_header(columns, len(col_values))
        header = columns.header_info
        self._check_column_header(header[4], extra_col[0], extra_col[1], extra_col[2])
        self._check_column_data(columns, col_values, col_length)

    def _check_default_header(
        self,
        columns,
        num_columns_expected,
        q_unit=MantidORSODataColumns.Unit.InverseAngstrom,
        r_error_is=MantidORSODataColumns.ErrorValue.Sigma,
        q_error_is=MantidORSODataColumns.ErrorValue.Sigma,
    ):
        header = columns.header_info
        self.assertIsNotNone(header)

        self.assertEqual(num_columns_expected, len(header), "Incorrect number of column headers")

        self._check_column_header(header[0], MantidORSODataColumns.LABEL_Q, q_unit.value, MantidORSODataColumns.QUANTITY_Q)
        self._check_column_header(header[1], MantidORSODataColumns.LABEL_REFLECTIVITY, None, MantidORSODataColumns.QUANTITY_REFLECTIVITY)

        self._check_error_column_header(
            header[2], MantidORSODataColumns.LABEL_REFLECTIVITY, MantidORSODataColumns.ErrorType.Uncertainty, r_error_is
        )
        if num_columns_expected >= 4:
            self._check_error_column_header(
                header[3], MantidORSODataColumns.LABEL_Q, MantidORSODataColumns.ErrorType.Resolution, q_error_is
            )

    def _check_column_header(self, column, label, unit, quantity):
        self.assertIsInstance(column, Column)
        self.assertEqual(label, column.name)
        self.assertEqual(unit, column.unit)
        self.assertEqual(quantity, column.physical_quantity)

    def _check_error_column_header(self, column, error_of, error_is, value_is):
        self.assertIsInstance(column, ErrorColumn)
        self.assertEqual(error_of, column.error_of)
        self.assertEqual(error_is.value, column.error_type)
        self.assertEqual(None if value_is is None else value_is.value, column.value_is)

    def _check_column_data(self, columns, col_values, col_length):
        """Checks the data returned from the MantidORSODataColumns class.

        :param columns: the columns object to check.
        :param col_values: a list containing the first value we expect to find in each column of the data.
        :param col_length: the number of values we expect to find in each column (this will be the same for all columns).
        """

        data = columns.data
        self.assertIsNotNone(data)
        # Check the number of columns
        self.assertEqual(len(col_values), len(data[0]), "Incorrect number of data columns")
        # Check the number of values in a column
        self.assertEqual(col_length, len(data), "Incorrect number of values in the data columns")

        # Check that each column contains the expected sample value
        for i, value in enumerate(col_values):
            if np.isnan(value):
                self.assertTrue(np.isnan(data[0][i]), "Incorrect value in data column")
            else:
                self.assertEqual(value, data[0][i], "Incorrect value in data column")


class MantidORSODatasetTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        ws = CreateSampleWorkspace()
        cls._ws = ws
        cls._data_columns = MantidORSODataColumns(np.full(5, 1), np.full(5, 2), np.full(5, 3), np.full(5, 4))

    def test_create_mantid_orso_dataset(self):
        dataset_name = "Test dataset"
        reduction_timestamp = datetime.now()
        creator_name = "Creator Name"
        creator_affiliation = "Creator Affiliation"
        dataset = MantidORSODataset(
            dataset_name, self._data_columns, self._ws, reduction_timestamp, creator_name, creator_affiliation, polarized_dataset=False
        )

        orso_dataset = dataset.dataset
        self.assertIsNotNone(orso_dataset)
        self._check_mantid_default_header(orso_dataset, dataset_name, self._ws, reduction_timestamp, creator_name, creator_affiliation)
        self._check_data(orso_dataset)

    def test_set_facility_on_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        facility_name = "ISIS"
        dataset.set_facility(facility_name)

        self.assertEqual(facility_name, dataset.dataset.info.data_source.experiment.facility)

    def test_set_proposal_id_on_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        proposal_id = "123456"
        dataset.set_proposal_id(proposal_id)

        self.assertEqual(proposal_id, dataset.dataset.info.data_source.experiment.proposalID)

    def test_set_doi_on_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        doi = "10.5286/ISIS.E.RB123456"
        dataset.set_doi(doi)

        self.assertEqual(doi, dataset.dataset.info.data_source.experiment.doi)

    def test_set_polarization_on_mantid_orso_dataset(self):
        dataset = self._create_test_dataset(polarized=True)
        pol = "pp"
        dataset.set_polarization(pol)

        self.assertEqual("pp", dataset.dataset.info.data_source.measurement.instrument_settings.polarization)

    def test_set_reduction_call_on_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        call = "ReflectometryReductionCall"
        dataset.set_reduction_call(call)

        self.assertEqual(call, dataset.dataset.info.reduction.call)

    def test_create_local_datetime_from_utc_string(self):
        date_string = "2023-10-18T12:30:45.12345"
        expected_value = datetime.combine(date(2023, 10, 18), time(12, 30, 45)).replace(tzinfo=timezone.utc).astimezone(tz=None)
        self.assertEqual(expected_value, MantidORSODataset.create_local_datetime_from_utc_string(date_string))

    def test_create_local_datetime_from_utc_string_with_invalid_format_raises_error(self):
        with self.assertRaisesRegex(ValueError, "Cannot parse datetime string"):
            MantidORSODataset.create_local_datetime_from_utc_string("23-10-18T12:30:45")

    def test_add_data_file_to_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        filename = "test.nxs"
        timestamp = datetime.now()
        comment = "A test comment"

        self._check_num_data_files(dataset, 0)
        self._check_num_additional_files(dataset, 0)

        dataset.add_measurement_data_file(filename, timestamp, comment)

        self._check_files_are_created(dataset, [filename], 1, 0, True)
        data_file = dataset.dataset.info.data_source.measurement.data_files[0]
        self.assertEqual(timestamp, data_file.timestamp)
        self.assertEqual(comment, data_file.comment)

    def test_add_data_file_with_name_only_to_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        filename = "test.nxs"

        self._check_num_data_files(dataset, 0)
        self._check_num_additional_files(dataset, 0)

        dataset.add_measurement_data_file(filename)

        self._check_files_are_created(dataset, [filename], 1, 0, True)
        data_file = dataset.dataset.info.data_source.measurement.data_files[0]
        self.assertEqual(None, data_file.timestamp)
        self.assertEqual(None, data_file.comment)

    def test_add_multiple_data_files_to_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        filenames = ["test1.nxs", "test2.nxs", "test3.nxs"]

        self._check_num_data_files(dataset, 0)
        self._check_num_additional_files(dataset, 0)

        for filename in filenames:
            dataset.add_measurement_data_file(filename)

        self._check_files_are_created(dataset, filenames, len(filenames), 0, True)

    def test_add_additional_file_to_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        filename = "test_additional.nxs"
        timestamp = datetime.now()
        comment = "A test comment"

        self._check_num_data_files(dataset, 0)
        self._check_num_additional_files(dataset, 0)

        dataset.add_measurement_additional_file(filename, timestamp, comment)

        self._check_files_are_created(dataset, [filename], 0, 1, False)
        data_file = dataset.dataset.info.data_source.measurement.additional_files[0]
        self.assertEqual(timestamp, data_file.timestamp)
        self.assertEqual(comment, data_file.comment)

    def test_add_additional_file_with_name_only_to_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        filename = "test_additional.nxs"

        self._check_num_data_files(dataset, 0)
        self._check_num_additional_files(dataset, 0)

        dataset.add_measurement_additional_file(filename)

        self._check_files_are_created(dataset, [filename], 0, 1, False)
        data_file = dataset.dataset.info.data_source.measurement.additional_files[0]
        self.assertEqual(None, data_file.timestamp)
        self.assertEqual(None, data_file.comment)

    def test_add_multiple_additional_files_to_mantid_orso_dataset(self):
        dataset = self._create_test_dataset()
        filenames = ["test_additional1.nxs", "test_additional2.nxs", "test_additional3.nxs"]

        self._check_num_data_files(dataset, 0)
        self._check_num_additional_files(dataset, 0)

        for filename in filenames:
            dataset.add_measurement_additional_file(filename)

        self._check_files_are_created(dataset, filenames, 0, len(filenames), False)

    def _create_test_dataset(self, polarized=False):
        return MantidORSODataset("Test dataset", self._data_columns, self._ws, datetime.now(), "", "", polarized_dataset=polarized)

    def _check_mantid_default_header(
        self, orso_dataset, dataset_name, ws, reduction_timestamp, creator_name, creator_affiliation, polarization=False
    ):
        """Check that the default header contains only the information that can be shared
        for all Mantid implementations of the standard
        """
        expected_header = (
            "data_source:\n"
            "  owner:\n"
            "    name: null\n"
            "    affiliation: null\n"
            "  experiment:\n"
            "    title: null\n"
            f"    instrument: {ws.getInstrument().getName()}\n"
            "    start_date: 2010-01-01T00:00:00\n"
            "    probe: neutron\n"
            "  sample:\n"
            f"    name: {ws.getTitle()}\n"
            "  measurement:\n"
            "    instrument_settings: null\n"
            "    data_files: []\n"
            "reduction:\n"
            f"  software: {{name: {MantidORSODataset.SOFTWARE_NAME}, version: {version()}}}\n"
            f"  timestamp: {reduction_timestamp.isoformat()}\n"
            "  creator:\n"
            f"    name: {creator_name}\n"
            f"    affiliation: {creator_affiliation}\n"
            f"data_set: {dataset_name}\n"
            "columns:\n"
            "- {name: Qz, unit: 1/angstrom, physical_quantity: normal_wavevector_transfer}\n"
            "- {name: R, physical_quantity: reflectivity}\n"
            "- {error_of: R, error_type: uncertainty, value_is: sigma}\n"
            "- {error_of: Qz, error_type: resolution, value_is: sigma}\n"
            "# Qz (1/angstrom)    R                      sR                     sQz                   "
        )

        self.assertEqual(expected_header, orso_dataset.header())

    def _check_num_data_files(self, dataset, expected_number):
        self.assertTrue(len(dataset.dataset.info.data_source.measurement.data_files) == expected_number)

    def _check_num_additional_files(self, dataset, expected_number):
        additional_files = dataset.dataset.info.data_source.measurement.additional_files
        if expected_number == 0:
            self.assertIsNone(additional_files)
        else:
            self.assertTrue(len(additional_files) == expected_number)

    def _check_files_are_created(self, dataset, filenames, num_data_files, num_additional_files, is_data_files):
        self._check_num_data_files(dataset, num_data_files)
        self._check_num_additional_files(dataset, num_additional_files)

        created_files = (
            dataset.dataset.info.data_source.measurement.data_files
            if is_data_files
            else dataset.dataset.info.data_source.measurement.additional_files
        )
        for i, filename in enumerate(filenames):
            created_file = created_files[i]
            self.assertEqual(filename, created_file.file)

    def _check_data(self, orso_dataset):
        self.assertTrue(np.array_equal(self._data_columns.data, orso_dataset.data))


class MantidORSOSaverTest(unittest.TestCase):
    @mock.patch("mantid.utils.reflectometry.orso_helper.save_orso")
    def test_save_orso_ascii_with_no_filename_extension(self, mock_save_orso):
        file_name = "Test file"
        saver = MantidORSOSaver(file_name)
        mock_dataset = self._add_mock_dataset(saver)

        saver.save_orso_ascii()
        self._assert_save_method_called(mock_save_orso, mock_dataset, file_name=f"{file_name}{MantidORSOSaver.ASCII_FILE_EXT}")

    @mock.patch("mantid.utils.reflectometry.orso_helper.save_orso")
    def test_save_orso_ascii_with_filename_extension(self, mock_save_orso):
        file_name = f"Test file{MantidORSOSaver.ASCII_FILE_EXT}"
        saver = MantidORSOSaver(file_name)
        mock_dataset = self._add_mock_dataset(saver)

        saver.save_orso_ascii()
        self._assert_save_method_called(mock_save_orso, mock_dataset, file_name)

    @mock.patch("mantid.utils.reflectometry.orso_helper.save_orso")
    def test_save_orso_ascii_with_comment(self, mock_save_orso):
        file_name = f"Test file{MantidORSOSaver.ASCII_FILE_EXT}"
        comment = "Test comment at top of file"
        saver = MantidORSOSaver(file_name, comment)
        mock_dataset = self._add_mock_dataset(saver)

        saver.save_orso_ascii()
        self._assert_save_method_called(mock_save_orso, mock_dataset, file_name, comment)

    @mock.patch("mantid.utils.reflectometry.orso_helper.save_nexus")
    def test_save_orso_nexus_with_no_filename_extension(self, mock_save_nexus):
        file_name = "Test file"
        saver = MantidORSOSaver(file_name)
        mock_dataset = self._add_mock_dataset(saver)

        saver.save_orso_nexus()
        self._assert_save_method_called(mock_save_nexus, mock_dataset, file_name=f"{file_name}{MantidORSOSaver.NEXUS_FILE_EXT}")

    @mock.patch("mantid.utils.reflectometry.orso_helper.save_nexus")
    def test_save_orso_nexus_with_filename_extension(self, mock_save_nexus):
        file_name = f"Test file{MantidORSOSaver.NEXUS_FILE_EXT}"
        saver = MantidORSOSaver(file_name)
        mock_dataset = self._add_mock_dataset(saver)

        saver.save_orso_nexus()
        self._assert_save_method_called(mock_save_nexus, mock_dataset, file_name)

    @mock.patch("mantid.utils.reflectometry.orso_helper.save_nexus")
    def test_save_orso_nexus_with_comment(self, mock_save_nexus):
        file_name = f"Test file{MantidORSOSaver.NEXUS_FILE_EXT}"
        comment = "Test comment at top of file"
        saver = MantidORSOSaver(file_name, comment)
        mock_dataset = self._add_mock_dataset(saver)

        saver.save_orso_nexus()
        self._assert_save_method_called(mock_save_nexus, mock_dataset, file_name, comment)

    def test_is_supported_extension(self):
        test_cases = [
            (f"filename{MantidORSOSaver.ASCII_FILE_EXT}", True),
            (f"filename{MantidORSOSaver.NEXUS_FILE_EXT}", True),
            ("filename.unsupported", False),
            ("filename", False),
            ("", False),
        ]
        for extension, expected_result in test_cases:
            with self.subTest(extension=extension, expected_result=expected_result):
                self.assertEqual(MantidORSOSaver.is_supported_extension(extension), expected_result)

    @staticmethod
    def _add_mock_dataset(saver):
        mock_dataset = mock.Mock()
        mock_dataset.dataset = mock.Mock()
        saver.add_dataset(mock_dataset)

        return mock_dataset

    @staticmethod
    def _assert_save_method_called(mock_save_method, mock_dataset, file_name, comment=None):
        mock_save_method.assert_called_once_with(datasets=[mock_dataset.dataset], fname=file_name, comment=comment)


if __name__ == "__main__":
    unittest.main()
