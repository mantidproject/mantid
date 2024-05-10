# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from mantid.simpleapi import (
    CloneWorkspace,
    CreateSingleValuedWorkspace,
    DeleteWorkspaces,
    DeleteWorkspace,
    Divide,
    DivideMD,
    Minus,
    MinusMD,
    mtd,
    Multiply,
    MultiplyMD,
    Plus,
    PlusMD,
    RenameWorkspace,
    Scale,
    WeightedMean,
    WeightedMeanMD,
)
from mantid.api import WorkspaceGroup, AlgorithmManager
from mantid.dataobjects import MDHistoWorkspace, WorkspaceSingleValue


class WorkspaceCalculatorModel:
    """This class stores all of the objects necessary to perform required
    operation and places the product in the ADS."""

    def __init__(self, lhs_scale=1.0, lhs_ws=None, rhs_scale=1.0, rhs_ws=None, output_ws=None, operation="+"):
        """Initializes the model with all parameters necessary for performing the desired operation.
        Default parameters do not pass the validation step."""
        self._lhs_scale = lhs_scale
        self._lhs_ws = lhs_ws
        self._rhs_scale = rhs_scale
        self._rhs_ws = rhs_ws
        self._output_ws = output_ws
        self._operation = operation
        self._md_lhs = None
        self._md_rhs = None

    def _figure_out_algorithm(self):
        alg_name = ""
        if self._operation == "+":
            alg_name = "Plus"
        elif self._operation == "-":
            alg_name = "Minus"
        elif self._operation == "*":
            alg_name = "Multiply"
        elif self._operation == "/":
            alg_name = "Divide"
        elif self._operation == "WM":
            alg_name = "WeightedMean"

        if self._md_lhs or self._md_rhs:
            alg_name += "MD"
        return alg_name

    def _validate_algorithm(self, lhs=None, rhs=None):
        """Validates the inputs for algorithm that is going to be called to process inputs."""
        valid_lhs = valid_rhs = True
        err_msg_lhs = err_msg_rhs = ""
        err_msg = ""
        alg_name = self._figure_out_algorithm()
        alg = AlgorithmManager.createUnmanaged(alg_name)
        alg.initialize()
        if lhs:
            try:
                if alg_name == "WeightedMean":
                    alg.setProperty("InputWorkspace1", lhs)
                else:
                    alg.setProperty("LHSWorkspace", lhs)
            except (RuntimeError, ValueError) as err:
                valid_lhs = False
                err_msg_lhs = str(err)
                err_msg = err_msg_lhs
        if rhs:
            try:
                if alg_name == "WeightedMean":
                    alg.setProperty("InputWorkspace2", rhs)
                else:
                    alg.setProperty("RHSWorkspace", rhs)
            except (RuntimeError, ValueError) as err:
                valid_rhs = False
                err_msg_rhs = str(err)
                err_msg = err_msg_rhs
        if lhs and rhs:
            err_msg = [err_msg_lhs, err_msg_rhs]
        return valid_lhs, valid_rhs, err_msg

    def _check_group_for_md(self, group_name, info):
        multi_dim_err_msg = "Group contains MD and non-MD workspaces."
        md_check = self._md_lhs if info == "LHS" else self._md_rhs
        for entry in mtd[group_name]:
            if isinstance(entry, MDHistoWorkspace):
                if md_check is not None and not md_check:
                    return multi_dim_err_msg
                md_check = True
            else:
                if md_check:
                    return multi_dim_err_msg
                md_check = False
        if info == "LHS":
            self._md_lhs = md_check
        else:
            self._md_rhs = md_check
        return ""

    def _validateMD(self):
        multi_dim_err_msg = "Only one of the provided workspaces is multidimensional."
        err_msg = str()
        if isinstance(mtd[self._lhs_ws], WorkspaceGroup) or isinstance(mtd[self._rhs_ws], WorkspaceGroup):
            if isinstance(mtd[self._lhs_ws], WorkspaceGroup):
                err_msg = self._check_group_for_md(self._lhs_ws, "LHS")
            if isinstance(mtd[self._rhs_ws], WorkspaceGroup):
                err_msg = self._check_group_for_md(self._rhs_ws, "RHS")
        else:
            if self._md_lhs and (
                not isinstance(mtd[self._lhs_ws], MDHistoWorkspace) and not isinstance(mtd[self._lhs_ws], WorkspaceSingleValue)
            ):
                err_msg = multi_dim_err_msg
            if self._md_lhs and (
                not isinstance(mtd[self._lhs_ws], MDHistoWorkspace) and not isinstance(mtd[self._lhs_ws], WorkspaceSingleValue)
            ):
                err_msg = multi_dim_err_msg
        if self._md_lhs != self._md_rhs and (
            not isinstance(mtd[self._lhs_ws], WorkspaceSingleValue) and not isinstance(mtd[self._rhs_ws], WorkspaceSingleValue)
        ):
            err_msg = multi_dim_err_msg
        return err_msg == str(), err_msg

    def _validateSingleInput(self, ws, info):
        try:
            mtd[ws]
        except KeyError:
            err_msg = "The {} input workspace does not exist.".format(info)
            return False, err_msg
        if isinstance(mtd[ws], WorkspaceGroup):
            err_msg = self._check_group_for_md(ws, info)
            if err_msg != str():
                return False, err_msg
        elif isinstance(mtd[ws], MDHistoWorkspace):
            if info == "LHS":
                self._md_lhs = True
            else:
                self._md_rhs = True
        else:
            if info == "LHS":
                self._md_lhs = False
            else:
                self._md_rhs = False
        return True, ""

    def validateInputs(self, lhs_ws=None, rhs_ws=None, operation=None):
        self._md_lhs = None  # needed to reset MD checks
        self._md_rhs = None
        if operation:
            self._operation = operation
        if lhs_ws:
            self._lhs_ws = lhs_ws
        valid_lhs, err_msg_lhs = self._validateSingleInput(self._lhs_ws, "LHS")
        if valid_lhs:  # validates algorithm input individually
            valid_lhs, _, err_msg_lhs = self._validate_algorithm(lhs=self._lhs_ws)
        if rhs_ws:
            self._rhs_ws = rhs_ws
        valid_rhs, err_msg_rhs = self._validateSingleInput(self._rhs_ws, "RHS")
        if valid_rhs:  # validates algorithm input individually
            _, valid_rhs, err_msg_rhs = self._validate_algorithm(rhs=self._rhs_ws)
        if not valid_lhs or not valid_rhs:
            return valid_lhs, valid_rhs, [err_msg_lhs, err_msg_rhs]

        valid, err_msg = self._validateMD()
        self._validate_algorithm(lhs=self._lhs_ws, rhs=self._rhs_ws)  # validates input together
        return valid, valid, err_msg

    def updateParameters(self, lhs_scale, lhs_ws, rhs_scale, rhs_ws, output_ws, operation):
        lhs_valid, rhs_valid, err_msg = self.validateInputs(lhs_ws=lhs_ws, rhs_ws=rhs_ws, operation=operation)
        if err_msg == str():
            self._lhs_scale = lhs_scale
            self._lhs_ws = lhs_ws
            self._rhs_scale = rhs_scale
            self._rhs_ws = rhs_ws
            self._output_ws = output_ws
            self._operation = operation
        return lhs_valid, rhs_valid, err_msg

    @staticmethod
    def _scale_md_group(ws_group, scale_ws):
        for entry in mtd[ws_group]:
            MultiplyMD(LHSWorkspace=entry, RHSWorkspace=scale_ws, OutputWorkspace=entry)

    def _scale_md(self, ws_name, scale):
        scale_ws = "scale_ws"
        CreateSingleValuedWorkspace(DataValue=scale, OutputWorkspace=scale_ws)
        if isinstance(mtd[ws_name], WorkspaceGroup):
            ws_name = self._scale_md_group(ws_name, scale_ws)
        else:
            MultiplyMD(LHSWorkspace=ws_name, RHSWorkspace=scale_ws, OutputWorkspace=ws_name)
        DeleteWorkspace(Workspace=scale_ws)

    def _scale_input_workspaces(self):
        lhs_ws = self._lhs_ws + "_tmp_lhs"
        CloneWorkspace(InputWorkspace=self._lhs_ws, OutputWorkspace=lhs_ws)
        rhs_ws = self._rhs_ws + "_tmp_rhs"
        CloneWorkspace(InputWorkspace=self._rhs_ws, OutputWorkspace=rhs_ws)

        if self._md_lhs:
            self._scale_md(lhs_ws, self._lhs_scale)
        else:
            Scale(InputWorkspace=self._lhs_ws, Factor=self._lhs_scale, OutputWorkspace=lhs_ws)
        if self._md_rhs:
            self._scale_md(rhs_ws, self._rhs_scale)
        else:
            Scale(InputWorkspace=self._rhs_ws, Factor=self._rhs_scale, OutputWorkspace=rhs_ws)
        return lhs_ws, rhs_ws

    @staticmethod
    def _regularize_output_names(output_ws):
        """Regularizes the workspace names in the output group, according to the group name
        and workspace index in the group. Only needed for workspace groups, otherwise the name
        is already correct."""
        if isinstance(mtd[output_ws], WorkspaceGroup):
            for entry_no, entry in enumerate(mtd[output_ws]):
                ordered_name = "{}_{}".format(output_ws, entry_no)
                RenameWorkspace(InputWorkspace=entry, OutputWorkspace=ordered_name)

    def performOperation(self):
        lhs_valid, rhs_valid, err_msg = self.validateInputs()
        if err_msg != str():
            return lhs_valid, rhs_valid, err_msg
        lhs_ws, rhs_ws = self._scale_input_workspaces()
        try:
            if self._operation == "+":
                if self._md_lhs or self._md_rhs:
                    PlusMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
                else:
                    Plus(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
            elif self._operation == "-":
                if self._md_lhs or self._md_rhs:
                    MinusMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
                else:
                    Minus(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
            elif self._operation == "*":
                if self._md_lhs or self._md_rhs:
                    MultiplyMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
                else:
                    Multiply(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
            elif self._operation == "WM":
                if self._md_lhs or self._md_rhs:
                    WeightedMeanMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
                else:
                    WeightedMean(InputWorkspace1=lhs_ws, InputWorkspace2=rhs_ws, OutputWorkspace=self._output_ws)
            else:
                if self._md_lhs or self._md_rhs:
                    DivideMD(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
                else:
                    Divide(LHSWorkspace=lhs_ws, RHSWorkspace=rhs_ws, OutputWorkspace=self._output_ws)
        except (RuntimeError, ValueError) as err:
            return False, False, str(err)
        else:
            self._regularize_output_names(self._output_ws)
        finally:
            DeleteWorkspaces(WorkspaceList=[lhs_ws, rhs_ws])
        return True, True, ""
