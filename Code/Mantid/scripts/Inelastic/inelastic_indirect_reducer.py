## IndirectEnergyConversionReducer class
from mantidsimple import *

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
    
    _rebin_string = None
    _grouping_policy = None
    _calibration_workspace = None
    _background_start = None
    _background_end = None
    _detailed_balance_temp = None
    _rename_result = True
    _save_formats = []
    
    def __init__(self):
        """
        """
        super(IndirectReducer, self).__init__()
        self._rebin_string = None
        self._grouping_policy = None
        self._calibration_workspace = None
        self._background_start = None
        self._background_end = None
        self._detailed_balance_temp = None
        self._rename_result = True
        self._save_formats = []
        
    def _setup_steps(self):
        """**NB: This function is run automatically by the base reducer class
        and so does not require user interaction.**
        Setup the steps for the reduction. Please refer to the individual
        steps for details on their operation.
        """
        
        # "HandleMonitor" converts the monitor to Wavelength, possibly Unwraps
        step = steps.HandleMonitor(MultipleFrames=self._multiple_frames)
        self.append_step(step)
        
        # "BackgroundOperations" just does a FlatBackground at the moment,
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
            
        step = steps.Grouping(MultipleFrames=self._multiple_frames)
        step.set_grouping_policy(self._grouping_policy)
        step.set_mask_list(self._masking_detectors)
        self.append_step(step)
        
        # "FoldData" puts workspaces that have been chopped back together.
        if self._multiple_frames:
            self.append_step(steps.FoldData())
            
        # The "SaveItem" step saves the files in the requested formats.
        if (len(self._save_formats) > 0):
            step = steps.SaveItem()
            step.set_formats(self._save_formats)
            self.append_step(step)
        
        if self._rename_result:
            step = steps.Naming()
            self.append_step(step)
    
    def set_save_formats(self, formats):
        """Selects the save formats in which to export the reduced data.
        formats should be a list object of strings containing the file
        extension that signifies the type.
        For example:
            reducer.set_save_formats(['nxs', 'spe'])
        Tells the reducer to save the final result as a NeXuS file, and as an
        SPE file.
        Please see the documentation for the SaveItem reduction step for more
        details.
        """
        if not isinstance(formats, list):
            raise TypeError("formats variable must be of list type")
        self._save_formats = formats
        
    def set_rebin_string(self, rebin):
        if not isinstance(rebin, str):
            raise TypeError("rebin variable must be of string type")
        self._rebin_string = rebin

    def set_grouping_policy(self, policy):
        self._grouping_policy = policy

    def set_calibration_workspace(self, workspace):
        if mtd[workspace] is None:
            raise ValueError("Selected calibration workspace not found.")
        self._calibration_workspace = workspace
        
    def set_background(self, start, end):
        self._background_start = float(start)
        self._background_end = float(end)
        
    def set_detailed_balance(self, temp):
        self._detailed_balance_temp = float(temp)
        
    def get_result_workspaces(self):
        nsteps = len(self._reduction_steps)
        for i in range(0, nsteps):
            try:
                step = self._reduction_steps[nsteps-(i+1)]
                return step.get_result_workspaces()
            except AttributeError:
                pass
            except IndexError:
                raise RuntimeError("None of the reduction steps implement "
                    "the get_result_workspaces() method.")
        
    def set_rename(self, value):
        if not isinstance(value, bool):
            raise TypeError("value must be either True or False (boolean)")
        self._rename_result = value
