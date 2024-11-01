# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import os
import tempfile
import shutil
from testhelpers import run_algorithm

"""
Helper type to represent an entry in a cal file
"""


class CalFileLine:

    _number = None
    _UDET = None
    _offset = None
    _select = None
    _group = None

    def __init__(self, number, UDET, offset, select, group):
        self._number = number
        self._UDET = UDET
        self._offset = offset
        self._select = select
        self._group = group

    def getNumber(self):
        return self._number

    def getUDET(self):
        return self._UDET

    def getOffset(self):
        return self._offset

    def getSelect(self):
        return self._select

    def getGroup(self):
        return self._group


"""
A helper resource managing wrapper over a new calfile object. Creates cal file and allows writing to it.
"""


class DisposableCalFileObject:

    _fullpath = None
    _dirpath = None

    def __init__(self, name):
        self._dirpath = tempfile.mkdtemp()
        self._fullpath = os.path.join(self._dirpath, name)
        file = open(self._fullpath, "w")
        file.close()

    def writeline(self, entry):
        file = open(self._fullpath, "a")
        file.write("%i\t%i\t%f\t%i\t%i\n" % (entry.getNumber(), entry.getUDET(), entry.getOffset(), entry.getSelect(), entry.getGroup()))
        file.close()

    def __del__(self):
        os.remove(self._fullpath)
        if os.path.exists(self._dirpath):
            shutil.rmtree(self._dirpath)

    def getPath(self):
        return self._fullpath


"""
A helper resource managing wrapper over an existing cal file for reading. Disposes of it after reading.
"""


class ReadableCalFileObject:

    _fullpath = None
    _dirpath = None

    def __init__(self, dirpath, filename):
        fullpath = os.path.join(dirpath, filename)
        if not os.path.exists(fullpath):
            raise RuntimeError("No readable cal file at location: " + fullpath)
        else:
            self._fullpath = fullpath
            self._dirpath = dirpath

    def __del__(self):
        pass
        os.remove(self._fullpath)
        shutil.rmtree(self._dirpath)

    def readline(self):
        result = None
        file = open(self._fullpath, "r")
        line = file.readline().split()
        number = int(line[0])
        udet = int(line[1])
        offset = float(line[2])
        select = int(line[3])
        group = int(line[4])
        result = CalFileLine(number, udet, offset, select, group)
        file.close()

        return result


class MergeCalFilesTest(unittest.TestCase):
    def do_execute(self, masterEntry, updateEntry, mergeOffsets, mergeSelect, mergeGroups):

        # Create the master cal file
        masterfile = DisposableCalFileObject("master.cal")
        masterfile.writeline(masterEntry)

        # Create the update cal file
        updatefile = DisposableCalFileObject("update.cal")
        updatefile.writeline(updateEntry)

        # Create a temp file location
        dirpath = tempfile.mkdtemp()
        outputfilestring = os.path.join(dirpath, "product.cal")

        # Run the algorithm
        run_algorithm(
            "MergeCalFiles",
            UpdateFile=updatefile.getPath(),
            MasterFile=masterfile.getPath(),
            OutputFile=outputfilestring,
            MergeOffsets=mergeOffsets,
            MergeSelections=mergeSelect,
            MergeGroups=mergeGroups,
        )

        # Read the results file and return the first line as a CalFileEntry
        outputfile = ReadableCalFileObject(dirpath, "product.cal")
        firstLineOutput = outputfile.readline()

        return firstLineOutput

    def test_replace_nothing(self):

        masterEntry = CalFileLine(1, 1, 1.0, 1, 1)
        updateEntry = CalFileLine(1, 1, 2.0, 2, 2)

        firstLineOutput = self.do_execute(
            masterEntry=masterEntry, updateEntry=updateEntry, mergeOffsets=False, mergeSelect=False, mergeGroups=False
        )

        self.assertEqual(masterEntry.getOffset(), firstLineOutput.getOffset())
        self.assertEqual(masterEntry.getSelect(), firstLineOutput.getSelect())
        self.assertEqual(masterEntry.getGroup(), firstLineOutput.getGroup())

    def test_replace_offset_only(self):

        masterEntry = CalFileLine(1, 1, 1.0, 1, 1)
        updateEntry = CalFileLine(1, 1, 2.0, 2, 2)

        firstLineOutput = self.do_execute(
            masterEntry=masterEntry, updateEntry=updateEntry, mergeOffsets=True, mergeSelect=False, mergeGroups=False
        )

        self.assertEqual(updateEntry.getOffset(), firstLineOutput.getOffset())
        self.assertEqual(masterEntry.getSelect(), firstLineOutput.getSelect())
        self.assertEqual(masterEntry.getGroup(), firstLineOutput.getGroup())

    def test_replace_select_only(self):

        masterEntry = CalFileLine(1, 1, 1.0, 1, 1)
        updateEntry = CalFileLine(1, 1, 2.0, 2, 2)

        # Run the algorithm and return the first line of the output merged cal file.
        firstLineOutput = self.do_execute(
            masterEntry=masterEntry, updateEntry=updateEntry, mergeOffsets=False, mergeSelect=True, mergeGroups=False
        )

        self.assertEqual(masterEntry.getOffset(), firstLineOutput.getOffset())
        self.assertEqual(updateEntry.getSelect(), firstLineOutput.getSelect())
        self.assertEqual(masterEntry.getGroup(), firstLineOutput.getGroup())

    def test_replace_group_only(self):

        masterEntry = CalFileLine(1, 1, 1.0, 1, 1)
        updateEntry = CalFileLine(1, 1, 2.0, 2, 2)

        # Run the algorithm and return the first line of the output merged cal file.
        firstLineOutput = self.do_execute(
            masterEntry=masterEntry, updateEntry=updateEntry, mergeOffsets=False, mergeSelect=False, mergeGroups=True
        )

        self.assertEqual(masterEntry.getOffset(), firstLineOutput.getOffset())
        self.assertEqual(masterEntry.getSelect(), firstLineOutput.getSelect())
        self.assertEqual(updateEntry.getGroup(), firstLineOutput.getGroup())

    def test_replace_all(self):

        masterEntry = CalFileLine(1, 1, 1.0, 1, 1)
        updateEntry = CalFileLine(1, 1, 2.0, 2, 2)

        # Run the algorithm and return the first line of the output merged cal file.
        firstLineOutput = self.do_execute(
            masterEntry=masterEntry, updateEntry=updateEntry, mergeOffsets=True, mergeSelect=True, mergeGroups=True
        )

        self.assertEqual(updateEntry.getOffset(), firstLineOutput.getOffset())
        self.assertEqual(updateEntry.getSelect(), firstLineOutput.getSelect())
        self.assertEqual(updateEntry.getGroup(), firstLineOutput.getGroup())


if __name__ == "__main__":
    unittest.main()
