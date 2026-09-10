# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import Load
from plugins.algorithms.component_info_utils import resolve_component_index

#
# Here testing against embedded instrument info in different raw file formats
#
# This include to test that embedded information in raw ISIS Nexus file formats
# get loaded correctly.
#

# here test against a custom made ISIS raw hist nexus file created by Freddie
# where the A1_window has be, for the purpose of testing, been put at a
# completely wrong location of (0,3,0)


class ISISRawHistNexus(systemtesting.MantidSystemTest):
    def runTest(self):
        # ISIS raw hist nexus file with A1_window at location (0,3,0)
        Load("MAPS00018314.nxs", OutputWorkspace="MAPS00018314_raw_ISIS_hist")

    def validate(self):
        MAPS00018314_raw_ISIS_hist = mtd["MAPS00018314_raw_ISIS_hist"]
        component_info = MAPS00018314_raw_ISIS_hist.componentInfo()
        # "MAPS/A1_window" is a hierarchical path, so it needs the shared resolver:
        # indexOfAny does not parse paths.
        A1window_index = resolve_component_index("MAPS/A1_window", component_info)

        if str(component_info.position(A1window_index)) != "[0,3,0]":
            return False

        return True
