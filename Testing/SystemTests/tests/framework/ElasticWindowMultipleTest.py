# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
from systemtesting import MantidSystemTest
from mantid.simpleapi import ElasticWindowMultiple, GroupWorkspaces, Load


class ElasticWindowMultipleBase:
    def _run(self):
        Load(Filename=f"{self._name}.nxs", OutputWorkspace=self._name, LoadHistory=False)
        GroupWorkspaces(InputWorkspaces=self._name, OutputWorkspace="Elwin_Input")
        ElasticWindowMultiple(
            InputWorkspaces="Elwin_Input",
            IntegrationRangeStart=self._range_start,
            IntegrationRangeEnd=self._range_end,
            OutputInQ=f"{self._name}_elwin_eq",
            OutputInQSquared=f"{self._name}_elwin_eq2",
            OutputELF=f"{self._name}_elwin_elf",
        )

    def _validate(self):
        self.tolerance = 1e-10
        return (
            f"{self._name}_elwin_eq",
            f"{self._name}_elwin_eq.nxs",
            f"{self._name}_elwin_eq2",
            f"{self._name}_elwin_eq2.nxs",
            f"{self._name}_elwin_elf",
            f"{self._name}_elwin_elf.nxs",
        )


class ElasticWindowMultipleISISDataTest(MantidSystemTest, ElasticWindowMultipleBase):
    _name = "irs26176_graphite002_red"
    _range_start = -0.54760000000000009
    _range_end = 0.54410000000000014

    runTest = ElasticWindowMultipleBase._run
    validate = ElasticWindowMultipleBase._validate


class ElasticWindowMultipleILLDataTest(MantidSystemTest, ElasticWindowMultipleBase):
    _name = "243489-243506ElwinHeat_H2O-HSA-stw"
    _range_start = -4.9999900000000004
    _range_end = 2.4999899999999999

    runTest = ElasticWindowMultipleBase._run
    validate = ElasticWindowMultipleBase._validate
