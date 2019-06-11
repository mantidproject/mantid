# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

""" SANSReductionCore algorithm runs the sequence of reduction steps which are necessary to reduce a data set."""

from __future__ import (absolute_import, division, print_function)
from mantid.api import AlgorithmFactory, Progress

from sans.algorithm_detail.mask_workspace import mask_bins
from sans.common.enums import (DetectorType, DataType)

from SANSReductionCoreBase import SANSReductionCoreBase


class SANSReductionCore(SANSReductionCoreBase):
    def category(self):
        return 'SANS\\Reduction'

    def summary(self):
        return ' Runs the the core reduction elements.'

    def PyInit(self):
        self._pyinit_input()
        self._pyinit_output()

    def PyExec(self):
        # Get the input
        state = self._get_state()
        state_serialized = state.property_manager
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

        # --------------------------------------------------------------------------------------------------------------
        # 3. Create event slice
        #    If we are dealing with an event workspace as input, this will cut out a time-based (user-defined) slice.
        #    In case of a histogram workspace, nothing happens.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Event slicing ...")
        data_type_as_string = self.getProperty("DataType").value
        monitor_workspace = self._get_monitor_workspace()
        workspace, monitor_workspace, slice_event_factor = self._slice(state_serialized, workspace, monitor_workspace,
                                                                       data_type_as_string)

        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        # COMPATIBILITY
        # The old reduction workflow converted the workspace to a histogram at this point.
        # A more recent workflow keeps the workspaces as Events for longer, to make use of cheap rebinning for
        # EventWorkspaces, and to optimise for event slicing.
        # However, in the new workflow ("non-compatibility mode") it is necessary to keep a workspace as a histogram
        # to keep track of the bin masking. These masks are lifted from the dummy workspace to the actual workspace
        # near the end of the reduction.
        # !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        workspace, dummy_mask_workspace, \
            use_dummy_workspace = self._check_compatibility_mode(workspace, monitor_workspace, state.compatibility)

        # ------------------------------------------------------------
        # 4. Move the workspace into the correct position
        #    The detectors in the workspaces are set such that the beam centre is at (0,0). The position is
        #    a user-specified value which can be obtained with the help of the beam centre finder.
        # ------------------------------------------------------------
        progress.report("Moving ...")
        workspace = self._move(state_serialized, workspace, component_as_string)
        monitor_workspace = self._move(state_serialized, monitor_workspace, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 5. Apply masking (pixel masking and time masking)
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Masking ...")
        workspace = self._mask(state_serialized, workspace, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 6. Convert to Wavelength
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Converting to wavelength ...")
        workspace = self._convert_to_wavelength(state_serialized, workspace)
        # Convert and rebin the dummy workspace to get correct bin flags
        if use_dummy_workspace:
            dummy_mask_workspace = mask_bins(state.mask, dummy_mask_workspace, DetectorType.from_string(component_as_string))
            dummy_mask_workspace = self._convert_to_wavelength(state_serialized, dummy_mask_workspace)

        # --------------------------------------------------------------------------------------------------------------
        # 7. Multiply by volume and absolute scale
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Multiplying by volume and absolute scale ...")
        workspace = self._scale(state_serialized, workspace)

        # --------------------------------------------------------------------------------------------------------------
        # 8. Create adjustment workspaces, those are
        #     1. pixel-based adjustments
        #     2. wavelength-based adjustments
        #     3. pixel-and-wavelength-based adjustments
        # Note that steps 4 to 7 could run in parallel if we don't use wide angle correction. If we do then the
        # creation of the adjustment workspaces requires the sample workspace itself and we have to run it sequentially.
        # We could consider to have a serial and a parallel strategy here, depending on the wide angle correction
        # settings. On the other hand it is not clear that this would be an advantage with the GIL.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Creating adjustment workspaces ...")
        wavelength_adjustment_workspace, pixel_adjustment_workspace, wavelength_and_pixel_adjustment_workspace, \
            calculated_transmission_workspace, unfitted_transmission_workspace = \
            self._adjustment(state_serialized, workspace, monitor_workspace, component_as_string, data_type_as_string)

        # ----------------------------------------------------------------
        # 9. Convert event workspaces to histogram workspaces, and re-mask
        # ----------------------------------------------------------------
        progress.report("Converting to histogram mode ...")
        workspace = self._convert_to_histogram(workspace)
        if use_dummy_workspace:
            workspace = self._copy_bin_masks(workspace, dummy_mask_workspace)

        # ------------------------------------------------------------
        # 10. Convert to Q
        # -----------------------------------------------------------
        progress.report("Converting to q ...")
        workspace, sum_of_counts, sum_of_norms = self._convert_to_q(state_serialized,
                                                                    workspace,
                                                                    wavelength_adjustment_workspace,
                                                                    pixel_adjustment_workspace,
                                                                    wavelength_and_pixel_adjustment_workspace)
        progress.report("Completed SANSReductionCore ...")

        # ------------------------------------------------------------
        # Populate the output
        # ------------------------------------------------------------
        self.setProperty("OutputWorkspace", workspace)

        # ------------------------------------------------------------
        # Diagnostic output
        # ------------------------------------------------------------
        if sum_of_counts:
            self.setProperty("SumOfCounts", sum_of_counts)
        if sum_of_norms:
            self.setProperty("SumOfNormFactors", sum_of_norms)

        self.setProperty("CalculatedTransmissionWorkspace", calculated_transmission_workspace)
        self.setProperty("UnfittedTransmissionWorkspace", unfitted_transmission_workspace)

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        try:
            state = self._get_state()
            state.validate()
        except ValueError as err:
            errors.update({"SANSSingleReduction": str(err)})
        return errors

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=10)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSReductionCore)
