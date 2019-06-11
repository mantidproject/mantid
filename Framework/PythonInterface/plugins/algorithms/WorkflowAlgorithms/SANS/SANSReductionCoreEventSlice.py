# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

""" SANSReductionCoreEventSlice algorithm runs the sequence of reduction steps which are necessary to reduce a data set,
for which data must be event sliced. These steps are: slicing, adjustment, convert to q."""

from __future__ import (absolute_import, division, print_function)

from mantid.api import (MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        Progress)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from sans.common.enums import (DetectorType, DataType)

from SANSReductionCoreBase import SANSReductionCoreBase


class SANSReductionCoreEventSlice(SANSReductionCoreBase):
    def category(self):
        return 'SANS\\Reduction'

    def summary(self):
        return 'Runs the the core reduction elements which need to be carried out ' \
               'on individual event slices.'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        # WORKSPACES
        # Scatter Workspaces
        self.declareProperty(MatrixWorkspaceProperty('ScatterWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The scatter workspace. This workspace does not contain monitors.')
        self.declareProperty(MatrixWorkspaceProperty('DummyMaskWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The histogram workspace containing mask bins for the event workspace, to be copied '
                                 'over after event slicing.')
        self.declareProperty(MatrixWorkspaceProperty('ScatterMonitorWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The scatter monitor workspace. This workspace only contains monitors.')
        # Direct Workspace
        self.declareProperty(MatrixWorkspaceProperty('DirectWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The direct workspace.')
        # Transmission Workspace
        self.declareProperty(MatrixWorkspaceProperty('TransmissionWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='The transmission workspace')

        self.setPropertyGroup("ScatterWorkspace", 'Data')
        self.setPropertyGroup("ScatterMonitorWorkspace", 'Data')
        self.setPropertyGroup("DummyMaskWorkspace", 'Data')
        self.setPropertyGroup("DirectWorkspace", 'Data')
        self.setPropertyGroup("TransmissionWorkspace", 'Data')

        # The component
        allowed_detectors = StringListValidator([DetectorType.to_string(DetectorType.LAB),
                                                 DetectorType.to_string(DetectorType.HAB)])
        self.declareProperty("Component", DetectorType.to_string(DetectorType.LAB),
                             validator=allowed_detectors, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

        # The data type
        allowed_data = StringListValidator([DataType.to_string(DataType.Sample),
                                            DataType.to_string(DataType.Can)])
        self.declareProperty("DataType", DataType.to_string(DataType.Sample),
                             validator=allowed_data, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

        # ----------
        # OUTPUT
        # ----------
        # SANSReductionCoreEventSlice has the same outputs as SANSReductionCore
        self._pyinit_output()

    def PyExec(self):
        # Get the input
        state = self._get_state()
        state_serialized = state.property_manager
        progress = self._get_progress()

        workspace = self.getProperty("ScatterWorkspace").value
        # --------------------------------------------------------------------------------------------------------------
        # 1. Create event slice
        #    If we are dealing with an event workspace as input, this will cut out a time-based (user-defined) slice.
        #    In case of a histogram workspace, nothing happens.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Event slicing ...")
        data_type_as_string = self.getProperty("DataType").value
        monitor_workspace = self._get_monitor_workspace()
        workspace, monitor_workspace, slice_event_factor = self._slice(state_serialized, workspace, monitor_workspace,
                                                                       data_type_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 7. Create adjustment workspaces, those are
        #     1. pixel-based adjustments
        #     2. wavelength-based adjustments
        #     3. pixel-and-wavelength-based adjustments
        # Note that steps 4 to 7 could run in parallel if we don't use wide angle correction. If we do then the
        # creation of the adjustment workspaces requires the sample workspace itself and we have to run it sequentially.
        # We could consider to have a serial and a parallel strategy here, depending on the wide angle correction
        # settings. On the other hand it is not clear that this would be an advantage with the GIL.
        # --------------------------------------------------------------------------------------------------------------
        component_as_string = self.getProperty("Component").value
        data_type_as_string = self.getProperty("DataType").value
        progress.report("Creating adjustment workspaces ...")
        wavelength_adjustment_workspace, pixel_adjustment_workspace, wavelength_and_pixel_adjustment_workspace, \
            calculated_transmission_workspace, unfitted_transmission_workspace = \
            self._adjustment(state_serialized, workspace, monitor_workspace, component_as_string, data_type_as_string)

        # ------------------------------------------------------------
        # 2. Convert event workspaces to histogram workspaces
        # ------------------------------------------------------------
        progress.report("Converting to histogram mode ...")
        workspace = self._convert_to_histogram(workspace)

        # ------------------------------------------------------------
        # 3. Re-mask. We need to bin mask in histogram mode in order
        #    to have knowledge of masked regions: masking
        #    EventWorkspaces simply removes their events
        # ------------------------------------------------------------
        dummy_mask_workspace = self.getProperty("DummyMaskWorkspace").value
        workspace = self._copy_bin_masks(workspace, dummy_mask_workspace)

        # ------------------------------------------------------------
        # 3. Convert to Q
        # -----------------------------------------------------------
        progress.report("Converting to q ...")
        workspace, sum_of_counts, sum_of_norms = self._convert_to_q(state_serialized,
                                                                    workspace,
                                                                    wavelength_adjustment_workspace,
                                                                    pixel_adjustment_workspace,
                                                                    wavelength_and_pixel_adjustment_workspace)

        progress.report("Completed SANSReductionCoreEventSlice...")

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
            errors.update({"SANSSingleReductionEventSlice": str(err)})
        return errors

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=5)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSReductionCoreEventSlice)
