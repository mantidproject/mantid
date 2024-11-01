# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd
from mantid.simpleapi import AddSampleLogMultiple, CreateSampleWorkspace, DeleteWorkspace


class AddSampleLogMultipleTest(unittest.TestCase):
    def setUp(self):
        """
        Crates a small sample workspace to test with.
        """
        CreateSampleWorkspace(OutputWorkspace="__AddSampleLogMultiple_test", NumBanks=1, BankPixelWidth=1, XMax=10, BinWidth=1)
        self._workspace = mtd["__AddSampleLogMultiple_test"]

    def tearDown(self):
        """
        Removes sample workspaces.
        """
        DeleteWorkspace(self._workspace)

    def _validate_sample_logs(self, names, values, types, units=None):
        """
        Validates sample logs set on workspace.

        @param names List of sample log names
        @param values List of sample log values
        @param types List of sample log types
        @param units List of sample unit names
        """
        logs = self._workspace.getRun().getProperties()
        matched_logs = list()
        if units == None:
            units = [""] * len(names)

        for log in logs:
            if log.name in names:
                matched_logs.append(log.name)
                idx = names.index(log.name)

                self.assertEqual(log.value, values[idx])
                self.assertEqual(log.type, types[idx].lower())
                self.assertEqual(log.units, units[idx])

        self.assertEqual(matched_logs, names)

    def test_strings(self):
        """
        Tests adding multiple strings.
        """
        names = ["a", "b", "c"]
        values = ["one", "two", "three"]
        types = ["string", "string", "string"]

        AddSampleLogMultiple(Workspace=self._workspace, LogNames=names, LogValues=values)

        self._validate_sample_logs(names, values, types)

    def test_strings_with_types(self):
        """
        Tests adding multiple strings.
        """
        names = ["a", "b", "c"]
        values = ["one", "two", "three"]
        types = ["String", "String", "String"]

        AddSampleLogMultiple(Workspace=self._workspace, LogNames=names, LogValues=values, ParseType=False, LogTypes=types)

        self._validate_sample_logs(names, values, types)

    def test_strings_and_numbers(self):
        """
        Tests adding multiple strings and numbers.
        """
        names = ["a", "b", "c", "d", "e", "f"]
        values = ["one", "two", "three", 4, 5.5, 6e2]
        types = ["string", "string", "string", "number", "number", "number"]

        AddSampleLogMultiple(Workspace=self._workspace, LogNames=names, LogValues=values)

        self._validate_sample_logs(names, values, types)

    def test_strings_and_numbers(self):
        """
        Tests adding multiple strings and numbers.
        """
        names = ["a", "b", "c", "d", "e", "f"]
        values = ["one", "two", "three", 4, 5.5, 6e2]
        types = ["String", "String", "String", "Number", "Number", "Number"]

        AddSampleLogMultiple(Workspace=self._workspace, LogNames=names, LogValues=values, ParseType=False, LogTypes=types)

        self._validate_sample_logs(names, values, types)

    def test_units(self):
        """
        Test validation for wrong number of units
        """
        names = ["a", "b", "c"]
        values = ["one", "two", "three"]
        units = ["unit_a", "unit_b", "unit_c"]
        types = ["string", "string", "string"]

        AddSampleLogMultiple(Workspace=self._workspace, LogNames=names, LogValues=values, LogUnits=units)
        self._validate_sample_logs(names, values, types, units)

    def test_validation_wrong_units(self):
        """
        Test validation for wrong number of units
        """
        names = ["a", "b", "c"]
        values = ["one", "two", "three"]
        units = ["unit_a", "unit_b"]

        self.assertRaisesRegex(
            RuntimeError,
            "Number of log units must be 0 or match the number of log names",
            AddSampleLogMultiple,
            Workspace=self._workspace,
            LogNames=names,
            LogValues=values,
            LogUnits=units,
        )

    def test_validation_no_names(self):
        """
        Test validation for no log names.
        """
        names = []
        values = ["one", "two", "three"]

        self.assertRaisesRegex(
            RuntimeError,
            "Must have at least one log name",
            AddSampleLogMultiple,
            Workspace=self._workspace,
            LogNames=names,
            LogValues=values,
        )

    def test_validation_no_values(self):
        """
        Test validation for no log values.
        """
        names = ["a", "b", "c"]
        values = []

        self.assertRaisesRegex(
            RuntimeError,
            "Must have at least one log value",
            AddSampleLogMultiple,
            Workspace=self._workspace,
            LogNames=names,
            LogValues=values,
        )

    def test_validation_differing_counts(self):
        """
        Test validation for differing numbers of log names and log values.
        """
        names = ["a", "b", "c"]
        values = ["one", "two"]

        self.assertRaisesRegex(
            RuntimeError,
            "Number of log values must match number of log names",
            AddSampleLogMultiple,
            Workspace=self._workspace,
            LogNames=names,
            LogValues=values,
        )

    def test_validation_differing_types(self):
        """
        Test validation for differing numbers of log names and log types.
        """
        names = ["a", "b", "c"]
        values = ["one", "two", "three"]
        types = ["String", "String", "String"]
        self.assertRaisesRegex(
            RuntimeError,
            "LogTypes array not used when ParseType=True",
            AddSampleLogMultiple,
            Workspace=self._workspace,
            LogNames=names,
            LogValues=values,
            ParseType=True,
            LogTypes=types,
        )

    def test_validation_differing_types(self):
        """
        Test validation for differing numbers of log names and log types.
        """
        names = ["a", "b", "c"]
        values = ["one", "two", "three"]
        types = ["String", "String"]

        self.assertRaisesRegex(
            RuntimeError,
            "Number of log types must be 0 or match the number of log names",
            AddSampleLogMultiple,
            Workspace=self._workspace,
            LogNames=names,
            LogValues=values,
            ParseType=False,
            LogTypes=types,
        )

    def test_validation_invalid_types(self):
        """
        Test validation for differing numbers of log names and log types.
        """
        names = ["a", "b", "c"]
        values = ["one", "two", "three"]
        types = ["String", "String", "String Series"]

        self.assertRaisesRegex(
            RuntimeError,
            "String Series is not an allowed log type",
            AddSampleLogMultiple,
            Workspace=self._workspace,
            LogNames=names,
            LogValues=values,
            ParseType=False,
            LogTypes=types,
        )


if __name__ == "__main__":
    unittest.main()
