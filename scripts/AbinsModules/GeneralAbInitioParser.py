# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import six
import re
import AbinsModules


class GeneralAbInitioParser(object):
    """
    Helper class which groups methods used by DFT Loaders for parsing files.
    """
    def __init__(self):
        pass

    def find_first(self, file_obj=None, msg=None, regex=None):
        """
        Finds the first line with msg. Moves file current position to the next line.
        :param file_obj: file object from which we read
        :type file_obj: BinaryIO
        :param msg: keyword to find (exact match)
        :type msg: str
        :param regex: regular expression to find (use *instead of* msg option).
            This string will be compiled to a Python re.Pattern .
        :type regex: str
        """
        if not six.PY2:
            if msg is not None:
                msg = bytes(msg, "utf8")
            if regex is not None:
                regex = bytes(regex, "utf8")

        if msg and regex:
            raise ValueError("msg or regex should be provided, not both")
        elif msg:
            while not self.file_end(file_obj=file_obj):
                line = file_obj.readline()
                if line.strip() and msg in line:
                    return line
            raise ValueError("'{}' not found".format(msg))
        elif regex:
            test = re.compile(regex)
            while not self.file_end(file_obj=file_obj):
                line = file_obj.readline()
                if test.match(line):
                    return(line)
            raise ValueError("'{}' not found".format(regex))
        else:
            raise ValueError("No msg or regex provided: nothing to match")

    def find_last(self, file_obj=None, msg=None):
        """
        Moves file current position to the last occurrence of msg.
        :param file_obj: file object from which we read
        :param msg: keyword to find
        """
        if not six.PY2:
            msg = bytes(msg, "utf8")

        found = False
        last_entry = None

        while not self.file_end(file_obj=file_obj):
            pos = file_obj.tell()
            line = file_obj.readline()
            if line.strip() and msg in line:
                last_entry = pos
                found = True

        if not found:
            raise ValueError("No entry " + msg + " has been found.")
        else:
            file_obj.seek(last_entry)

    def file_end(self, file_obj=None):
        """
        Checks end of the text file.
        :param file_obj: file object which was open in "r" mode
        :returns: True if end of file, otherwise False
        """
        n = AbinsModules.AbinsConstants.ONE_CHARACTER
        pos = file_obj.tell()
        potential_end = file_obj.read(n)
        if potential_end == AbinsModules.AbinsConstants.EOF:
            return True
        else:
            file_obj.seek(pos)
            return False

    def block_end(self, file_obj=None, msg=None):
        """
        Checks for msg which terminates block.
        :param file_obj: file object from which we read
        :param msg: list with messages which end kpoint block.
        :returns: True if end of block otherwise False
        """
        for item in msg:
            pos = file_obj.tell()
            line = file_obj.readline()
            file_obj.seek(pos)
            if not six.PY2:
                item = bytes(item, "utf8")
            if item in line:
                return True
        return False

    def move_to(self, file_obj=None, msg=None):
        """
        Finds the first line with msg and moves read file pointer to that line.
        :param file_obj: file object from which we read
        :param msg: keyword to find
        """
        if not six.PY2:
            msg = bytes(msg, "utf8")
        while not self.file_end(file_obj=file_obj):
            pos = file_obj.tell()
            line = file_obj.readline()
            if line.strip() and msg in line:
                file_obj.seek(pos)
                break
