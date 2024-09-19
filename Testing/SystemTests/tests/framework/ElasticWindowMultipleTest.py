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


class ElasticWindowMultipleILLTest(MantidSystemTest):
    def runTest(self):
        Load(Filename="243489-243506ElwinHeat_H2O-HSA-stw.nxs", OutputWorkspace="243489-243506ElwinHeat_H2O-HSA-stw", LoadHistory=False)
        GroupWorkspaces(InputWorkspaces="243489-243506ElwinHeat_H2O-HSA-stw", OutputWorkspace="Elwin_Input")
        ElasticWindowMultiple(
            InputWorkspaces="Elwin_Input",
            IntegrationRangeStart=-4.9999900000000004,
            IntegrationRangeEnd=2.4999899999999999,
            OutputInQ="243489-243506ElwinHeat_H2O-HSA-stw_elwin_eq",
            OutputInQSquared="243489-243506ElwinHeat_H2O-HSA-stw_elwin_eq2",
            OutputELF="243489-243506ElwinHeat_H2O-HSA-stw_elwin_elf",
        )

    def validate(self):
        self.tolerance = 1e-10
        return (
            "243489-243506ElwinHeat_H2O-HSA-stw_elwin_eq",
            "243489-243506ElwinHeat_H2O-HSA-stw_elwin_eq.nxs",
            "243489-243506ElwinHeat_H2O-HSA-stw_elwin_eq2",
            "243489-243506ElwinHeat_H2O-HSA-stw_elwin_eq2.nxs",
            "243489-243506ElwinHeat_H2O-HSA-stw_elwin_elf",
            "243489-243506ElwinHeat_H2O-HSA-stw_elwin_elf.nxs",
        )
