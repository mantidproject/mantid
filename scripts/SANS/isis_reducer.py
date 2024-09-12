# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, property-on-old-class, redefined-builtin, protected-access
"""
ISIS-specific implementation of the SANS Reducer.

WARNING: I'm still playing around with the ISIS reduction to try to
understand what's happening and how best to fit it in the Reducer design.

"""

from reducer_singleton import Reducer
import isis_reduction_steps
import isis_instrument
from reduction_settings import get_settings_object
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid.api import IEventWorkspace
import SANSUtility as su
import os
import copy

logger = Logger("ISISReducer")


class ReductionStateTransferer(object):
    def __init__(self):
        super(ReductionStateTransferer, self).__init__()
        # A copy of the reducer
        self.rc = None

    def get_copy_of_reducer(self, reducer):
        self.rc = copy.deepcopy(reducer)

    def apply_gui_changes_from_old_reducer_to_new_reducer(self, reducer):
        # Apply detector
        det_name = self.rc.instrument.det_selection
        reducer.instrument.setDetector(det_name)

        # Apply output type
        reducer.to_Q.output_type = self.rc.to_Q.output_type

        # Get radius limit
        reducer.to_Q.r_cut = self.rc.to_Q.r_cut
        reducer.mask.min_radius = self.rc.mask.min_radius
        reducer.mask.max_radius = self.rc.mask.max_radius
        reducer.mask.max_radius = self.rc.mask.max_radius
        reducer.CENT_FIND_RMIN = self.rc.CENT_FIND_RMIN
        reducer.CENT_FIND_RMAX = self.rc.CENT_FIND_RMAX

        # Get events binning
        if hasattr(reducer, "settings") and hasattr(self.rc, "settings"):
            settings1 = reducer.settings
            settings2 = self.rc.settings
            if "events.binning" in settings1 and "events.binning" in settings2:
                reducer.settings["events.binning"] = copy.deepcopy(self.rc.settings["events.binning"])

        # Get wavelength limits
        reducer.to_Q.w_cut = self.rc.to_Q.w_cut

        # Get Q limits
        reducer.to_Q.binning = self.rc.to_Q.binning

        # Get QXY limits
        reducer.QXY2 = self.rc.QXY2
        reducer.DQXY = self.rc.DQXY

        # Get Phi Limits
        reducer.mask.phi_min = self.rc.mask.phi_min
        reducer.mask.phi_max = self.rc.mask.phi_max
        reducer.mask.phi_mirror = self.rc.mask.phi_mirror

        # Get flood files
        reducer.prep_normalize._high_angle_pixel_file = self.rc.prep_normalize._high_angle_pixel_file
        reducer.prep_normalize._low_angle_pixel_file = self.rc.prep_normalize._low_angle_pixel_file

        # Transmission fits
        reducer.transmission_calculator.fit_settings = copy.deepcopy(self.rc.transmission_calculator.fit_settings)

        # Set front detector scale, shift and q range
        reducer.instrument.getDetector("FRONT").rescaleAndShift = copy.deepcopy(self.rc.instrument.getDetector("FRONT").rescaleAndShift)
        reducer.instrument.getDetector("FRONT").mergeRange = copy.deepcopy(self.rc.instrument.getDetector("FRONT").mergeRange)

        # Set Gravity and extra length
        reducer.to_Q._use_gravity = self.rc.to_Q._use_gravity
        reducer.to_Q._grav_extra_length = self.rc.to_Q._grav_extra_length
        reducer.to_Q._grav_extra_length_set = self.rc.to_Q._grav_extra_length_set

        # Set sample offset
        reducer.instrument.SAMPLE_Z_CORR = self.rc.instrument.SAMPLE_Z_CORR

        # Set monitor spectrum
        reducer.instrument._use_interpol_norm = self.rc.instrument._use_interpol_norm
        reducer.instrument.set_incident_mon(self.rc.instrument.get_incident_mon())

        # Set transmission spectrum
        reducer.instrument.incid_mon_4_trans_calc = self.rc.instrument.incid_mon_4_trans_calc
        reducer.transmission_calculator.interpolate = self.rc.transmission_calculator.interpolate

        # Set transmission settings
        reducer.transmission_calculator.trans_mon = self.rc.transmission_calculator.trans_mon
        if hasattr(self.rc.instrument, "monitor_4_offset"):
            reducer.instrument.monitor_4_offset = self.rc.instrument.monitor_4_offset
        reducer.transmission_calculator.radius = self.rc.transmission_calculator.radius
        reducer.transmission_calculator.roi_files = self.rc.transmission_calculator.roi_files
        reducer.transmission_calculator.mask_files = self.rc.transmission_calculator.mask_files

        # Set q resolution settings
        reducer.to_Q.use_q_resolution = self.rc.to_Q.use_q_resolution
        reducer.to_Q._q_resolution_moderator_file_name = self.rc.to_Q._q_resolution_moderator_file_name
        reducer.to_Q._q_resolution_delta_r = self.rc.to_Q._q_resolution_delta_r
        reducer.to_Q._q_resolution_a1 = self.rc.to_Q._q_resolution_a1
        reducer.to_Q._q_resolution_a2 = self.rc.to_Q._q_resolution_a2
        reducer.to_Q._q_resolution_h1 = self.rc.to_Q._q_resolution_h1
        reducer.to_Q._q_resolution_w1 = self.rc.to_Q._q_resolution_w1
        reducer.to_Q._q_resolution_h2 = self.rc.to_Q._q_resolution_h2
        reducer.to_Q._q_resolution_w2 = self.rc.to_Q._q_resolution_w2
        reducer.to_Q._q_resolution_collimation_length = self.rc.to_Q._q_resolution_collimation_length

        # Set background correction settings
        reducer.dark_run_subtraction = copy.deepcopy(self.rc.dark_run_subtraction)

        # Set centre
        reducer._front_beam_finder = copy.deepcopy(self.rc._front_beam_finder)
        reducer._beam_finder = copy.deepcopy(self.rc._beam_finder)

        # Set mask
        reducer.mask.spec_mask_f = copy.deepcopy(self.rc.mask.spec_mask_f)
        reducer.mask.spec_mask_r = copy.deepcopy(self.rc.mask.spec_mask_r)
        reducer.mask.time_mask = copy.deepcopy(self.rc.mask.time_mask)
        reducer.mask.time_mask_f = copy.deepcopy(self.rc.mask.time_mask_f)
        reducer.mask.time_mask_r = copy.deepcopy(self.rc.mask.time_mask_r)
        reducer.mask.arm_width = copy.deepcopy(self.rc.mask.arm_width)
        reducer.mask.arm_angle = copy.deepcopy(self.rc.mask.arm_angle)
        reducer.mask.arm_x = copy.deepcopy(self.rc.mask.arm_x)
        reducer.mask.arm_y = copy.deepcopy(self.rc.mask.arm_y)

        # Add slices
        reducer._slices_def = copy.deepcopy(self.rc._slices_def)
        reducer._slice_index = copy.deepcopy(self.rc._slice_index)


## Version number
__version__ = "0.0"

current_settings = None


class Sample(object):
    ISSAMPLE = True

    def __init__(self):
        # will contain a LoadSample() object that converts the run number into a file name and loads that file
        self.loader = None
        # geometry that comes from the run and can be overridden by user settings
        self.geometry = isis_reduction_steps.GetSampleGeom()
        # record options for the set_run
        self.run_option = None
        self.reload_option = None
        self.period_option = None

    def reload(self, reducer):
        """When changing the detector bank for LOQ, it may be necessary to reload the
        data file, so to move te detector bank to the center of the scattering beam pattern.
        The reload method, allows to reload the data, moving to the correct center,
        and applying the same inputs used in the creation of the `Sample` object.
        """
        if self.run_option is None:
            raise RuntimeError("Trying to reload without set_run is impossible!")
        self.set_run(self.run_option, self.reload_option, self.period_option, reducer)

    def set_run(self, run, reload, period, reducer):
        """
        Assigns and load the run
        @param run: the run in a number.raw|nxs format
        @param reload: if this sample should be reloaded before the first reduction
        @param period: the period within the sample to be analysed
        """
        self.run_option = str(run)  # to self-guard against keeping reference to workspace
        self.reload_option = reload
        self.period_option = period

        self.loader = isis_reduction_steps.LoadSample(run, reload, period)
        self.loader.execute(reducer, self.ISSAMPLE)
        if self.ISSAMPLE:
            self.geometry.execute(None, self.get_wksp_name())

    def get_wksp_name(self):
        return self.loader.wksp_name

    def get_monitor(self, index=None):
        try:
            _ws = mtd[self.loader.wksp_name + "_monitors"]
        except (Exception, Warning):
            _ws = mtd[self.loader.wksp_name]

        if index is not None:
            __monitor = ExtractSingleSpectrum(_ws, index)
            return __monitor
        else:
            return _ws

    def get_periods_in_file(self):
        return self.loader.periods_in_file

    wksp_name = property(get_wksp_name, None, None, None)
    periods_in_file = property(get_periods_in_file, None, None, None)


class Can(Sample):
    ISSAMPLE = False

    def set_run(self, run, reload, period, reducer):
        super(Can, self).set_run(run, reload, period, reducer)


class ISISReducer(Reducer):
    # pylint: disable=too-many-public-methods
    """
    ISIS Reducer

    Inside ISIS there are two detector banks (low-angle and high-angle) and this particularity is responsible for
    requiring a special class to deal with SANS data reduction inside ISIS.

    For example, it requires the knowledge of the beam center inside the low-angle detector and the high-angle detector
    as well, and this cause to extend the method get_beam_center and set_beam_center to support both.

    TODO: need documentation for all the data member
    TODO: need to see whether all those data members really belong here
    """

    ## Beam center finder ReductionStep object
    _beam_finder = None
    _front_beam_finder = None

    QXY2 = None
    DQY = None

    # Component positions
    PHIMIN = -90.0
    PHIMAX = 90.0
    PHIMIRROR = True

    ## Path for user settings files
    _user_file_path = "."

    _can = None
    _tidy = None
    _conv_Q = None
    _reduction_steps = None
    user_settings = None
    _out_name = None
    event2hist = None
    crop_detector = None
    mask = None
    to_wavelen = None
    norm_mon = None
    transmission_calculator = None
    _corr_and_scale = None
    prep_normalize = None
    to_Q = None
    _background_subtracter = None
    geometry_correcter = None
    _rem_nans = None
    _sample_run = None
    _can_run = None
    _slices_def = None
    _slice_index = None
    samp_trans_load = None
    can_trans_load = None

    def _to_steps(self):
        """
        Defines the steps that are run and their order
        """
        proc_TOF = [self.event2hist]
        proc_TOF.append(self.crop_detector)
        proc_TOF.append(self.mask)
        proc_TOF.append(self.to_wavelen)

        proc_wav = [self.norm_mon]
        proc_wav.append(self.transmission_calculator)
        proc_wav.append(self._corr_and_scale)
        proc_wav.append(self.geometry_correcter)

        self._can = [self._background_subtracter]

        #        self._tidy = [self._zero_error_flags]
        self._tidy = [self._rem_nans]

        # the last step in the list must be ConvertToQ or can processing wont work
        self._conv_Q = proc_TOF + proc_wav + [self.to_Q]

        # list of steps to completely reduce a workspace
        self._reduction_steps = self._conv_Q + self._can + self._tidy

    def _init_steps(self):
        """
        Initialises the steps that are not initialised by (ISIS)CommandInterface.
        """
        # these steps are not executed by reduce
        self.user_settings = None
        self._out_name = isis_reduction_steps.GetOutputName()

        # except self.prep_normalize all the steps below are used by the reducer
        self.event2hist = isis_reduction_steps.SliceEvent()
        self.crop_detector = isis_reduction_steps.CropDetBank()
        self.mask = isis_reduction_steps.Mask_ISIS()
        self.to_wavelen = isis_reduction_steps.UnitsConvert("Wavelength")
        self.norm_mon = isis_reduction_steps.NormalizeToMonitor()
        self.transmission_calculator = isis_reduction_steps.TransmissionCalc(loader=None)
        self._corr_and_scale = isis_reduction_steps.AbsoluteUnitsISIS()

        # note CalculateNormISIS does not inherit from ReductionStep
        # so currently do not understand why it is in isis_reduction_steps
        # Also the main purpose of this class is to use it as an input argument
        # to ConvertToQ below
        self.prep_normalize = isis_reduction_steps.CalculateNormISIS([self.norm_mon, self.transmission_calculator])

        self.to_Q = isis_reduction_steps.ConvertToQISIS(self.prep_normalize)
        self._background_subtracter = isis_reduction_steps.CanSubtraction()
        self.geometry_correcter = isis_reduction_steps.SampleGeomCor()
        #        self._zero_error_flags=isis_reduction_steps.ReplaceErrors()
        self._rem_nans = isis_reduction_steps.StripEndNans()

        self.set_Q_output_type(self.to_Q.output_type)
        # keep information about event slicing
        self._slices_def = []
        self._slice_index = 0

        # As mentioned above, this is not a reductions step!

    def _clean_loaded_data(self):
        self._sample_run = Sample()
        self._can_run = Can()
        self.samp_trans_load = None
        self.can_trans_load = None

    def __init__(self):
        # pylint: disable=super-on-old-class
        super(ISISReducer, self).__init__()
        self.output_wksp = None
        self.full_trans_wav = False
        self._monitor_set = False
        # workspaces that this reducer uses and will delete at the end
        self._temporys = {}
        # the output workspaces created by a data analysis
        self._outputs = {}
        # all workspaces created by this reducer
        self._workspace = [self._temporys, self._outputs]

        self._clean_loaded_data()
        self._init_steps()

        # process the background (can) run instead of the sample
        self._process_can = False
        # option to indicate if wide_angle_correction will be applied.
        self.wide_angle_correction = False
        # Due to the way that ISISReducer is used to the reduction of the Can
        # creating a new copy of ISISReducer through the SingleTon interface, we have
        # to add this two attributes __transmission_sample and __transmission_can
        # to keep the values of the __transmission workspaces generated on the reduction.
        # register the value of transmission of sample
        self.__transmission_sample = ""
        # register the value of transmission can
        self.__transmission_can = ""

        self.settings = get_settings_object()

        # Dark Run Subtraction handler. This is not a step but a utility class
        # which gets used during cropping and Transmission calculation
        self.dark_run_subtraction = isis_reduction_steps.DarkRunSubtraction()

        # Unwrap monitors
        self._unwrap_monitors = False

    def set_instrument(self, configuration):
        """
        Sets the instrument and put in the default beam center (usually the
        center of the detector)
        @param configuration: instrument object
        """
        super(ISISReducer, self).set_instrument(configuration)
        center = self.instrument.get_default_beam_center()
        self._beam_finder = isis_reduction_steps.BaseBeamFinder(center[0], center[1])

    def set_sample(self, run, reload, period):
        """
        Assigns and load the run that this reduction chain will analysis
        @param run: the run in a number.raw|nxs format
        @param reload: if this sample should be reloaded before the first reduction
        @param period: the period within the sample to be analysed
        """
        if not self.user_settings.executed:
            raise RuntimeError("User settings must be loaded before the sample can be assigned, run UserFile() first")

        # At this point we need to check if the IDF associated with the
        self._match_IDF(run=run)

        # ensure that when you set sample, you start with no can, transmission previously used.
        self._clean_loaded_data()
        self._sample_run.set_run(run, reload, period, self)

    def set_can(self, run, reload, period):
        self._can_run.set_run(run, reload, period, self)

    def get_sample(self):
        """
        Gets information about the experimental run that is to be reduced
        @return: the object with information about the sample
        """
        if not self._process_can:
            return self._sample_run
        else:
            return self.get_can()

    def get_transmissions(self):
        """Get the transmission and direct workspace if they were given
        for the can and for the sample"""
        if self._process_can:
            loader = self.can_trans_load
        else:
            loader = self.samp_trans_load
        if loader:
            return loader.trans.wksp_name, loader.direct.wksp_name
        else:
            return "", ""

    def get_can(self):
        if self._can_run.loader and self._can_run.wksp_name:
            return self._can_run
        else:
            return None

    # for compatibility reason, previously, background_subtracter was used to
    # query if the can was provided and for the can reduction.
    background_subtracter = property(get_can, None, None, None)

    def get_out_ws_name(self, show_period=True):
        """
        Returns name of the workspace that will be created by this reduction
        which is based on the settings passed to the chain
        @param show_period: if True (default) the period (entry) number of the run is included in the name after a p
        @return: the workspace name to create
        """
        sample_obj = self.get_sample().loader
        name = str(sample_obj.shortrun_no)
        if show_period and (sample_obj.periods_in_file > 1 or sample_obj._period != -1):
            period = sample_obj.curr_period()
            name += "p" + str(period)

        name += self.instrument.cur_detector().name("short")
        name += "_" + self.to_Q.output_type
        name += "_" + self.to_wavelen.get_range()
        name += self.mask.get_phi_limits_tag()

        if self.getNumSlices() > 0:
            limits = self.getCurrSliceLimit()
            if limits[0] != -1:
                name += "_t%.2f" % limits[0]
            if limits[1] != -1:
                name += "_T%.2f" % limits[1]

        return name

    # pylint: disable=global-statement
    def deep_copy(self):
        """
        Returns a copy of the reducer that was created when the settings were set but
        before first execution
        @return: deep copy of the settings
        """
        global current_settings
        return copy.deepcopy(current_settings)

    def remove_settings(self):
        logger.debug("Clearing reducer settings.")
        global current_settings
        current_settings = None
        self.settings.clear()
        assert len(self.settings) == 0

    def cur_settings(self):
        """
        Retrieves the state of the reducer after it was setup and before running or None
        if the reducer hasn't been setup
        """
        return current_settings

    def is_can(self):
        """
        Indicates if the current run is a can reduction or not
        @return: True if the can is being processed
        """
        return self._process_can

    def _reduce(self, init=True, post=True, steps=None):
        """
        Execute the list of all reduction steps
        @param init: if False it assumes that the reducer is fully setup, default=True
        @param post: if to run the post run commands, default True
        @param steps: the list of ReductionSteps to execute, defaults to _reduction_steps if not set
        @return: name of the output workspace
        """
        if init:
            self.pre_process()

        if not steps:
            steps = self._reduction_steps

        # create the workspace that will be used throughout the reduction
        CloneWorkspace(self.get_sample().wksp_name, OutputWorkspace=self.output_wksp)

        # the main part of the reduction is done here, go through and execute each step
        for item in steps:
            if item:
                item.execute(self, self.output_wksp)

        # any clean up, possibly removing workspaces
        if post:
            self.post_process()

        return self.output_wksp

    def reduce_can(self, new_wksp=None, run_Q=True):
        """
        Apply the sample corrections to a can workspace. This reducer is deep copied
        and the output workspace name, transmission and monitor workspaces are changed.
        Then the reduction is applied to the given workspace
        @param new_wksp: the name of the workspace that will store the result
        @param run_Q: set to false to stop just before converting to Q, default is convert to Q
        """
        # copy settings
        sample_wksp_name = self.output_wksp
        sample_trans_name = self.transmission_calculator.output_wksp
        # configure can
        self._process_can = True
        # set the workspace that we've been setting up as the one to be processed
        self.output_wksp = new_wksp

        can_steps = self._conv_Q
        if not run_Q:
            # the last step in the list must be ConvertToQ or this wont work
            can_steps = can_steps[0 : len(can_steps) - 1]

        # the reducer is completely setup, run it
        self._reduce(init=False, post=False, steps=can_steps)

        # restore settings
        self._process_can = False
        self.output_wksp = sample_wksp_name
        self.__transmission_can = self.transmission_calculator.output_wksp
        self.__transmission_sample = sample_trans_name

    def run_from_raw(self):
        """
        Assumes the reducer is copied from a running one
        Executes all the steps after moving the components
        """
        self._out_name.execute(self)
        return self._reduce(init=False, post=True)

    def set_Q_output_type(self, out_type):
        self.to_Q.set_output_type(out_type)

    def pre_process(self):
        """
        Reduction steps that are meant to be executed only once per set
        of data files. After this is executed, all files will go through
        the list of reduction steps.
        """
        if self.instrument is None:
            raise RuntimeError("ISISReducer: trying to run a reduction with no instrument specified")

        if self._beam_finder is not None:
            result = self._beam_finder.execute(self)
            self.log_text += "%s\n" % str(result)

        # Create the list of reduction steps
        self._to_steps()

        self._out_name.execute(self)
        global current_settings
        current_settings = copy.deepcopy(self)

    def post_process(self):
        # Store the mask file within the final workspace so that it is saved to the CanSAS file
        if self.user_settings is None:
            user_file = "None"
        else:
            user_file = self.user_settings.filename
        AddSampleLog(Workspace=self.output_wksp, LogName="UserFile", LogText=user_file)

        # get the value of __transmission_sample from the transmission_calculator if it has
        if (not self.get_can()) and self.transmission_calculator.output_wksp:
            # it updates only if there was not can, because, when there is can, the __transmission_sample
            # is already correct and transmission_calculator.output_wksp points to the can transmission
            self.__transmission_sample = self.transmission_calculator.output_wksp

        # The reducer itself sometimes will be reset, and the users of the singleton
        # not always will have access to its settings. So, we will add the transmission workspaces
        # to the SampleLog, to be connected to the workspace, and be available outside. These values
        # are current being used for saving CanSAS (ticket #6929)
        if self.__transmission_sample:
            unfitted_transmission_workspace_name = su.get_unfitted_transmission_workspace_name(self.__transmission_sample)
            AddSampleLog(Workspace=self.output_wksp, LogName="Transmission", LogText=unfitted_transmission_workspace_name)
        if self.__transmission_can:
            unfitted_transmission_workspace_name = su.get_unfitted_transmission_workspace_name(self.__transmission_can)
            AddSampleLog(Workspace=self.output_wksp, LogName="TransmissionCan", LogText=unfitted_transmission_workspace_name)

        # clean these values for subsequent executions
        self.__transmission_sample = ""
        self.__transmission_can = ""

        for role in list(self._temporys.keys()):
            try:
                DeleteWorkspace(Workspace=self._temporys[role])
            except (Exception, Warning):
                # if cleaning up isn't possible there is probably nothing we can do
                pass

    def set_user_path(self, path):
        """
        Set the path for user files
        @param path: user file path
        """
        if os.path.isdir(path):
            self._user_file_path = path
        else:
            raise RuntimeError("ISISReducer.set_user_path: provided path is not a directory (%s)" % path)

    def get_user_path(self):
        return self._user_file_path

    user_file_path = property(get_user_path, set_user_path, None, None)

    def set_trans_fit(self, lambda_min=None, lambda_max=None, fit_method="Log", selector="BOTH"):
        self.transmission_calculator.set_trans_fit(fit_method, lambda_min, lambda_max, override=True, selector=selector)

    # pylint: disable=too-many-arguments
    def set_trans_sample(self, sample, direct, reload=True, period_t=-1, period_d=-1):
        self.samp_trans_load = isis_reduction_steps.LoadTransmissions(reload=reload)
        self.samp_trans_load.set_trans(sample, period_t)
        self.samp_trans_load.set_direc(direct, period_d)

    def set_trans_can(self, can, direct, reload=True, period_t=-1, period_d=-1):
        self.can_trans_load = isis_reduction_steps.LoadTransmissions(is_can=True, reload=reload)
        self.can_trans_load.set_trans(can, period_t)
        self.can_trans_load.set_direc(direct, period_d)

    def set_monitor_spectrum(self, specNum, interp=False, override=True):
        if override:
            self._monitor_set = True

        self.instrument.set_interpolating_norm(interp)

        if not self._monitor_set or override:
            self.instrument.set_incident_mon(specNum)

    def set_trans_spectrum(self, specNum, interp=False, override=True):
        self.instrument.incid_mon_4_trans_calc = int(specNum)

        self.transmission_calculator.interpolate = interp

    def step_num(self, step):
        """
        Returns the index number of a step in the
        list of steps that have _so_ _far_ been
        added to the chain
        """
        return self._reduction_steps.index(step)

    def get_instrument(self):
        """
        Convenience function used by the inst property to make
        calls shorter
        """
        return self.instrument

    def get_instrument_name(self):
        """
        Get the name of the instrument
        """
        return self.instrument._NAME

    def get_idf_file_path(self):
        """
        Get the IDF path
        """
        return self.instrument.get_idf_file_path()

    # quicker to write than .instrument
    inst = property(get_instrument, None, None, None)

    def Q_string(self):
        return "    Q range: " + self.to_Q.binning + "\n    QXY range: " + self.QXY2 + "-" + self.DQXY

    def reference(self):
        return self

    CENT_FIND_RMIN = None
    CENT_FIND_RMAX = None

    # override some functions from the Base Reduction

    # set_beam_finder: override to accept the front detector
    def set_beam_finder(self, finder, det_bank="rear"):
        """
        Extends the SANS_reducer in order to support 2 bank detectors
        Set the ReductionStep object that finds the beam center
        @param finder: BaseBeamFinder object
        @param det_bank: two valid options: 'rear', 'front'
        """
        if issubclass(finder.__class__, isis_reduction_steps.BaseBeamFinder) or finder is None:
            if det_bank == "front":
                self._front_beam_finder = finder
            else:
                self._beam_finder = finder
        else:
            raise RuntimeError("Reducer.set_beam_finder expects an object of class ReductionStep")

    def get_beam_center(self, bank=None):
        """
        Return the beam center position according to the
        bank detector current selected.

        From its instrument, this class is able to find what
        is the current selected detector, and them return the
        beam center according to this bank.

        Another possibility is to direct ask for the beam_center
        related to a specified detector bank.
        @param bank: Optional : front
        """
        if bank is None:
            if self.instrument.lowAngDetSet or not self._front_beam_finder:
                return self._beam_finder.get_beam_center()
            else:
                return self._front_beam_finder.get_beam_center()
        else:
            if bank in ["front", "FRONT", "hab", "HAB"] and self._front_beam_finder:
                return self._front_beam_finder.get_beam_center()
            else:
                return self._beam_finder.get_beam_center()

    def get_beam_center_scale_factor1(self):
        """
        Return the beam center scale factor 1 defined in the parameter file.
        """
        return self.instrument.beam_centre_scale_factor1

    def get_beam_center_scale_factor2(self):
        """
        Return the beam center scale factor 2 defined in the parameter file.
        """
        return self.instrument.beam_centre_scale_factor2

    def getCurrSliceLimit(self):
        if not self._slices_def:
            self._slices_def = su.sliceParser("")
            assert self._slice_index == 0
        return self._slices_def[self._slice_index]

    def getNumSlices(self):
        # slices are defined only for event workspaces
        ws = mtd[self.get_sample().wksp_name]
        if not isinstance(ws, IEventWorkspace):
            return 0
        if not self._slices_def:
            return 0
        return len(self._slices_def)

    def setSliceIndex(self, index):
        if index < self.getNumSlices():
            self._slice_index = index
        else:
            raise IndexError("Outside range")

    def setSlicesLimits(self, str_def):
        self._slices_def = su.sliceParser(str_def)
        self._slice_index = 0

    def deleteWorkspaces(self, workspaces):
        """
        Deletes a list of workspaces if they exist but ignores any errors
        """
        for wk in workspaces:
            try:
                if wk and wk in mtd:
                    DeleteWorkspace(Workspace=wk)
            except (Exception, Warning):
                # if the workspace can't be deleted this function does nothing
                pass

    def get_reduction_steps(self):
        """
        Provides a way to access the reduction steps
        @returns the reduction steps
        """
        return self._reduction_steps

    def perform_consistency_check(self):
        """
        Runs the consistency check over all reduction steps
        """
        was_empty = False
        if not self._reduction_steps:
            self._to_steps()
            was_empty = True

        try:
            to_check = self._reduction_steps
            for element in to_check:
                element.run_consistency_check()
        except RuntimeError as details:
            if was_empty:
                self._reduction_steps = None
            raise RuntimeError(str(details))

    def update_beam_center(self):
        """
        Gets a possible new beam center position from the instrument after translation
        or rotation. Previously this was not necessary as the reducer told the instrument
        how to position, but now the instrument can get further positioning information
        from the Instrument Parameter File.
        """
        centre_pos1, centre_pos2 = self.instrument.get_updated_beam_centre_after_move()
        # Update the beam centre finder for the rear
        self._beam_finder.update_beam_center(centre_pos1, centre_pos2)

    def _match_IDF(self, run):
        """
        Compares the IDF in the stored instrument with the IDF in the workspace.
        If they are the same all is well. If they diff, then load the adequate
        user file.
        @param run: name of the run for which the file is to be extracted
        """
        # We need the instrument name and the measurement time to determine
        # the IDF
        measurement_time = None
        instrument_name = self.get_instrument_name()
        # We need to be able to handle file-based and workspace-based queries
        # If we have a workspace we look at the end time, else we
        # need a sophisticated extraction mechanism
        if isinstance(run, Workspace):
            ws = None
            if isinstance(run, WorkspaceGroup):
                # Just look at the first element in a workspace group
                ws = run[0]
            else:
                ws = run
            measurement_time = str(ws.getRun().endTime()).strip()
        else:
            if run is None or run == "":
                return
            measurement_time = su.get_measurement_time_from_file(run)

        # Get the path to the instrument definition file
        idf_path_workspace = ExperimentInfo.getInstrumentFilename(instrument_name, measurement_time)
        idf_path_workspace = os.path.normpath(idf_path_workspace)

        # Get the idf from the reducer
        idf_path_reducer = self.get_idf_file_path()
        idf_path_reducer = os.path.normpath(idf_path_reducer)

        # Now check both underlying files. If they are equal, then don't do anything
        # else switch the underlying instrument
        if su.are_two_files_identical(idf_path_workspace, idf_path_reducer):
            return
        else:
            logger.notice("Updating the IDF of the Reducer. Switching from " + str(idf_path_reducer) + " to " + str(idf_path_workspace))
            idf_path = os.path.basename(idf_path_workspace)
            instrument = self._get_correct_instrument(instrument_name, idf_path)

            # Get detector of the old instrument
            old_instrument = self.get_instrument()
            old_detector_selection = old_instrument.get_detector_selection()

            if instrument is not None:
                # Read the values of some variables which are set on the reduction
                # framework.
                state_transfer = ReductionStateTransferer()
                state_transfer.get_copy_of_reducer(self)

                self.set_instrument(instrument)

                # We need to update the instrument, by reloading the user file.
                # This is pretty bad, but looking at the reducer architecture this
                # seems to be the only reasonable way to do this.
                self.user_settings.execute(self)

                # Apply the settings to reloaded reduction framework
                state_transfer.apply_gui_changes_from_old_reducer_to_new_reducer(self)

                # Now we set the correct detector, this is also being done in the GUI
                self.get_instrument().setDetector(old_detector_selection)

    def _get_correct_instrument(self, instrument_name, idf_path=None):
        """
        Creates an ISIS instrument based on the name and the chosen idf_path
        @param instrument_name: the name of the instrument
        @param idf_path: the full path to the IDF
        """
        instrument = None
        try:
            if instrument_name.upper() == "LARMOR":
                instrument = isis_instrument.LARMOR(idf_path)
            elif instrument_name.upper() == "SANS2D":
                instrument = isis_instrument.SANS2D(idf_path)
            elif instrument_name.upper() == "LOQ":
                instrument = isis_instrument.LOQ(idf_path)
        # pylint: disable=bare-except
        except:
            instrument = None
        return instrument

    def add_dark_run_setting(self, dark_run_setting):
        """
        Adds a dark run setting to the dark run subtraction
        @param dark_run_setting: a dark run setting
        """
        self.dark_run_subtraction.add_setting(dark_run_setting)

    def get_dark_run_setting(self, is_time, is_mon):
        """
        Gets one of the four dark run settings
        @param is_time: is it time_based or not
        @param is_mon: monitors or not
        @returns the requested setting
        """
        setting = None
        if is_time and is_mon:
            setting = self.dark_run_subtraction.get_time_based_setting_monitors()
        elif is_time and not is_mon:
            setting = self.dark_run_subtraction.get_time_based_setting_detectors()
        elif not is_time and is_mon:
            setting = self.dark_run_subtraction.get_uamp_based_setting_monitors()
        elif not is_time and not is_mon:
            setting = self.dark_run_subtraction.get_uamp_based_setting_detectors()
        return setting

    def clear_dark_run_settings(self):
        self.dark_run_subtraction.clear_settings()

    def is_based_on_event(self):
        """
        One way to determine if we are dealing with an original event workspace
        is if the monitor workspace was loaded separately
        @returns true if the input was an event workspace
        """
        was_event = False
        if self.is_can():
            can = self.get_can()
            if can.loader.wksp_name + "_monitors" in mtd.getObjectNames():
                was_event = True
        else:
            sample = self.get_sample()
            if sample.loader.wksp_name + "_monitors" in mtd.getObjectNames():
                was_event = True
        return was_event

    def get_unwrap_monitors(self):
        return self._unwrap_monitors

    def set_unwrap_monitors(self, value):
        self._unwrap_monitors = value

    unwrap_monitors = property(get_unwrap_monitors, set_unwrap_monitors, None, None)
