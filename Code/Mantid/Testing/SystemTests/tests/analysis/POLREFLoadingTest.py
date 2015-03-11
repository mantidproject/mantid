#pylint: disable=no-init
from LoadAndCheckBase import *

# Test File loading and basic data integrity checks of POLREF data in Mantid.
class POLREFLoadingTest(LoadAndCheckBase):
    def get_raw_workspace_filename(self):
        return "POLREF00004699.raw"

    def get_nexus_workspace_filename(self):
        return "POLREF00004699.nxs"

    def get_expected_number_of_periods(self):
        return 2

    def get_integrated_reference_workspace_filename(self):
        return "POLREF00004699_1Integrated.nxs"

    def get_expected_instrument_name(self):
        return "POLREF"

