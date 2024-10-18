# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from LoadAndCheckBase import LoadAndCheckBase


class SurfLoadingTest(LoadAndCheckBase):
    """
    Test File loading and basic data integrity checks of SURF data in Mantid.
    """

    def get_raw_workspace_filename(self):
        return "SRF92132.raw"

    def get_nexus_workspace_filename(self):
        return "SRF92132.nxs"

    def get_expected_number_of_periods(self):
        return 22

    def get_integrated_reference_workspace_filename(self):
        return "SRF92132_1Integrated.nxs"

    def get_expected_instrument_name(self):
        return "SURF"

    def enable_instrument_checking(self):
        return True  # No IDF in Mantid
