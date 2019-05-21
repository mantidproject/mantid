# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import re
from csv import reader
from sans.common.enums import BatchReductionEntry
from sans.common.file_information import find_full_file_path
from sans.common.constants import ALL_PERIODS


class BatchCsvParser(object):
    batch_file_keywords = {"sample_sans": BatchReductionEntry.SampleScatter,
                           "output_as": BatchReductionEntry.Output,
                           "sample_trans": BatchReductionEntry.SampleTransmission,
                           "sample_direct_beam": BatchReductionEntry.SampleDirect,
                           "can_sans": BatchReductionEntry.CanScatter,
                           "can_trans": BatchReductionEntry.CanTransmission,
                           "can_direct_beam": BatchReductionEntry.CanDirect,
                           "user_file": BatchReductionEntry.UserFile}
    batch_file_keywords_which_are_dropped = {"background_sans": None,
                                             "background_trans": None,
                                             "background_direct_beam": None,
                                             "": None}

    data_keys = {BatchReductionEntry.SampleScatter: BatchReductionEntry.SampleScatterPeriod,
                 BatchReductionEntry.SampleTransmission: BatchReductionEntry.SampleTransmissionPeriod,
                 BatchReductionEntry.SampleDirect: BatchReductionEntry.SampleDirectPeriod,
                 BatchReductionEntry.CanScatter: BatchReductionEntry.CanScatterPeriod,
                 BatchReductionEntry.CanTransmission: BatchReductionEntry.CanTransmissionPeriod,
                 BatchReductionEntry.CanDirect: BatchReductionEntry.CanDirectPeriod}

    def __init__(self, batch_file_name):
        super(BatchCsvParser, self).__init__()
        # Get the full file path
        self._batch_file_name = find_full_file_path(batch_file_name)
        if not self._batch_file_name:
            raise RuntimeError("batch_csv_file_parser: Could not find specified batch file. Make sure it is available"
                               "in the Mantid path settings.")

    def parse_batch_file(self):
        """
        Parses the batch csv file and returns the elements in a parsed form

        Returns: parsed csv elements
        """

        parsed_rows = []

        with open(self._batch_file_name, 'r') as csvfile:
            batch_reader = reader(csvfile, delimiter=",")
            row_number = 0
            for row in batch_reader:
                # Check if the row is empty
                if not row:
                    continue

                # If the first element contains a # symbol then ignore it
                if "MANTID_BATCH_FILE" in row[0]:
                    continue

                # Else we perform a parse of the row
                parsed_row = self._parse_row(row, row_number)
                parsed_rows.append(parsed_row)
                row_number += 1
        return parsed_rows

    def _parse_row(self, row, row_number):
        # Clean all elements of the row
        row = list(map(str.strip, row))

        # If the reader has ignored the final empty row, we add it back here
        if len(row) == 15 and row[-1] in self.batch_file_keywords.keys():
            row.append("")

        # Go sequentially through the row with a stride of two. The user can either leave entries away, or he can leave
        # them blank, ie ... , sample_direct_beam, , can_sans, XXXXX, ...  or even ..., , ,...
        # This means we expect an even length of entries
        if len(row) % 2 != 0:
            raise RuntimeError("We expect an even number of entries, but row {0} has {1} entries.".format(row_number,
                                                                                                          len(row)))
        output = {}
        # Special attention has to go to the specification of the period in a run number. The user can
        # specify something like 5512p for sample scatter. This means she wants run number 5512 with period 7.
        for key, value in zip(row[::2], row[1::2]):
            if key in list(BatchCsvParser.batch_file_keywords.keys()):
                new_key = BatchCsvParser.batch_file_keywords[key]
                value = value.strip()
                if BatchCsvParser._is_data_entry(new_key):
                    run_number, period, period_key = BatchCsvParser._get_run_number_and_period(new_key, value)
                    output.update({new_key: run_number})
                    output.update({period_key: period})
                else:
                    output.update({new_key: value})
            elif key in list(self.batch_file_keywords_which_are_dropped.keys()):
                continue
            else:
                raise RuntimeError("The key {0} is not part of the SANS batch csv file keywords".format(key))

        # Ensure that sample_scatter was set
        if BatchReductionEntry.SampleScatter not in output or not output[BatchReductionEntry.SampleScatter]:
            raise RuntimeError("The sample_scatter entry in row {0} seems to be missing.".format(row_number))

        # Ensure that the transmission data for the sample is specified either completely or not at all.
        has_sample_transmission = BatchReductionEntry.SampleTransmission in output and \
                                  output[BatchReductionEntry.SampleTransmission]  # noqa
        has_sample_direct_beam = BatchReductionEntry.SampleDirect in output and output[BatchReductionEntry.SampleDirect]

        if (not has_sample_transmission and has_sample_direct_beam) or \
                (has_sample_transmission and not has_sample_direct_beam):
            raise RuntimeError("Inconsistent sample transmission settings in row {0}. Either both the transmission "
                               "and the direct beam run are set or none.".format(row_number))

        # Ensure that the transmission data for the can is specified either completely or not at all.
        has_can_transmission = BatchReductionEntry.CanTransmission in output and \
                               output[BatchReductionEntry.CanTransmission]  # noqa
        has_can_direct_beam = BatchReductionEntry.CanDirect in output and output[BatchReductionEntry.CanDirect]

        if (not has_can_transmission and has_can_direct_beam) or \
                (has_can_transmission and not has_can_direct_beam):
            raise RuntimeError("Inconsistent can transmission settings in row {0}. Either both the transmission "
                               "and the direct beam run are set or none.".format(row_number))

        # Ensure that can scatter is specified if the transmissions are set
        has_can_scatter = BatchReductionEntry.CanScatter in output and output[BatchReductionEntry.CanScatter]
        if not has_can_scatter and has_can_transmission:
            raise RuntimeError("The can transmission was set but not the scatter file in row {0}.".format(row_number))
        return output

    @staticmethod
    def _is_data_entry(entry):
        data_entry_keys = list(BatchCsvParser.data_keys.keys())
        for data_enum in data_entry_keys:
            if entry is data_enum:
                return True
        return False

    @staticmethod
    def _get_run_number_and_period(data_type, entry):
        """
        Gets the run number and the period from a csv data entry.

        @patam data_type: the type of data entry, e.g. BatchReductionEntry.SampleScatter
        @param entry: a data entry, e.g. 5512 or 5512p7
        @return: the run number, the period selection and the corresponding key word
        """
        data_period_type = BatchCsvParser.data_keys[data_type]

        # Slice off period if it exists. If it does not exist, then the period is ALL_PERIODS
        period_pattern = "[p,P][0-9]$"

        has_period = re.search(period_pattern, entry)

        period = ALL_PERIODS
        run_number = entry
        if has_period:
            run_number = re.sub(period_pattern, "", entry)
            period_partial = re.sub(run_number, "", entry)
            period = re.sub("[p,P]", "", period_partial)
            period = int(period)

        return run_number, period, data_period_type
