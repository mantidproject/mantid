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
                              Plus, PlusMD, WeightedMean, WeightedMeanMD)
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

    def _validateSingleInput(self, ws, info):
        try:
            mtd[ws]
        except KeyError:
            err_msg = "The {} input workspace does not exist.".format(info)
            if ws is not None:
                log.error(err_msg)
            return False, err_msg
        return True, ""

    def _validateMD(self):
        if isinstance(mtd[self._lhs_ws], MDHistoWorkspace):
            self._md_ws = True
        multi_dim_err_msg = "Only one of the provided workspaces is multidimensional."
        if self._md_ws and not isinstance(mtd[self._rhs_ws], MDHistoWorkspace):
            log.error(multi_dim_err_msg)
            return False, multi_dim_err_msg

        def check_group_for_md(group_name):
            for entry in mtd[group_name]:
                if isinstance(entry, MDHistoWorkspace):
                    self._md_ws = True
                elif self._md_ws:
                    log.error(multi_dim_err_msg)
                    return multi_dim_err_msg
            return ""
        err_msg = str()
        if isinstance(mtd[self._lhs_ws], WorkspaceGroup):
            err_msg = check_group_for_md(self._lhs_ws)
        if isinstance(mtd[self._rhs_ws], WorkspaceGroup):
            err_msg = check_group_for_md(self._rhs_ws)
        if err_msg != str():
            return False, err_msg
        return True, ""

    def validateInputs(self, lhs_ws=None, rhs_ws=None, operation="+"):
        if operation:
            self._operation = operation
        if lhs_ws:
            self._lhs_ws = lhs_ws
        valid_lhs, err_msg_lhs = self._validateSingleInput(self._lhs_ws, "LHS")
        if rhs_ws:
            self._rhs_ws = rhs_ws
        valid_rhs, err_msg_rhs = self._validateSingleInput(self._rhs_ws, "RHS")
        if not valid_lhs or not valid_rhs:
            return valid_lhs, valid_rhs, [err_msg_lhs, err_msg_rhs]

        valid, err_msg = self._validateMD()
        return valid, valid, err_msg

    def performOperation(self):
        lhs_valid, rhs_valid, err_msg = self.validateInputs()
        if err_msg != str():
            return lhs_valid, rhs_valid, err_msg
        lhs_ws = self._lhs_scale * mtd[self._lhs_ws]
        rhs_ws = self._rhs_scale * mtd[self._rhs_ws]

        try:
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
        except RuntimeError as err:
            return str(err)
        return True, True, ""
