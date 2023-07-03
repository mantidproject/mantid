# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from contextlib import contextmanager
from io import BufferedReader
import re
from typing import Callable, Sequence, Tuple

from abins.constants import EOF, ONE_CHARACTER


class TextParser:
    """
    File parsing tools: wraps a few methods for buffer navigation
    """

    @staticmethod
    @contextmanager
    def save_excursion(file_obj):
        """Temporarily save the position in file and return when leaving context"""
        pos = file_obj.tell()
        try:
            yield pos

        finally:
            file_obj.seek(pos)

    @staticmethod
    def _get_test_from_args(msg: str = None, regex: str = None) -> Tuple[Callable[[str], bool], str]:
        """Validate string/regex search and create a line-testing function"""
        if (msg is not None) and (regex is not None):
            raise ValueError("msg or regex should be provided, not both")

        elif msg is not None:
            msg = bytes(msg, "utf8")

            def test(line):
                if line.strip() and msg in line:
                    return True
                else:
                    return False

            pattern = msg

        elif regex is not None:
            compiled_regex = re.compile(bytes(regex, "utf8"))

            def test(line):
                return compiled_regex.match(line)

            pattern = regex

        else:
            raise ValueError("No msg or regex provided: nothing to match")

        return test, pattern

    @classmethod
    def find_first(cls, *, file_obj: BufferedReader, msg: str = None, regex: str = None) -> str:
        """
        Match a string/regex and moves file current position to the next line.

        :param file_obj: file object
        :param msg: keyword to find (exact match)
        :param regex: regular expression to find (use *instead of* msg option).
             This string will be compiled to a Python re.Pattern.
        """
        test, pattern = cls._get_test_from_args(msg=msg, regex=regex)

        while not cls.file_end(file_obj):
            line = file_obj.readline()

            if test(line):
                return line

        raise EOFError(f'"{pattern}" not found')

    @classmethod
    def find_last(cls, *, file_obj: BufferedReader, msg: str = None, regex: str = None) -> str:
        """
        Moves file current position to the last occurrence of msg or regex.

        :param file_obj: file object from which we read
        :param msg: keyword to find
        """
        test, pattern = cls._get_test_from_args(msg=msg, regex=regex)

        last_entry = None

        while not cls.file_end(file_obj):
            pos = file_obj.tell()
            line = file_obj.readline()
            if test(line):
                last_entry = pos

        if last_entry is None:
            raise EOFError(f'No entry "{pattern}" has been found.')
        else:
            file_obj.seek(last_entry)

    @staticmethod
    def file_end(file_obj: BufferedReader) -> bool:
        """
        Checks end of the text file.

        :param file_obj: file object which was open in "r" mode
        :returns: True if end of file, otherwise False
        """
        pos = file_obj.tell()
        potential_end = file_obj.read(ONE_CHARACTER)
        if potential_end == EOF:
            return True
        else:
            file_obj.seek(pos)
            return False

    @staticmethod
    def block_end(file_obj: BufferedReader, *, msg: Sequence[str]) -> bool:
        """
        Checks for msg which terminates block.

        :param file_obj: file object from which we read
        :param msg: list of messages that can end kpoint block.
        :returns: True if end of block otherwise False
        """
        for item in msg:
            pos = file_obj.tell()
            line = file_obj.readline()
            file_obj.seek(pos)
            item = bytes(item, "utf8")
            if item in line:
                return True
        return False

    @classmethod
    def move_to(cls, file_obj=None, msg=None):
        """
        Finds the first line with msg and moves read file pointer to that line.
        :param file_obj: file object from which we read
        :param msg: keyword to find
        """
        msg = bytes(msg, "utf8")
        while not cls.file_end(file_obj):
            pos = file_obj.tell()
            line = file_obj.readline()
            if line.strip() and msg in line:
                file_obj.seek(pos)
                break
