# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from mantid.simpleapi import (Divide, DivideMD, Minus, MinusMD, mtd, Multiply, MultiplyMD,
                              Plus, PlusMD, Scale, WeightedMean, WeightedMeanMD)
from mantid.api import WorkspaceGroup
from mantid.dataobjects import MDHistoWorkspace
from mantid.kernel import logger as log


class WorkspaceCalculatorModel():
    """This class stores all of the objects necessary to perform required
    operation and places the product in the ADS."""

    def __init__(self, lhs_scale=1.0, lhs_ws=None, rhs_scale=1.0, rhs_ws=None, output_ws=None, operation='+'):
        """ """
        self._lhs_scale = lhs_scale
        self._lhs_ws = lhs_ws
        self._rhs_scale = rhs_scale
        self._rhs_ws = rhs_ws
        self._output_ws = output_ws
        self._operation = operation
        self._md_ws = False

    def validateInputs(self):
        correct_inputs = True
        if self._lhs_scale == 0 or self._rhs_scale == 0:
            log.error("At least one scaling factor is equal to zero.")
            correct_inputs = False
        try:
            mtd[self._lhs_ws]
            mtd[self._rhs_ws]
        except KeyError:
            log.error("At least one of the input workspaces does not exist.")
            correct_inputs = False
            return correct_inputs

        if isinstance(mtd[self._lhs_ws], MDHistoWorkspace):
            self._md_ws = True
        multi_dim_err_msg = "Only one of the provided workspaces is multidimensional."
        if self._md_ws and not isinstance(mtd[self._rhs_ws], MDHistoWorkspace):
            log.error(multi_dim_err_msg)
            correct_inputs = False

        def check_group_for_md(group_name):
            for entry in mtd[group_name]:
                if isinstance(entry, MDHistoWorkspace):
                    self._md_ws = True
                elif self._md_ws:
                    log.error(multi_dim_err_msg)
                    return False
        if isinstance(mtd[self._lhs_ws], WorkspaceGroup):
            correct_inputs = check_group_for_md (mtd[self._lhs_ws])
        if isinstance(mtd[self._rhs_ws], WorkspaceGroup):
            correct_inputs = check_group_for_md (mtd[self._rhs_ws])

        return correct_inputs

    def performOperation(self):
        if not self.validateInputs():
            return
        lhs_ws = Scale(InputWorkspace=self._lhs_ws, Factor=self._lhs_scale)
        rhs_ws = Scale(InputWorkspace=self._rhs_ws, Factor=self._rhs_scale)

        if self._operation == '+':
            if self._md_ws:
                PlusMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
            else:
                Plus(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
        elif self._operation == '-':
            if self._md_ws:
                MinusMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
            else:
                Minus(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
        elif self._operation == '*':
            if self._md_ws:
                MultiplyMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
            else:
                Multiply(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
        elif self._operation == 'WM':
            if self._md_ws:
                WeightedMeanMD(InputWorkspace1=lhs_ws, InputWorkspace2=rhs_ws, OutputWorkspace=self._output_ws)
            else:
                WeightedMean(InputWorkspace1=lhs_ws, InputWorkspace2=rhs_ws, OutputWorkspace=self._output_ws)
        else:
            if self._md_ws:
                DivideMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
            else:
                Divide(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
