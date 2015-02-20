#pylint: disable=invalid-name
"""
    ISIS-specific implementation of the SANS Reducer.

    WARNING: I'm still playing around with the ISIS reduction to try to
    understand what's happening and how best to fit it in the Reducer design.

"""
from reducer_singleton import Reducer
import isis_reduction_steps
from reduction_settings import get_settings_object
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid.api import IEventWorkspace
import SANSUtility as su
import os
import copy

import sys

logger = Logger("ISISReducer")

################################################################################
# Avoid a bug with deepcopy in python 2.6, details and workaround here:
# http://bugs.python.org/issue1515
if sys.version_info[0] == 2 and sys.version_info[1] == 6:
    import types
    def _deepcopy_method(x, memo):
        return type(x)(x.im_func, copy.deepcopy(x.im_self, memo), x.im_class)
    copy._deepcopy_dispatch[types.MethodType] = _deepcopy_method
################################################################################

## Version number
__version__ = '0.0'

current_settings = None

class Sample(object):
    ISSAMPLE = True
    def __init__(self):
        #will contain a LoadSample() object that converts the run number into a file name and loads that file
        self.loader = None
        #geometry that comes from the run and can be overridden by user settings
        self.geometry = isis_reduction_steps.GetSampleGeom()
        #record options for the set_run
        self.run_option = None
        self.reload_option = None
        self.period_option = None

    def reload(self, reducer):
        """ When changing the detector bank for LOQ, it may be necessary to reload the
        data file, so to move te detector bank to the center of the scattering beam pattern.
        The reload method, allows to reload the data, moving to the correct center,
        and applying the same inputs used in the creation of the `Sample` object.
        """
        if self.run_option is None:
            raise RuntimeError('Trying to reload without set_run is impossible!')
        self.set_run(self.run_option,self.reload_option,self.period_option, reducer)

    def set_run(self, run, reload, period, reducer):
        """
            Assigns and load the run
            @param run: the run in a number.raw|nxs format
            @param reload: if this sample should be reloaded before the first reduction
            @param period: the period within the sample to be analysed
        """
        self.run_option = str(run) #to self-guard against keeping reference to workspace
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
        except:
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

        # currently, no slices will be applied to Can #8535
        for period in reversed(range(self.loader.periods_in_file)):
            self.loader.move2ws(period)
            name = self.loader.wksp_name
            if su.isEventWorkspace(name):
                su.fromEvent2Histogram(mtd[name], self.get_monitor())

class ISISReducer(Reducer):
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
    PHIMIN=-90.0
    PHIMAX=90.0
    PHIMIRROR=True

    ## Path for user settings files
    _user_file_path = '.'

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

        #the last step in the list must be ConvertToQ or can processing wont work
        self._conv_Q = proc_TOF + proc_wav + [self.to_Q]

        #list of steps to completely reduce a workspace
        self._reduction_steps = (self._conv_Q + self._can + self._tidy)

    def _init_steps(self):
        """
            Initialises the steps that are not initialised by (ISIS)CommandInterface.
        """
        #these steps are not executed by reduce
        self.user_settings =   None
        self._out_name =       isis_reduction_steps.GetOutputName()

        #except self.prep_normalize all the steps below are used by the reducer
        self.event2hist =      isis_reduction_steps.SliceEvent()
        self.crop_detector =   isis_reduction_steps.CropDetBank()
        self.mask = isis_reduction_steps.Mask_ISIS()
        self.to_wavelen =      isis_reduction_steps.UnitsConvert('Wavelength')
        self.norm_mon =        isis_reduction_steps.NormalizeToMonitor()
        self.transmission_calculator =\
                               isis_reduction_steps.TransmissionCalc(loader=None)
        self._corr_and_scale = isis_reduction_steps.AbsoluteUnitsISIS()

        # note CalculateNormISIS does not inherit from ReductionStep
        # so currently do not understand why it is in isis_reduction_steps
        # Also the main purpose of this class is to use it as an input argument
        # to ConvertToQ below
        self.prep_normalize = isis_reduction_steps.CalculateNormISIS(
                            [self.norm_mon, self.transmission_calculator])

        self.to_Q =            isis_reduction_steps.ConvertToQISIS(
                                                        self.prep_normalize)
        self._background_subtracter = isis_reduction_steps.CanSubtraction()
        self.geometry_correcter =       isis_reduction_steps.SampleGeomCor()
#        self._zero_error_flags=isis_reduction_steps.ReplaceErrors()
        self._rem_nans =      isis_reduction_steps.StripEndNans()

        self.set_Q_output_type(self.to_Q.output_type)
        # keep information about event slicing
        self._slices_def = []
        self._slice_index = 0


    def _clean_loaded_data(self):
        self._sample_run = Sample()
        self._can_run = Can()
        self.samp_trans_load = None
        self.can_trans_load = None

    def __init__(self):
        super(ISISReducer, self).__init__()
        self.output_wksp = None
        self.full_trans_wav = False
        self._monitor_set = False
        #workspaces that this reducer uses and will delete at the end
        self._temporys = {}
        #the output workspaces created by a data analysis
        self._outputs = {}
        #all workspaces created by this reducer
        self._workspace = [self._temporys, self._outputs]

        self._clean_loaded_data()
        self._init_steps()

        #process the background (can) run instead of the sample
        self._process_can = False
        #option to indicate if wide_angle_correction will be applied.
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
            raise RuntimeError('User settings must be loaded before the sample can be assigned, run UserFile() first')

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
        """ Get the transmission and direct workspace if they were given
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
            name += 'p'+str(period)

        name += self.instrument.cur_detector().name('short')
        name += '_' + self.to_Q.output_type
        name += '_' + self.to_wavelen.get_range()
        if self.to_Q.get_output_type() == "1D":
            name += self.mask.get_phi_limits_tag()

        if self.getNumSlices() > 0:
            limits = self.getCurrSliceLimit()
            if limits[0] != -1:
                name += '_t%.2f'%limits[0]
            if limits[1] != -1:
                name += '_T%.2f'%limits[1]

        return name

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

        #the main part of the reduction is done here, go through and execute each step
        for item in steps:
            if item:
                item.execute(self, self.output_wksp)

        #any clean up, possibly removing workspaces
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
            #the last step in the list must be ConvertToQ or this wont work
            can_steps = can_steps[0:len(can_steps)-1]

        #the reducer is completely setup, run it
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
            raise RuntimeError, "ISISReducer: trying to run a reduction with no instrument specified"

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
            user_file = 'None'
        else:
            user_file = self.user_settings.filename
        AddSampleLog(Workspace=self.output_wksp,LogName="UserFile", LogText=user_file)

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
            AddSampleLog(Workspace=self.output_wksp,LogName= "Transmission", LogText=self.__transmission_sample + str('_unfitted'))
        if self.__transmission_can:
            AddSampleLog(Workspace=self.output_wksp,LogName= "TransmissionCan", LogText=self.__transmission_can + str('_unfitted'))

        # clean these values for subsequent executions
        self.__transmission_sample = ""
        self.__transmission_can = ""

        for role in self._temporys.keys():
            try:
                DeleteWorkspace(Workspace=self._temporys[role])
            except:
            #if cleaning up isn't possible there is probably nothing we can do
                pass

    def set_user_path(self, path):
        """
            Set the path for user files
            @param path: user file path
        """
        if os.path.isdir(path):
            self._user_file_path = path
        else:
            raise RuntimeError, "ISISReducer.set_user_path: provided path is not a directory (%s)" % path

    def get_user_path(self):
        return self._user_file_path

    user_file_path = property(get_user_path, set_user_path, None, None)

    def set_trans_fit(self, lambda_min=None, lambda_max=None, fit_method="Log", selector='BOTH'):
        self.transmission_calculator.set_trans_fit(fit_method, lambda_min, lambda_max, override=True, selector=selector)

    def set_trans_sample(self, sample, direct, reload=True, period_t = -1, period_d = -1):
        self.samp_trans_load = isis_reduction_steps.LoadTransmissions(reload=reload)
        self.samp_trans_load.set_trans(sample, period_t)
        self.samp_trans_load.set_direc(direct, period_d)

    def set_trans_can(self, can, direct, reload = True, period_t = -1, period_d = -1):
        self.can_trans_load = isis_reduction_steps.LoadTransmissions(is_can=True, reload=reload)
        self.can_trans_load.set_trans(can, period_t)
        self.can_trans_load.set_direc(direct, period_d)

    def set_monitor_spectrum(self, specNum, interp=False, override=True):
        if override:
            self._monitor_set=True

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

    #quicker to write than .instrument
    inst = property(get_instrument, None, None, None)

    def Q_string(self):
        return '    Q range: ' + self.to_Q.binning +'\n    QXY range: ' + self.QXY2+'-'+self.DQXY

    def ViewCurrentMask(self):
        """
            In MantidPlot this opens InstrumentView to display the masked
            detectors in the bank in a different colour
        """
        self.mask.view(self.instrument)

    def reference(self):
        return self

    CENT_FIND_RMIN = None
    CENT_FIND_RMAX = None


    # override some functions from the Base Reduction

    # set_beam_finder: override to accept the front detector
    def set_beam_finder(self, finder, det_bank='rear'):
        """
            Extends teh SANS_reducer in order to support 2 bank detectors
            Set the ReductionStep object that finds the beam center
            @param finder: BaseBeamFinder object
            @param det_bank: two valid options: 'rear', 'front'
        """
        if issubclass(finder.__class__, isis_reduction_steps.BaseBeamFinder) or finder is None:
            if det_bank == 'front':
                self._front_beam_finder = finder
            else:
                self._beam_finder = finder
        else:
            raise RuntimeError, "Reducer.set_beam_finder expects an object of class ReductionStep"

    def get_beam_center(self, bank = None):
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
            if bank in ['front','FRONT','hab','HAB'] and self._front_beam_finder:
                return self._front_beam_finder.get_beam_center()
            else:
                return self._beam_finder.get_beam_center()

    def getCurrSliceLimit(self):
        if not self._slices_def:
            self._slices_def = su.sliceParser("")
            assert(self._slice_index == 0)
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
            except:
                #if the workspace can't be deleted this function does nothing
                pass

