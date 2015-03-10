#pylint: disable=no-init
from LoadAndCheckBase import *

'''
Test File loading and basic data integrity checks of OFFSPEC data in Mantid.
'''
class OFFSPECLoadingTest(LoadAndCheckBase):
    def get_raw_workspace_filename(self):
        return "OFFSPEC00010791.raw"

    def get_nexus_workspace_filename(self):
        return "OFFSPEC00010791.nxs"

    def get_expected_number_of_periods(self):
        return 2

    def get_integrated_reference_workspace_filename(self):
        return "OFFSPEC00010791_1Integrated.nxs"
