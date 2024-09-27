# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""SANSReductionCorePreprocess algorithm runs the sequence of reduction steps which are necessary to reduce a data set,
which can be performed before event slicing."""

from SANSReductionCoreBase import SANSReductionCoreBase

from mantid.api import MatrixWorkspaceProperty, AlgorithmFactory, Progress
from mantid.kernel import Direction
from sans_core.algorithm_detail.mask_workspace import mask_bins
from sans_core.common.enums import DetectorType


class SANSReductionCorePreprocess(SANSReductionCoreBase):
    def category(self):
        return "SANS\\Reduction"

    def summary(self):
        return "Runs the initial core reduction elements. These are the steps which " "can be carried out before event slicing."

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        # SANSReductionCorePreprocess has the same inputs as SANSReductionCore
        self._pyinit_input()

        # ----------
        # OUTPUT
        # ----------
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="The output workspace.")
        self.declareProperty(
            MatrixWorkspaceProperty("DummyMaskWorkspace", "", direction=Direction.Output),
            doc="The histogram workspace which contains bin masks for non-compatibility mode.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputMonitorWorkspace", "", direction=Direction.Output), doc="The output monitor workspace."
        )

    def PyExec(self):
        # Get the input
        state = self._get_state()
        component_as_string = self.getProperty("Component").value
        progress = self._get_progress()

        # --------------------------------------------------------------------------------------------------------------
        # 1. Crop workspace by detector name
        #    This will create a reduced copy of the original workspace with only those spectra which are relevant
        #    for this particular reduction.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Cropping ...")
        workspace = self._get_cropped_workspace(component_as_string)

        # --------------------------------------------------------------------------------------------
        # 2. Perform dark run subtraction
        #    This will subtract a dark background from the scatter workspace. Note that dark background subtraction
        #    will also affect the transmission calculation later on.
        # --------------------------------------------------------------------------------------------------------------

        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY BEGIN
        # IMPORTANT: This section of the code should only be temporary. It allows us to convert to histogram
        # early on and hence compare the new reduction results with the output of the new reduction chain.
        # Once the new reduction chain is established, we should remove the compatibility feature.
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        monitor_workspace = self._get_monitor_workspace()
        workspace, dummy_mask_workspace, use_dummy_workspace = self._check_compatibility_mode(
            workspace, monitor_workspace, state.compatibility
        )

        # ------------------------------------------------------------
        # 3. Move the workspace into the correct position
        #    The detectors in the workspaces are set such that the beam centre is at (0,0). The position is
        #    a user-specified value which can be obtained with the help of the beam centre finder.
        # ------------------------------------------------------------
        progress.report("Moving ...")
        workspace = self._move(state=state, workspace=workspace, component=component_as_string)
        monitor_workspace = self._move(state=state, workspace=monitor_workspace, component=component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 4. Apply masking (pixel masking and time masking)
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Masking ...")
        workspace = self._mask(state=state, workspace=workspace, component=component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 5. Convert to Wavelength
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Converting to wavelength ...")
        workspace = self._convert_to_wavelength(wavelength_state=state.wavelength, workspace=workspace)
        # Convert and rebin the dummy workspace to get correct bin flags
        if use_dummy_workspace:
            dummy_mask_workspace = mask_bins(state.mask, dummy_mask_workspace, DetectorType(component_as_string))
            dummy_mask_workspace = self._convert_to_wavelength(wavelength_state=state.wavelength, workspace=dummy_mask_workspace)

        # --------------------------------------------------------------------------------------------------------------
        # 6. Multiply by volume and absolute scale
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Multiplying by volume and absolute scale ...")
        workspace = self._scale(state=state, workspace=workspace)

        progress.report("Completed SANSReductionCorePreprocess ...")

        # ------------------------------------------------------------
        # Populate the output
        # ------------------------------------------------------------
        self.setProperty("OutputWorkspace", workspace)
        if use_dummy_workspace:
            self.setProperty("DummyMaskWorkspace", dummy_mask_workspace)
        self.setProperty("OutputMonitorWorkspace", monitor_workspace)

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        try:
            state = self._get_state()
            state.validate()
        except ValueError as err:
            errors.update({"SANSSingleReductionEventSlice": str(err)})
        return errors

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=6)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSReductionCorePreprocess)
