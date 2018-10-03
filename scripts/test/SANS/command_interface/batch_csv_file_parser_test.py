# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import os
import mantid
from sans.common.enums import BatchReductionEntry
from sans.common.constants import ALL_PERIODS
from sans.command_interface.batch_csv_file_parser import BatchCsvParser


class BatchCsvParserTest(unittest.TestCase):

    @staticmethod
    def _save_to_csv(content):
        test_file_path = os.path.join(mantid.config.getString('defaultsave.directory'), 'sans_batch_test_file.csv')
        BatchCsvParserTest._remove_csv(test_file_path)

        with open(test_file_path, 'w') as f:
            f.write(content)
        return test_file_path

    @staticmethod
    def _remove_csv(test_file_path):
        if os.path.exists(test_file_path):
            os.remove(test_file_path)

    def test_that_raises_when_unknown_keyword_is_used(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,74044,output_as,test,new_key_word,test\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)
        self.assertRaises(RuntimeError, parser.parse_batch_file)
        BatchCsvParserTest._remove_csv(batch_file_path)

    def test_raises_if_the_batch_file_contains_an_uneven_number_of_entries(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,74044,sample_trans,74024,sample_direct_beam,74014,can_sans,74019,can_trans,74020," \
                   "can_direct_beam,output_as, first_eim\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)
        self.assertRaises(RuntimeError, parser.parse_batch_file)
        BatchCsvParserTest._remove_csv(batch_file_path)

    def test_that_raises_when_sample_scatter_is_missing(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,,output_as,test_file\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)
        self.assertRaises(RuntimeError, parser.parse_batch_file)
        BatchCsvParserTest._remove_csv(batch_file_path)

    def test_that_raises_when_output_is_missing(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,test,output_as,\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)
        self.assertRaises(RuntimeError, parser.parse_batch_file)
        BatchCsvParserTest._remove_csv(batch_file_path)

    def test_that_raises_when_sample_transmission_is_specified_incompletely(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,test,output_as,test, sample_trans,test, sample_direct_beam,\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)
        self.assertRaises(RuntimeError, parser.parse_batch_file)
        BatchCsvParserTest._remove_csv(batch_file_path)

    def test_that_raises_when_can_transmission_is_specified_incompletely(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,test,output_as,test, can_trans,, can_direct_beam, test\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)
        self.assertRaises(RuntimeError, parser.parse_batch_file)
        BatchCsvParserTest._remove_csv(batch_file_path)

    def test_that_raises_when_can_transmission_is_specified_but_no_can_scatter(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,test,output_as,test, can_trans,, can_direct_beam, test\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)
        self.assertRaises(RuntimeError, parser.parse_batch_file)
        BatchCsvParserTest._remove_csv(batch_file_path)

    def test_that_parses_two_lines_correctly(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,1,sample_trans,2,sample_direct_beam,3,output_as,test_file,user_file,user_test_file\n" \
                   "sample_sans,1,can_sans,2,output_as,test_file2\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)

        # Act
        output = parser.parse_batch_file()

        # Assert
        self.assertTrue(len(output) == 2)

        first_line = output[0]
        # Should have 5 user specified entries and 3 period entries
        self.assertTrue(len(first_line) == 8)
        self.assertTrue(first_line[BatchReductionEntry.SampleScatter] == "1")
        self.assertTrue(first_line[BatchReductionEntry.SampleScatterPeriod] == ALL_PERIODS)
        self.assertTrue(first_line[BatchReductionEntry.SampleTransmission] == "2")
        self.assertTrue(first_line[BatchReductionEntry.SampleTransmissionPeriod] == ALL_PERIODS)
        self.assertTrue(first_line[BatchReductionEntry.SampleDirect] == "3")
        self.assertTrue(first_line[BatchReductionEntry.SampleDirectPeriod] == ALL_PERIODS)
        self.assertTrue(first_line[BatchReductionEntry.Output] == "test_file")
        self.assertTrue(first_line[BatchReductionEntry.UserFile] == "user_test_file")
        second_line = output[1]

        # Should have 3 user specified entries and 2 period entries
        self.assertTrue(len(second_line) == 5)
        self.assertTrue(second_line[BatchReductionEntry.SampleScatter] == "1")
        self.assertTrue(second_line[BatchReductionEntry.SampleScatterPeriod] == ALL_PERIODS)
        self.assertTrue(second_line[BatchReductionEntry.CanScatter] == "2")
        self.assertTrue(second_line[BatchReductionEntry.CanScatterPeriod] == ALL_PERIODS)
        self.assertTrue(second_line[BatchReductionEntry.Output] == "test_file2")

        BatchCsvParserTest._remove_csv(batch_file_path)

    def test_that_parses_period_selection(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,1p7,can_sans,2P3,output_as,test_file2\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)

        # Act
        output = parser.parse_batch_file()

        # Assert
        self.assertTrue(len(output) == 1)

        first_line = output[0]
        # Should have 5 user specified entries and 3 period entries
        self.assertTrue(len(first_line) == 5)
        self.assertTrue(first_line[BatchReductionEntry.SampleScatter] == "1")
        self.assertTrue(first_line[BatchReductionEntry.SampleScatterPeriod] == 7)
        self.assertTrue(first_line[BatchReductionEntry.CanScatter] == "2")
        self.assertTrue(first_line[BatchReductionEntry.CanScatterPeriod] == 3)
        self.assertTrue(first_line[BatchReductionEntry.Output] == "test_file2")

        BatchCsvParserTest._remove_csv(batch_file_path)

    def test_that_does_not_return_excluded_keywords(self):
        content = "# MANTID_BATCH_FILE add more text here\n" \
                   "sample_sans,1,sample_trans,2,sample_direct_beam,3,output_as,test_file,user_file,user_test_file\n" \
                   "sample_sans,1,can_sans,2,output_as,test_file2,"","", background_sans, background\n"
        batch_file_path = BatchCsvParserTest._save_to_csv(content)
        parser = BatchCsvParser(batch_file_path)

        # Act
        output = parser.parse_batch_file()

        # Assert
        self.assertTrue(len(output) == 2)

        first_line = output[0]
        # Should have 5 user specified entries and 3 period entries
        self.assertTrue(len(first_line) == 8)
        self.assertTrue(first_line[BatchReductionEntry.SampleScatter] == "1")
        self.assertTrue(first_line[BatchReductionEntry.SampleScatterPeriod] == ALL_PERIODS)
        self.assertTrue(first_line[BatchReductionEntry.SampleTransmission] == "2")
        self.assertTrue(first_line[BatchReductionEntry.SampleTransmissionPeriod] == ALL_PERIODS)
        self.assertTrue(first_line[BatchReductionEntry.SampleDirect] == "3")
        self.assertTrue(first_line[BatchReductionEntry.SampleDirectPeriod] == ALL_PERIODS)
        self.assertTrue(first_line[BatchReductionEntry.Output] == "test_file")
        self.assertTrue(first_line[BatchReductionEntry.UserFile] == "user_test_file")
        second_line = output[1]

        # Should have 3 user specified entries and 2 period entries
        self.assertTrue(len(second_line) == 5)
        self.assertTrue(second_line[BatchReductionEntry.SampleScatter] == "1")
        self.assertTrue(second_line[BatchReductionEntry.SampleScatterPeriod] == ALL_PERIODS)
        self.assertTrue(second_line[BatchReductionEntry.CanScatter] == "2")
        self.assertTrue(second_line[BatchReductionEntry.CanScatterPeriod] == ALL_PERIODS)
        self.assertTrue(second_line[BatchReductionEntry.Output] == "test_file2")

        BatchCsvParserTest._remove_csv(batch_file_path)


if __name__ == '__main__':
    unittest.main()
