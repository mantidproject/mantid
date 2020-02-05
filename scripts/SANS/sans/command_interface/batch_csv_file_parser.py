# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import re
from csv import reader
from enum import Enum

from sans.common.constants import ALL_PERIODS
from sans.common.file_information import find_full_file_path
from sans.gui_logic.models.RowEntries import RowEntries


class BatchFileKeywords(Enum):
    SAMPLE_SCATTER = "sample_sans"
    OUTPUT = "output_as"
    SAMPLE_TRANSMISSION = "sample_trans"
    SAMPLE_DIRECT = "sample_direct_beam"
    CAN_SCATTER = "can_sans"
    CAN_TRANSMISSION = "can_trans"
    CAN_DIRECT = "can_direct_beam"
    USER_FILE = "user_file"
    SAMPLE_THICKNESS = "sample_thickness"
    SAMPLE_HEIGHT = "sample_height"
    SAMPLE_WIDTH = "sample_width"


class BatchCsvParser(object):

    IGNORED_KEYWORDS = {"background_sans", "background_trans", "background_direct_beam", ""}
    COMMENT_KEWORDS = {'#', "MANTID_BATCH_FILE"}

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
            read_rows = list(batch_reader)

        for row_number, row in enumerate(read_rows):
            # Check if the row is empty or is a comment
            if not row:
                continue

            key = row[0].strip()
            if key in self.COMMENT_KEWORDS:
                continue

            # Else we perform a parse of the row
            parsed_row = self._parse_row(row, row_number)
            parsed_rows.append(parsed_row)
        return parsed_rows

    def _parse_row(self, row, row_number):
        # Clean all elements of the row
        row = list(map(str.strip, row))
        output = RowEntries()
        # Special attention has to go to the specification of the period in a run number. The user can
        # specify something like 5512p for sample scatter. This means they want run number 5512 with period 7.
        for key, value in zip(row[::2], row[1::2]):
            if key in self.IGNORED_KEYWORDS:
                continue

            try:
                key_enum = BatchFileKeywords(key)
            except ValueError:
                raise KeyError("The key {0} is not part of the SANS batch csv file keywords".format(key))

            try:
                _ = BatchFileKeywords(value)
                raise KeyError("The value {0} is a keyword, you may have missed a comma after {1}".format(value, key))
            except ValueError:
                pass  # User did not accidentally use a key as a value

            self._parse_row_entry(key_enum, value, row_entry=output)

        self.validate_output(output, row_number)

        return output

    def _parse_row_entry(self, key_enum, value, row_entry):
        value = value.strip()
        if key_enum is BatchFileKeywords.CAN_DIRECT:
            run_number, period = self._get_run_number_and_period(value)
            row_entry.can_direct = run_number
            row_entry.can_direct_period = period

        elif key_enum is BatchFileKeywords.CAN_SCATTER:
            run_number, period = self._get_run_number_and_period(value)
            row_entry.can_scatter = run_number
            row_entry.can_scatter_period = period

        elif key_enum is BatchFileKeywords.CAN_TRANSMISSION:
            run_number, period = self._get_run_number_and_period(value)
            row_entry.can_transmission = run_number
            row_entry.can_transmission_period = period

        elif key_enum is BatchFileKeywords.SAMPLE_DIRECT:
            run_number, period = self._get_run_number_and_period(value)
            row_entry.sample_direct = run_number
            row_entry.sample_direct_period = period

        elif key_enum is BatchFileKeywords.SAMPLE_SCATTER:
            run_number, period = self._get_run_number_and_period(value)
            row_entry.sample_scatter = run_number
            row_entry.sample_scatter_period = period

        elif key_enum is BatchFileKeywords.SAMPLE_TRANSMISSION:
            run_number, period = self._get_run_number_and_period(value)
            row_entry.sample_transmission = run_number
            row_entry.sample_transmission_period = period

        elif key_enum is BatchFileKeywords.SAMPLE_HEIGHT:
            row_entry.sample_height = value
        elif key_enum is BatchFileKeywords.SAMPLE_THICKNESS:
            row_entry.sample_thickness = value
        elif key_enum is BatchFileKeywords.SAMPLE_WIDTH:
            row_entry.sample_width = value

        elif key_enum is BatchFileKeywords.OUTPUT:
            row_entry.output_name = value
        elif key_enum is BatchFileKeywords.USER_FILE:
            row_entry.user_file = value

        else:
            raise RuntimeError("Batch Enum key {0} has not been handled".format(key_enum))

    @staticmethod
    def _get_run_number_and_period(entry):
        """
        Gets the run number and the period from a csv data entry.
        @param entry: a data entry, e.g. 5512 or 5512p7
        @return: the run number, the period selection and the corresponding key word
        """
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

        return run_number, period

    @staticmethod
    def validate_output(output, row_number):
        # Ensure that sample_scatter was set
        if not output.sample_scatter:
            raise ValueError("The sample_scatter entry in row {0} seems to be missing.".format(row_number))
        if bool(output.sample_transmission) != bool(output.sample_direct):
            raise ValueError("Inconsistent sample transmission settings in row {0}. Either both the transmission "
                               "and the direct beam run are set or none.".format(row_number))
        if bool(output.can_transmission) != bool(output.can_direct):
            raise ValueError("Inconsistent can transmission settings in row {0}. Either both the transmission "
                               "and the direct beam run are set or none.".format(row_number))

        # Ensure that can scatter is specified if the transmissions are set
        if bool(output.can_transmission) and not bool(output.can_scatter):
            raise ValueError("The can transmission was set but not the scatter file in row {0}.".format(row_number))
