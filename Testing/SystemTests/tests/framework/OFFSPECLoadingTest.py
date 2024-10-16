# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from LoadAndCheckBase import LoadAndCheckBase


class OFFSPECLoadingTest(LoadAndCheckBase):
    """
    Test File loading and basic data integrity checks of OFFSPEC data in Mantid.
    """

    def get_raw_workspace_filename(self):
        return "OFFSPEC00010791.raw"

    def get_nexus_workspace_filename(self):
        return "OFFSPEC00010791.nxs"

    def get_expected_number_of_periods(self):
        return 2

    def get_integrated_reference_workspace_filename(self):
        return "OFFSPEC00010791_1Integrated.nxs"
