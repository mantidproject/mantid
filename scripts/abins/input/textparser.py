# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from contextlib import contextmanager
from io import BufferedReader
import re
from typing import Sequence

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

    @classmethod
    def find_first(cls, *, file_obj: BufferedReader, msg: str = None, regex: str = None) -> str:
        """
        Match a string/regex and moves file current position to the next line.

        :param file_obj: file object
        :param msg: keyword to find (exact match)
        :param regex: regular expression to find (use *instead of* msg option).
             This string will be compiled to a Python re.Pattern.
        """
        if msg is not None:
            msg = bytes(msg, "utf8")
        if regex is not None:
            regex = bytes(regex, "utf8")

        if msg and regex:
            raise ValueError("msg or regex should be provided, not both")
        elif msg:
            while not cls.file_end(file_obj):
                line = file_obj.readline()
                if line.strip() and msg in line:
                    return line
            raise EOFError(f'"{msg.decode()}" not found')
        elif regex:
            test = re.compile(regex)
            while not cls.file_end(file_obj):
                line = file_obj.readline()
                if test.match(line):
                    return line
            raise EOFError(f'"{regex.decode()}" not found')
        else:
            raise ValueError("No msg or regex provided: nothing to match")

    @classmethod
    def find_last(cls, file_obj=None, msg=None):
        """
        Moves file current position to the last occurrence of msg.

        :param file_obj: file object from which we read
        :param msg: keyword to find
        """
        msg = bytes(msg, "utf8")

        found = False
        last_entry = None

        while not cls.file_end(file_obj):
            pos = file_obj.tell()
            line = file_obj.readline()
            if line.strip() and msg in line:
                last_entry = pos
                found = True

        if not found:
            raise EOFError(f'No entry "{msg.decode()}" has been found.')
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
