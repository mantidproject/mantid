# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from LoadAndCheckBase import LoadAndCheckBase


class CRISPLoadingTest(LoadAndCheckBase):
    """
    Test File loading and basic data integrity checks of CRISP data in Mantid.
    """

    def __init__(self):
        super(self.__class__, self).__init__()
        self.disableChecking.append("Instrument")

    def get_raw_workspace_filename(self):
        return "CSP85423.raw"

    def get_nexus_workspace_filename(self):
        return "CSP85423.nxs"

    def get_expected_number_of_periods(self):
        return 2

    def get_integrated_reference_workspace_filename(self):
        return "CSP85423_1Integrated.nxs"

    def get_expected_instrument_name(self):
        return "CRISP"
