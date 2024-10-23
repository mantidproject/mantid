# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from LoadAndCheckBase import LoadAndCheckBase

#
# Test File loading and basic data integrity checks of INTER data in Mantid.
#


class INTERLoadingTest(LoadAndCheckBase):
    def get_raw_workspace_filename(self):
        return "INTER00007709.raw"

    def get_nexus_workspace_filename(self):
        return "INTER00007709.nxs"

    def get_integrated_reference_workspace_filename(self):
        return "INTER00007709Integrated.nxs"

    def get_expected_instrument_name(self):
        return "INTER"
