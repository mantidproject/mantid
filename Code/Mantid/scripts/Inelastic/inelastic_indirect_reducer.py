## IndirectEnergyConversionReducer class
from mantid.simpleapi import *

from msg_reducer import MSGReducer
import inelastic_indirect_reduction_steps as steps

class IndirectReducer(MSGReducer):
    """Reducer class for Inelastic Indirect Spectroscopy.

    Example for use:
    >> import inelastic_indirect_reducer as iir
    >> reducer = iir.IndirectReducer()
    >> reducer.set_instrument_name('IRIS')
    >> reducer.set_parameter_file('IRIS_graphite_002_Parameters.xml')
    >> reducer.set_detector_range(2,52)
    >> reducer.append_data_file('IRS21360.raw')
    >> reducer.reduce()

    Will perform the same steps as the ConvertToEnergy interface does on the
    default settings.
    """

    _grouping_policy = None
    _calibration_workspace = None
    _background_start = None
    _background_end = None
    _detailed_balance_temp = None
    _rename_result = True
    _save_to_cm_1 = False
    _scale_factor = None

    def __init__(self):
        """
        """
        super(IndirectReducer, self).__init__()
        self._grouping_policy = None
        self._calibration_workspace = None
        self._background_start = None
        self._background_end = None
        self._detailed_balance_temp = None
        self._rename_result = True
        self._scale_factor = None

    def _setup_steps(self):
        """**NB: This function is run automatically by the base reducer class
        and so does not require user interaction.**
        Setup the steps for the reduction. Please refer to the individual
        steps for details on their operation.
        """

        step = steps.IdentifyBadDetectors(MultipleFrames=self._multiple_frames)
        self.append_step(step)

        # "HandleMonitor" converts the monitor to Wavelength, possibly Unwraps
        step = steps.HandleMonitor(MultipleFrames=self._multiple_frames)
        self.append_step(step)

        # "BackgroundOperations" just does a CalculateFlatBackground at the moment,
        # will be extended for SNS stuff
        if (self._background_start is not None and
                self._background_end is not None):
            step = steps.BackgroundOperations(
                MultipleFrames=self._multiple_frames)
            step.set_range(self._background_start, self._background_end)
            self.append_step(step)

        # "ApplyCalibration" divides the workspace by the calibration workspace
        if self._calibration_workspace is not None:
            step = steps.ApplyCalibration()
            step.set_is_multiple_frames(self._multiple_frames)
            step.set_calib_workspace(self._calibration_workspace)
            self.append_step(step)

        # "CorrectByMonitor" converts the data into Wavelength, then divides by
        # the monitor workspace.
        step = steps.CorrectByMonitor(MultipleFrames=self._multiple_frames)
        self.append_step(step)

        # "ConvertToEnergy" runs ConvertUnits to DeltaE, CorrectKiKf, and also
        # Rebin if a rebin string has been specified.
        step = steps.ConvertToEnergy(MultipleFrames=self._multiple_frames)
        step.set_rebin_string(self._rebin_string)
        self.append_step(step)

        if self._detailed_balance_temp is not None:
            step = steps.DetailedBalance(MultipleFrames=self._multiple_frames)
            step.set_temperature(self._detailed_balance_temp)
            self.append_step(step)

        # Multiplies the scale by the factor specified.
        if self._scale_factor is not None:
            step = steps.Scaling(MultipleFrames=self._multiple_frames)
            step.set_scale_factor(self._scale_factor)
            self.append_step(step)

        step = steps.Grouping(MultipleFrames=self._multiple_frames)
        step.set_grouping_policy(self._grouping_policy)
        self.append_step(step)

        # "FoldData" puts workspaces that have been chopped back together.
        if self._multiple_frames:
            if self._fold_multiple_frames:
                self.append_step(steps.FoldData())
            else:
                return

        step = steps.ConvertToCm1(MultipleFrames=self._multiple_frames)
        step.set_save_to_cm_1(self._save_to_cm_1)
        self.append_step(step)

        # The "SaveItem" step saves the files in the requested formats.
        if len(self._save_formats) > 0:
            step = steps.SaveItem()
            step.set_formats(self._save_formats)
            step.set_save_to_cm_1(self._save_to_cm_1)
            self.append_step(step)

        if self._rename_result:
            step = steps.Naming()
            self.append_step(step)

    def set_grouping_policy(self, policy):
        self._grouping_policy = policy

    def set_save_to_cm_1(self, save_to_cm_1):
        self._save_to_cm_1 = save_to_cm_1

    def set_calibration_workspace(self, workspace):
        if not mtd.doesExist(workspace):
            raise ValueError("Selected calibration workspace not found.")
        self._calibration_workspace = workspace

    def set_background(self, start, end):
        self._background_start = float(start)
        self._background_end = float(end)

    def set_detailed_balance(self, temp):
        self._detailed_balance_temp = float(temp)

    def set_scale_factor(self, scaleFactor):
        self._scale_factor = float(scaleFactor)

    def set_rename(self, value):
        if not isinstance(value, bool):
            raise TypeError("value must be either True or False (boolean)")
        self._rename_result = value
