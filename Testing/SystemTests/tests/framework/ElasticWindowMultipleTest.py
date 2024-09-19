# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
from systemtesting import MantidSystemTest
from mantid.simpleapi import ElasticWindowMultiple, GroupWorkspaces, Load


class ElasticWindowMultipleISISTest(MantidSystemTest):
    def runTest(self):
        Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace="irs26176_graphite002_red", LoadHistory=False)
        GroupWorkspaces(InputWorkspaces="irs26176_graphite002_red", OutputWorkspace="Elwin_Input")
        ElasticWindowMultiple(
            InputWorkspaces="Elwin_Input",
            IntegrationRangeStart=-0.54760000000000009,
            IntegrationRangeEnd=0.54410000000000014,
            OutputInQ="irs26176_graphite002_red_elwin_eq",
            OutputInQSquared="irs26176_graphite002_red_elwin_eq2",
            OutputELF="irs26176_graphite002_red_elwin_elf",
        )

    def validate(self):
        self.tolerance = 1e-10
        return (
            "irs26176_graphite002_red_elwin_eq",
            "irs26176_graphite002_red_elwin_eq.nxs",
            "irs26176_graphite002_red_elwin_eq2",
            "irs26176_graphite002_red_elwin_eq2.nxs",
            "irs26176_graphite002_red_elwin_elf",
            "irs26176_graphite002_red_elwin_elf.nxs",
        )
