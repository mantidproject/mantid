import unittest
from mantid.simpleapi import *
from mantid.api import *


class AddSampleLogMultipleTest(unittest.TestCase):

    def setUp(self):
        """
        Crates a small sample workspace to test with.
        """
        CreateSampleWorkspace(OutputWorkspace='__AddSampleLogMultiple_test',
                              NumBanks=1,
                              BankPixelWidth=1,
                              XMax=10,
                              BinWidth=1)
        self._workspace = mtd['__AddSampleLogMultiple_test']


    def tearDown(self):
        """
        Removes sample workspaces.
        """
        DeleteWorkspace(self._workspace)


    def _validate_sample_logs(self, names, values, types):
        """
        Validates sample logs set on workspace.

        @param names List of sample log names
        @param values List of sample log values
        @param types List of sample log types
        """
        logs = self._workspace.getSampleDetails().getLogData()
        matched_logs = list()

        for log in logs:
            if log.name in names:
                matched_logs.append(log.name)
                idx = names.index(log.name)

                self.assertEqual(log.value, values[idx])
                self.assertEqual(log.type, types[idx])

        self.assertEqual(matched_logs, names)


    def test_strings(self):
        """
        Tests adding multiple strings.
        """
        names = ['a', 'b', 'c']
        values = ['one', 'two', 'three']
        types = ['string', 'string', 'string']

        AddSampleLogMultiple(Workspace=self._workspace,
                             LogNames=names,
                             LogValues=values)

        self._validate_sample_logs(names, values, types)


    def test_strings_and_numbers(self):
        """
        Tests adding multiple strings and numbers.
        """
        names = ['a', 'b', 'c', 'd', 'e', 'f']
        values = ['one', 'two', 'three', 4, 5.5, 6e2]
        types = ['string', 'string', 'string', 'number', 'number', 'number']

        AddSampleLogMultiple(Workspace=self._workspace,
                             LogNames=names,
                             LogValues=values)

        self._validate_sample_logs(names, values, types)


    def test_validation_no_names(self):
        """
        Test validation for no log names.
        """
        names = []
        values = ['one', 'two', 'three']

        self.assertRaises(RuntimeError,
                          AddSampleLogMultiple,
                          Workspace=self._workspace,
                          LogNames=names,
                          LogValues=values)


    def test_validation_no_values(self):
        """
        Test validation for no log values.
        """
        names = ['a', 'b', 'c']
        values = []

        self.assertRaises(RuntimeError,
                          AddSampleLogMultiple,
                          Workspace=self._workspace,
                          LogNames=names,
                          LogValues=values)


    def test_validation_differing_counts(self):
        """
        Test validation for differing numbers of log names and log values.
        """
        names = ['a', 'b', 'c']
        values = ['one', 'two']

        self.assertRaises(RuntimeError,
                          AddSampleLogMultiple,
                          Workspace=self._workspace,
                          LogNames=names,
                          LogValues=values)


if __name__ == '__main__':
    unittest.main()
