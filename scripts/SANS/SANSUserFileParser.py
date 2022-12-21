# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from collections import namedtuple
import re

# A settings tuple for dark runs.
DarkRunSettings = namedtuple("DarkRunSettings", "run_number time mean mon mon_number")

# pylint: disable=too-many-instance-attributes


class BackCommandParser(object):
    def __init__(self):
        super(BackCommandParser, self).__init__()
        self._uniform_key = ["TIME", "UAMP"]
        self._mean_key = ["MEAN", "TOF"]
        self._run_key = ["RUN="]

        # Standard because they all have the same pattern, eg /TIME/MEAN/RUN=1234
        self._first_level_keys_standard = self._uniform_key

        # Special because they are not standard, eg MON/RUN=1234/MEAN/TIME or M4/RUN=1234/MEAN/TIME
        self._first_level_keys_special = ["MON", "^M[1-9][0-9]*$"]

        # Evaluation chains
        self._standard_chain = [self._first_level_keys_standard, self._mean_key, self._run_key]
        self._special_chain = [self._first_level_keys_special, self._run_key, self._uniform_key, self._mean_key]
        self._evaluation_chain = None

        # Parse results
        self._use_mean = None
        self._use_time = None
        self._mon = False  # This is not being parsed by the standard chain
        self._run_number = None
        self._mon_number = None

        # Key - method mapping
        self._method_map = {
            "".join(self._first_level_keys_standard): self._evaluate_uniform,
            "".join(self._first_level_keys_special): self._evaluate_mon,
            "".join(self._uniform_key): self._evaluate_uniform,
            "".join(self._mean_key): self._evaluate_mean,
            "".join(self._run_key): self._evaluate_run,
        }

    def _reset_parse_results(self):
        self._use_mean = None
        self._use_time = None
        self._mon = False  # This is not being parsed by the standard chain
        self._run_number = None
        self._mon_number = None

    def _get_method(self, key_list):
        return self._method_map["".join(key_list)]

    def _evaluate_mean(self, argument):
        """
        Evaluates if the argument is either MEAN, TOF or something else.
        @param argument: string to investigate
        @raise RuntimeError: If the argument cannot be parsed correctly
        """
        if argument == self._mean_key[0]:
            self._use_mean = True
        elif argument == self._mean_key[1]:
            self._use_mean = False
        else:
            raise RuntimeError(
                "BackCommandParser: Cannot parse the MEAN/TOF value. " "Read in " + argument + ". " + "Make sure it is set correctly."
            )

    def _evaluate_uniform(self, argument):
        """
        Evaluates if the argument is either TIME, UAMP or something else.
        @param argument: string to investigate
        @raise RuntimeError: If the argument cannot be parsed correctly
        """
        if argument == self._uniform_key[0]:
            self._use_time = True
        elif argument == self._uniform_key[1]:
            self._use_time = False
        else:
            raise RuntimeError(
                "BackCommandParser: Cannot parse the TIME/UAMP value. " "Read in " + argument + ". " + "Make sure it is set correctly."
            )

    def _evaluate_run(self, argument):
        """
        Evaluates if the argument is RUN=
        @param argument: string to investigate
        @raise RuntimeError: If the argument cannot be parsed correctly
        """
        if not argument.startswith(self._run_key[0]):
            raise RuntimeError(
                "BackCommandParser: Cannot parse the RUN= value. " "Read in " + argument + ". " + "Make sure it is set correctly."
            )

        # Remove the Run= part and take the rest as the run parameter. At this point we cannot
        # check if it is a valid run
        self._run_number = argument.replace(self._run_key[0], "")

    def _evaluate_mon(self, argument):
        """
        Evaluates which detector to use. At this point the validty of this has already been checked, so
        we can just take it as is.
        @param argument: string to investigate
        @raise RuntimeError: If the argument cannot be parsed correctly
        """
        if argument == self._first_level_keys_special[0]:  # Check if MON
            self._mon = True
        elif re.match(self._first_level_keys_special[1], argument):
            self._mon = True
            mon_number = argument.replace("M", "")
            self._mon_number = int(mon_number)
        else:
            raise RuntimeError(
                "BackCommandParser: Cannot parse the MON value. " "Read in " + argument + ". " + "Make sure it is set correctly."
            )

    def can_attempt_to_parse(self, arguments):
        """
        Check if the parameters can be parsed with this class
        @param param: a string to be parsed
        @returns true if it can be parsed
        """
        # Convert to capital and split the string
        to_check = self._prepare_argument(arguments)

        # We expect 3 arguemnts for the standard chain and 4 arguments
        # for the special monitor chain
        if len(to_check) != 4 and len(to_check) != 3:
            return False

        can_parse = False
        # Check if part of the first entry of the standard chain
        if self._is_parsable_with_standard_chain(to_check):
            self._evaluation_chain = self._standard_chain
            can_parse = True
        elif self._is_parsable_with_special_chain(to_check):
            self._evaluation_chain = self._special_chain
            can_parse = True

        return can_parse

    def _is_parsable_with_special_chain(self, argument):
        """
        Check if the first entry corresponds to a standard chain, i.e. starting with monitor and followed by run spec.
        @param arguments: the string list containing the arguments
        """
        can_parse = False

        if argument[0] == self._first_level_keys_special[0] and argument[1].startswith(self._special_chain[1][0]):
            can_parse = True
        elif re.match(self._first_level_keys_special[1], argument[0]) and argument[1].startswith(self._special_chain[1][0]):
            can_parse = True

        return can_parse

    def _is_parsable_with_standard_chain(self, argument):
        """
        Check if the first entry corresponds to a standard chain, i.e. not starting with a monitor
        @param arguments: the string list containing the arguments
        """
        if argument[0] in self._standard_chain[0]:
            return True
        else:
            return False

    def _prepare_argument(self, argumentstring):
        """
        Takes an argument string and returns splits it into a list
        @param argumentstring: the input
        @returns a list
        """
        # Convert to capital and split the string
        split_arguments = argumentstring.split("/")

        # Make everything to upper except for the file name
        for index in range(0, len(split_arguments)):
            arg = split_arguments[index]
            if "=" in arg:
                parts = arg.split("=")
                # If there are not exactly two elements, then the input is invalid
                # We need Run=somerun
                if len(parts) != 2:
                    return []
                new_arg = "RUN=" + parts[1].strip()
                split_arguments[index] = new_arg
            else:
                split_arguments[index] = arg.strip().upper()
        return [element.strip() for element in split_arguments]

    def parse_and_set(self, arguments):
        """
        Parse the values of the parameter string and set them on the reducer
        @param arguments: the string containing the arguments
        @returns an error message if something went wrong or else nothing
        """
        # Parse the arguments.
        self._parse(arguments)

        # Now pass the arguments back in a defined format
        setting = DarkRunSettings(
            mon=self._mon, run_number=self._run_number, time=self._use_time, mean=self._use_mean, mon_number=self._mon_number
        )

        # Reset the parse results just in case we want to use it again
        self._reset_parse_results()

        return setting

    def _parse(self, arguments):
        """
        Parse the arguments and store the results
        @param arguments: the string containing the arguments
        @raise RuntimeError: If the argument cannot be parsed correctly
        """
        to_parse = self._prepare_argument(arguments)

        if not self.can_attempt_to_parse(arguments):
            raise RuntimeError("BackCommandParser: Cannot parse provided arguments." "They are not compatible")

        index = 0
        for element in to_parse:
            key = self._evaluation_chain[index]
            evaluation_method = self._get_method(key)
            evaluation_method(element)
            index += 1
