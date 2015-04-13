#pylint: disable=no-init,invalid-name
"""
These system tests are to verify the behaviour of the ISIS reflectometry reduction scripts
"""

import stresstesting
from mantid.simpleapi import *
import mantid.api._api

from abc import ABCMeta, abstractmethod

class LoadAndCheckBase(stresstesting.MantidStressTest):

    __metaclass__ = ABCMeta # Mark as an abstract class

    __comparison_out_workspace_name = 'a_integrated'

    @abstractmethod
    def get_raw_workspace_filename(self):
        """Returns the name of the raw workspace file"""
        raise NotImplementedError("Implement get_raw_workspace_filename")

    @abstractmethod
    def get_nexus_workspace_filename(self):
        """Returns the name of the nexus workspace file"""
        raise NotImplementedError("Implement get_nexus_workspace_filename")

    @abstractmethod
    def get_expected_instrument_name(self):
        """Returns the name of the instrument"""
        raise NotImplementedError("Implement get_expected_instrument_name")

    def get_expected_number_of_periods(self):
        return 1

    def get_integrated_reference_workspace_filename(self):
        """Returns the name of the benchmark file used for end-of-test comparison."""
        if self.enable_reference_result_checking():
            # Must have a reference result file if reference result checking is required
            raise NotImplementedError("Implement get_nexus_workspace_filename")

    def enable_reference_result_checking(self):
        return True

    def enable_instrument_checking(self):
        return True


    def do_check_workspace_shape(self, ws1, ws2):
        self.assertTrue(ws1.getNumberHistograms(), ws2.getNumberHistograms())
        self.assertTrue(len(ws1.readX(0)) == len(ws2.readX(0)))
        self.assertTrue(len(ws1.readY(0)) == len(ws2.readY(0)))

    def do_check_instrument_applied(self, ws1, ws2):
        instrument_name = self.get_expected_instrument_name()
        self.assertTrue(ws1.getInstrument().getName() == instrument_name)
        self.assertTrue(ws2.getInstrument().getName() == instrument_name)

    def runTest(self):
        Load(Filename=self.get_nexus_workspace_filename(), OutputWorkspace='nexus')
        Load(Filename=self.get_raw_workspace_filename(), OutputWorkspace='raw')

        a = mtd['nexus']
        b = mtd['raw']
        n_periods = self.get_expected_number_of_periods()

        self.assertTrue(type(a) == type(b))

        #raise NotImplementedError()
        if isinstance(a,mantid.api._api.WorkspaceGroup):
            self.assertEqual(a.size(), b.size())
            self.assertEqual(a.size(), n_periods)
            # Loop through each workspace in the group and apply some simple comaprison checks.
            for i in range(0, a.size()):
                self.do_check_workspace_shape(a[i], b[i])
            if self.enable_instrument_checking():
                self.do_check_instrument_applied(a[i], b[i])
            if self.enable_reference_result_checking():
                Integration(InputWorkspace=a[0], OutputWorkspace=self.__comparison_out_workspace_name)
        else:
            self.do_check_workspace_shape(a, b)
            if self.enable_instrument_checking():
                self.do_check_instrument_applied(a, b)
            if self.enable_reference_result_checking():
                Integration(InputWorkspace=a, OutputWorkspace=self.__comparison_out_workspace_name)

    def validate(self):
        self.disableChecking.append('Instrument')
        if self.enable_reference_result_checking():
            return self.__comparison_out_workspace_name, self.get_integrated_reference_workspace_filename()
        else:
            return True



