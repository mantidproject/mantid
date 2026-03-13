# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re
from Engineering.common.instrument_config import get_instr_config

ENGINX_MAX_SPECTRA = 2400  # 2512 spectra appear in the ws. But from testing, anything > 2400 doesn't work.
VALID_PUNCT = [",", " ", "-"]
SPLITTING_REGEX = ",|-"


class CroppingModel(object):
    def validate_and_clean_spectrum_numbers(self, numbers):
        numbers = numbers.strip()
        try:
            if self.validate_spectrum_numbers(numbers):
                numbers = self._clean_spectrum_numbers(numbers)
                return "", numbers
            else:
                return "Invalid spectrum numbers entered. Limits are 1-" + str(ENGINX_MAX_SPECTRA), ""
        except ValueError as e:
            return str(e), ""

    def validate_spectrum_numbers(self, numbers):
        if self._validate_numeric_or_valid_punct(numbers):
            if "-" in numbers or "," in numbers:
                return self._validate_spectra_list(numbers)
            else:
                return self.validate_spectrum(numbers)
        return False

    @staticmethod
    def _validate_numeric_or_valid_punct(string):
        if all(c.isdigit() or c in VALID_PUNCT for c in string):
            return True
        else:
            raise ValueError("Invalid characters entered. Only numeric characters, ',', and '-' are allowed.")

    def _validate_spectra_list(self, numbers):
        numbers = re.split(SPLITTING_REGEX, numbers)
        return all(self.validate_spectrum(i) for i in numbers)

    @staticmethod
    def validate_spectrum(number):
        number = number.strip()
        return number.isdigit() and 1 <= int(number) <= ENGINX_MAX_SPECTRA

    def _clean_spectrum_numbers(self, numbers):
        numbers = [word.strip() for word in numbers.split(",")]
        return ",".join([self._clean_ranges(i) for i in numbers])

    @staticmethod
    def _clean_ranges(word):
        if "-" in word:
            nums = word.split("-")
            num1, num2 = (i.strip() for i in nums)
            if num1 and num2:
                if int(num1) > int(num2):
                    word = "-".join([num2, num1])
                elif int(num1) < int(num2):
                    word = "-".join([num1, num2])
                else:  # Not a valid range
                    raise ValueError("Ranges cannot contain the same value twice. Invalid Range: " + word)
        return word

    @staticmethod
    def get_cropping_options(instrument):
        config = get_instr_config(instrument)
        return config.interactive_grouping_options
