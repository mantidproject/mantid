# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,too-many-instance-attributes
from mantid.simpleapi import AddSampleLog, ScaleX, Divide, Minus, Multiply, RenameWorkspace, ConvertUnits, CloneWorkspace, RebinToWorkspace
from mantid.api import (
    PythonAlgorithm,
    AlgorithmFactory,
    MatrixWorkspaceProperty,
    WorkspaceGroupProperty,
    PropertyMode,
    MatrixWorkspace,
    Progress,
    WorkspaceGroup,
)
from mantid.kernel import Direction, logger


class ApplyPaalmanPingsCorrection(PythonAlgorithm):
    _sample_ws_name = None
    _corrections_ws_name = None
    _use_can = False
    _can_ws_name = None
    _use_corrections = False
    _can_scale_factor = 1.0
    _scale_can = False
    _output_ws_name = None
    _corrections = None
    _shift_can = False
    _shifted_container = None
    _can_shift_factor = 0.0
    _sample_ws_wavelength = None
    _rebin_container_ws = False
    _factors = []

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Applies a calculated absorption correction in the Paalman and Pings factor style."

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("SampleWorkspace", "", direction=Direction.Input), doc="Name for the input Sample workspace."
        )

        self.declareProperty(
            WorkspaceGroupProperty("CorrectionsWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="Name for the input Corrections workspace.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("CanWorkspace", "", optional=PropertyMode.Optional, direction=Direction.Input),
            doc="Name for the input Can workspace.",
        )

        self.declareProperty(name="CanScaleFactor", defaultValue=1.0, doc="Factor to scale the can data")

        self.declareProperty(name="CanShiftFactor", defaultValue=0.0, doc="Amount by which to shift the container data")

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The output corrections workspace."
        )

        self.declareProperty(name="RebinCanToSample", defaultValue=True, doc="Enable or disable RebinToWorkspace on CanWorkspace.")

    # pylint: disable=too-many-branches
    def PyExec(self):
        self._setup()
        if not self._use_corrections:
            logger.information("Not using corrections")
        if not self._use_can:
            logger.information("Not using container")

        prog_container = Progress(self, start=0.0, end=0.2, nreports=4)
        prog_container.report("Starting algorithm")

        # Units should be wavelength
        sample_unit = self._sample_workspace.getAxis(0).getUnit().unitID()
        sample_ws_wavelength = self._convert_units_wavelength(self._sample_workspace)

        container_ws_wavelength = self._process_container_workspace(self._container_workspace, prog_container) if self._use_can else None

        prog_corr = Progress(self, start=0.2, end=0.6, nreports=2)
        if self._use_corrections:
            prog_corr.report("Preprocessing corrections")

            if self._use_can:
                # Use container factors
                prog_corr.report("Correcting sample and container")
                factor_workspaces = self._get_factor_workspaces()
                output_workspace = self._correct_sample_can(sample_ws_wavelength, container_ws_wavelength, factor_workspaces)
                correction_type = "sample_and_can_corrections"
            else:
                # Use sample factor only
                output_workspace = self._correct_sample(sample_ws_wavelength, self._corrections_workspace[0])
                correction_type = "sample_corrections_only"
                # Add corrections filename to log values
                prog_corr.report("Correcting sample")
                AddSampleLog(
                    Workspace=output_workspace, LogName="corrections_filename", LogType="String", LogText=self._corrections_ws_name
                )
        else:
            # Do simple subtraction
            output_workspace = self._subtract(sample_ws_wavelength, container_ws_wavelength)
            correction_type = "can_subtraction"
            # Add container filename to log values
            can_base = self.getPropertyValue("CanWorkspace")
            can_base = can_base[: can_base.index("_")]
            prog_corr.report("Adding container filename")
            AddSampleLog(Workspace=output_workspace, LogName="container_filename", LogType="String", LogText=can_base)

        prog_wrkflow = Progress(self, 0.6, 1.0, nreports=5)
        # Record the container scale factor
        if self._use_can and self._scale_can:
            prog_wrkflow.report("Adding container scaling")
            AddSampleLog(Workspace=output_workspace, LogName="container_scale", LogType="Number", LogText=str(self._can_scale_factor))

        # Record the container shift amount
        if self._use_can and self._shift_can:
            prog_wrkflow.report("Adding container shift")
            AddSampleLog(Workspace=output_workspace, LogName="container_shift", LogType="Number", LogText=str(self._can_shift_factor))

        # Record the type of corrections applied
        prog_wrkflow.report("Adding correction type")
        AddSampleLog(Workspace=output_workspace, LogName="corrections_type", LogType="String", LogText=correction_type)

        # Add original sample as log entry
        sam_base = self.getPropertyValue("SampleWorkspace")

        if "_" in sam_base:
            sam_base = sam_base[: sam_base.index("_")]
            prog_wrkflow.report("Adding sample filename")
            AddSampleLog(Workspace=output_workspace, LogName="sample_filename", LogType="String", LogText=sam_base)

        # Convert Units back to original
        emode = str(output_workspace.getEMode())
        efixed = 0.0
        if emode == "Indirect":
            efixed = self._get_e_fixed(output_workspace)
        if sample_unit != "Label":
            output_workspace = self._convert_units(output_workspace, sample_unit, emode, efixed)

        if output_workspace.name():
            RenameWorkspace(InputWorkspace=output_workspace, OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        self.setProperty("OutputWorkspace", output_workspace)
        prog_wrkflow.report("Algorithm Complete")

    def validateInputs(self):
        """
        Validate user input.
        """

        self._setup()
        issues = dict()

        # Need something to get corrections from
        if not (self._use_can or self._use_corrections):
            error_msg = "Must provide either CorrectionsWorkspace or CanWorkspace or both"
            issues["CorrectionsWorkspace"] = error_msg
            issues["CanWorkspace"] = error_msg

        if self._use_corrections:
            if not isinstance(self._corrections_workspace, WorkspaceGroup):
                issues["CorrectionsWorkspace"] = "The corrections workspace should be a workspace group."

            if self._corrections_workspace.size() == 0:
                issues["CorrectionsWorkspace"] = "No corrections found in the supplied corrections workspace group."
            else:
                corrections_issues = []

                for factor in self._factors:
                    if not any(factor in correction_name for correction_name in self._corrections_workspace.getNames()):
                        corrections_issues.append(factor + " workspace not present in corrections workspace group.\n")

                if corrections_issues:
                    issues["CorrectionsWorkspace"] = "\n".join(corrections_issues)

        if isinstance(self._sample_workspace, MatrixWorkspace):
            sample_unit_id = self._sample_workspace.getAxis(0).getUnit().unitID()

            # Check sample and container X axis units match
            if self._use_can:
                if isinstance(self._container_workspace, MatrixWorkspace):
                    can_unit_id = self._container_workspace.getAxis(0).getUnit().unitID()
                    if can_unit_id != sample_unit_id:
                        issues["CanWorkspace"] = "X axis unit must match SampleWorkspace"
                else:
                    issues["CanWorkspace"] = "Must be a MatrixWorkspace"
        else:
            issues["SampleWorkspace"] = "Must be a MatrixWorkspace"

        return issues

    def _setup(self):
        """
        Get properties and setup instance variables.
        """

        self._sample_workspace = self.getProperty("SampleWorkspace").value

        # Get corrections workspace
        self._corrections_workspace = self.getProperty("CorrectionsWorkspace").value
        self._use_corrections = bool(self._corrections_workspace)

        # Get container workspace
        self._container_workspace = self.getProperty("CanWorkspace").value
        self._use_can = bool(self._container_workspace)
        self._can_scale_factor = self.getProperty("CanScaleFactor").value
        self._scale_can = self._can_scale_factor != 1.0

        self._can_shift_factor = self.getProperty("CanShiftFactor").value
        self._shift_can = self._can_shift_factor != 0.0

        self._rebin_container_ws = self.getProperty("RebinCanToSample").value

        if self._use_corrections:
            if self._corrections_workspace.size() == 1:
                correction_name = self._corrections_workspace[0].name()
                if "acc" in correction_name:
                    self._factors = ["acc"]
                elif "ass" in correction_name:
                    self._factors = ["ass"]
            if self._corrections_workspace.size() == 2:
                self._factors = ["acc", "ass"]
                self._corrections_approximation = self._two_factor_corrections_approximation
            elif self._corrections_workspace.size() >= 3:
                self._factors = ["acc", "assc", "acsc"]
                self._corrections_approximation = self._three_factor_corrections_approximation

    def _shift_workspace(self, workspace, shift_factor):
        return ScaleX(InputWorkspace=workspace, Factor=shift_factor, OutputWorkspace="__shifted", Operation="Add", StoreInADS=False)

    def _convert_units_wavelength(self, workspace):
        unit = workspace.getAxis(0).getUnit().unitID()

        if unit != "Wavelength":
            # Configure conversion
            if unit == "dSpacing" or unit == "DeltaE":
                if unit == "dSpacing":
                    emode = "Elastic"
                    efixed = 0.0
                else:
                    emode = "Indirect"
                    efixed = self._get_e_fixed(workspace)
                return self._convert_units(workspace, "Wavelength", emode, efixed)
            else:
                # for fixed window scans the unit might be empty (e.g. temperature)
                return workspace
        else:
            return workspace

    def _convert_units(self, workspace, target, emode, efixed):
        return ConvertUnits(
            InputWorkspace=workspace, OutputWorkspace="__units_converted", Target=target, EMode=emode, EFixed=efixed, StoreInADS=False
        )

    def _get_e_fixed(self, workspace):
        from IndirectCommon import get_efixed

        return get_efixed(workspace)

    def _process_container_workspace(self, container_workspace, prog_container):
        # Appy container shift if needed
        if self._shift_can:
            # Use temp workspace so we don't modify data
            prog_container.report("Shifting can")
            shifted_container = self._shift_workspace(container_workspace, self._can_shift_factor)
            logger.information("Container shifted by %f" % self._can_shift_factor)
        else:
            shifted_container = container_workspace

        # Apply container scale factor if needed
        if self._scale_can:
            # Use temp workspace so we don't modify original data
            prog_container.report("Scaling can")
            scaled_container = self._convert_units_wavelength(shifted_container * self._can_scale_factor)
            logger.information("Container scaled by %f" % self._can_scale_factor)
            return scaled_container
        else:
            return self._convert_units_wavelength(shifted_container)

    def _get_correction_factor_workspace(self, factor_type):
        """
        Gets the full name for a correction factor workspace given the correction type.

        @param factor_type Factory type (ass, acc, acsc, assc)
        @return Full name of workspace (None if not found)
        """
        if factor_type == "ass":

            def predicate(workspace_name):
                return factor_type in workspace_name and "assc" not in workspace_name

        else:

            def predicate(workspace_name):
                return factor_type in workspace_name

        for workspace in self._corrections_workspace:
            if predicate(workspace.name()):
                return workspace
        return None

    def _get_factor_workspaces(self):
        """
        :return:    A dictionary of the factors to the factor workspaces.
        """
        return {factor: self._get_correction_factor_workspace(factor) for factor in self._factors}

    def _subtract(self, minuend_workspace, subtrahend_workspace):
        """
        Do a simple container subtraction (when no corrections are given).
        """

        logger.information("Using simple container subtraction")

        if self._rebin_container_ws:
            logger.information("Rebining container to ensure Minus")
            subtrahend_workspace = RebinToWorkspace(
                WorkspaceToRebin=subtrahend_workspace, WorkspaceToMatch=minuend_workspace, OutputWorkspace="__rebinned", StoreInADS=False
            )
        return minuend_workspace - subtrahend_workspace

    def _clone(self, workspace):
        """
        Clones the specified workspace.
        :param workspace:   The workspace to clone.
        :return:            A clone of the specified workspace.
        """
        return CloneWorkspace(InputWorkspace=workspace, OutputWorkspace="cloned", StoreInADS=False)

    def _correct_sample(self, sample_workspace, a_ss_workspace):
        """
        Correct for sample only (when no container is given).
        """
        logger.information("Correcting sample")
        correction_in_lambda = self._convert_units_wavelength(a_ss_workspace)
        corrected = Divide(LHSWorkspace=sample_workspace, RHSWorkspace=correction_in_lambda)
        return corrected

    def _correct_sample_can(self, sample_workspace, container_workspace, factor_workspaces):
        """
        Correct for sample and container.
        """

        logger.information("Correcting sample and container")

        factor_workspaces_wavelength = {
            factor: self._convert_units_wavelength(workspace) for factor, workspace in factor_workspaces.items()
        }

        if self._rebin_container_ws:
            container_workspace = RebinToWorkspace(
                WorkspaceToRebin=container_workspace,
                WorkspaceToMatch=factor_workspaces_wavelength["acc"],
                OutputWorkspace="rebinned",
                StoreInADS=False,
            )
        return self._corrections_approximation(sample_workspace, container_workspace, factor_workspaces_wavelength)

    def _three_factor_corrections_approximation(self, sample_workspace, container_workspace, factor_workspaces):
        acc = factor_workspaces["acc"]
        acsc = factor_workspaces["acsc"]
        assc = factor_workspaces["assc"]
        subtrahend = Multiply(container_workspace, (acsc / acc), StoreInADS=False)
        difference = Minus(sample_workspace, subtrahend, StoreInADS=False)
        quotient = Divide(difference, assc, OutputWorkspace="__quotient")
        return quotient

    def _two_factor_corrections_approximation(self, sample_workspace, container_workspace, factor_workspaces):
        acc = factor_workspaces["acc"]
        ass = factor_workspaces["ass"]
        minuend = Divide(sample_workspace, ass, StoreInADS=False)
        subtrahend = Divide(container_workspace, acc, StoreInADS=False)
        difference = Minus(minuend, subtrahend, OutputWorkspace="__difference")
        return difference


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ApplyPaalmanPingsCorrection)
