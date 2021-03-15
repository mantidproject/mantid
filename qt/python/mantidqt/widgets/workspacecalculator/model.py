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
from mantid.dataobjects import MDHistoWorkspace, WorkspaceSingleValue
from mantid.kernel import logger as log


class WorkspaceCalculatorModel():
    """This class stores all of the objects necessary to perform required
    operation and places the product in the ADS."""

    def __init__(self, lhs_scale=1.0, lhs_ws=None, rhs_scale=1.0, rhs_ws=None, output_ws=None, operation='+'):
        """Initializes the model with all parameters necessary for performing the desired operation.
         Default parameters do not pass the validation step."""
        self._lhs_scale = lhs_scale
        self._lhs_ws = lhs_ws
        self._rhs_scale = rhs_scale
        self._rhs_ws = rhs_ws
        self._output_ws = output_ws
        self._operation = operation
        self._md_ws = None

    def _check_group_for_md(self, group_name):
        multi_dim_err_msg = "Group contains MD and non-MD workspaces."
        for entry in mtd[group_name]:
            if isinstance(entry, MDHistoWorkspace):
                if self._md_ws is not None and not self._md_ws:
                    log.error(multi_dim_err_msg)
                    return multi_dim_err_msg
                self._md_ws = True
            else:
                if self._md_ws:
                    log.error(multi_dim_err_msg)
                    return multi_dim_err_msg
                else:
                    self._md_ws = False
        return ""

    def _validateSingleInput(self, ws, info):
        try:
            mtd[ws]
        except KeyError:
            err_msg = "The {} input workspace does not exist.".format(info)
            if ws is not None:
                log.error(err_msg)
            return False, err_msg
        if isinstance(mtd[ws], WorkspaceGroup):
            err_msg = self._check_group_for_md(ws)
            if err_msg != str():
                return False, err_msg
        return True, ""

    def _validateMD(self):
        if isinstance(mtd[self._lhs_ws], MDHistoWorkspace):
            self._md_ws = True
        multi_dim_err_msg = "Only one of the provided workspaces is multidimensional."
        err_msg = str()
        if (isinstance(mtd[self._lhs_ws], WorkspaceGroup)
                or isinstance(mtd[self._rhs_ws], WorkspaceGroup)):
            if isinstance(mtd[self._lhs_ws], WorkspaceGroup):
                err_msg = self._check_group_for_md(self._lhs_ws)
            if isinstance(mtd[self._rhs_ws], WorkspaceGroup):
                err_msg = self._check_group_for_md(self._rhs_ws)
        else:
            if (self._md_ws
                    and (not isinstance(mtd[self._rhs_ws], MDHistoWorkspace)
                         and not isinstance(mtd[self._rhs_ws], WorkspaceSingleValue))):
                log.error(multi_dim_err_msg)
                err_msg = multi_dim_err_msg
        return err_msg == str(), err_msg

    def validateInputs(self, lhs_ws=None, rhs_ws=None, operation=None):
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

    @staticmethod
    def _scale_md_group(ws_group, scale):
        md_group = mtd[ws_group].clone()
        for entry in md_group:
            entry *= scale
        return md_group

    def _scale_input_workspaces(self):

        def scale_ws(ws_name, scale):
            return mtd[ws_name] * scale

        if (self._md_ws
                and (isinstance(mtd[self._lhs_ws], WorkspaceGroup)
                     or isinstance(mtd[self._rhs_ws], WorkspaceGroup))):
            if isinstance(mtd[self._lhs_ws], WorkspaceGroup):
                lhs_ws = self._scale_md_group(self._lhs_ws, self._lhs_scale)
            else:
                lhs_ws = scale_ws(self._lhs_ws, self._lhs_scale)
            if isinstance(mtd[self._rhs_ws], WorkspaceGroup):
                rhs_ws = self._scale_md_group(self._rhs_ws, self._rhs_scale)
            else:
                rhs_ws = scale_ws(self._rhs_ws, self._rhs_scale)
        else:
            lhs_ws = scale_ws(self._lhs_ws, self._lhs_scale)
            rhs_ws = scale_ws(self._rhs_ws, self._rhs_scale)
        return lhs_ws, rhs_ws

    def performOperation(self):
        lhs_valid, rhs_valid, err_msg = self.validateInputs()
        if err_msg != str():
            return lhs_valid, rhs_valid, err_msg

        lhs_ws, rhs_ws = self._scale_input_workspaces()

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
                    WeightedMeanMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
                else:
                    WeightedMean(InputWorkspace1=lhs_ws, InputWorkspace2=rhs_ws, OutputWorkspace=self._output_ws)
            else:
                if self._md_ws:
                    DivideMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
                else:
                    Divide(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
        except (RuntimeError, ValueError) as err:
            return False, False, str(err)
        return True, True, ""
