# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-lines, too-many-branches, invalid-name, super-on-old-class, protected-access,
# pylint: disable=too-few-public-methods,too-few-public-methods, too-many-arguments, too-many-instance-attributes
"""
This file defines what happens in each step in the data reduction, it's
the guts of the reduction. See ISISReducer for order the steps are run
in and the names they are given to identify them

Most of this code is a copy-paste from SANSReduction.py, organized to be used with
ReductionStep objects. The guts needs refactoring.
"""

import os
import re
import math
from collections import namedtuple
from mantid.kernel import Logger

from mantid.api import WorkspaceGroup, Workspace, IEventWorkspace
from mantid.simpleapi import *
from SANSUtility import (
    GetInstrumentDetails,
    MaskByBinRange,
    isEventWorkspace,
    getFilePathFromWorkspace,
    getWorkspaceReference,
    slice2histogram,
    getFileAndName,
    mask_detectors_with_masking_ws,
    check_child_ws_for_name_and_type_for_added_eventdata,
    extract_spectra,
    extract_child_ws_for_added_eventdata,
    load_monitors_for_multiperiod_event_data,
    MaskWithCylinder,
    get_masked_det_ids,
    get_masked_det_ids_from_mask_file,
    INCIDENT_MONITOR_TAG,
    can_load_as_event_workspace,
    is_convertible_to_float,
    correct_q_resolution_for_can,
    is_valid_user_file_extension,
    ADD_TAG,
)
import DarkRunCorrection as DarkCorr

import SANSUserFileParser as UserFileParser
from reducer_singleton import ReductionStep

sanslog = Logger("SANS")

DEBUG = False

# A global name for the Q Resolution workspace which lives longer than a reducer core
QRESOLUTION_WORKSPACE_NAME = "Q_Resolution_ISIS_SANS"
QRESOLUTION_MODERATOR_WORKSPACE_NAME = "Q_Resolution_MODERATOR_ISIS_SANS"


def _issueWarning(msg):
    """
    Prints a message to the log marked as warning
    @param msg: message to be issued
    """
    print(msg)
    sanslog.warning(msg)


def _issueInfo(msg):
    """
    Prints a message to the log
    @param msg: message to be issued
    """
    print(msg)
    sanslog.notice(msg)


def is_prompt_peak_instrument(reducer):
    if reducer.instrument.name() == "LOQ" or reducer.instrument.name() == "LARMOR":
        return True
    else:
        return False


def get_wavelength_min_and_max(reducer):
    return reducer.to_wavelen.wav_low, reducer.to_wavelen.wav_high


class LoadRun(object):
    UNSET_PERIOD = -1

    def __init__(self, run_spec=None, trans=False, reload=True, entry=UNSET_PERIOD):
        """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
        @param run_spec: the run number followed by dot and the extension
        @param trans: set to true if the file is from a transmission run (default: False)
        @param reload: if to reload the workspace if it is already present
        @param entry: the entry number of the run, useful for multi-period files (default: load the entire file)
        """
        super(LoadRun, self).__init__()
        self._data_file = run_spec
        self._is_trans = trans
        self._reload = reload
        # entry number of the run inside the run file that will be analysed, as requested by the caller
        self._period = int(entry)
        self._index_of_group = 0

        # set to the total number of periods in the file
        self.periods_in_file = None
        self.ext = ""
        self.shortrun_no = -1
        # the name of the loaded workspace in Mantid
        self._wksp_name = ""

    def curr_period(self):
        if self._period != self.UNSET_PERIOD:
            return self._period
        return self._index_of_group + 1

    def move2ws(self, index):
        if self.periods_in_file > 1:
            if index < self.periods_in_file:
                self._index_of_group = index
                return True
        else:
            return False

    def get_wksp_name(self):
        ref_ws = mtd[str(self._wksp_name)]
        if isinstance(ref_ws, WorkspaceGroup):
            return ref_ws[self._index_of_group].name()
        else:
            return self._wksp_name

    wksp_name = property(get_wksp_name, None, None, None)

    def _load_transmission(self, inst=None, is_can=False, extra_options=None):
        if extra_options is None:
            extra_options = dict()
        if ".raw" in self._data_file or ".RAW" in self._data_file:
            self._load(inst, is_can, extra_options)
            return

        # the intention of the code below is a good idea. Hence the reason why
        # I have left in the code but commented it out. As of this writing
        # LoadNexusMonitors throws an error if LoadNexusMonitors is a histogram
        # i.e. this algorithm only works for event files at present. The error
        # gets presented in red to the user and causes confusion. When either
        # LoadNexusMonitor can load histogram data as well or other equivalent
        # change the code below which is not commented out can be deleted and
        # the code commented out can be uncomment and modified as necessary

        self._load(inst, is_can, extra_options)

        workspace = self._get_workspace_name()
        if workspace in mtd:
            outWs = mtd[workspace]
            if isinstance(outWs, IEventWorkspace):
                if workspace + "_monitors" in mtd:
                    RenameWorkspace(InputWorkspace=workspace + "_monitors", OutputWorkspace=workspace)
                    self.periods_in_file = 1
                    self._wksp_name = workspace

                    # For sans, in transmission, we care only about the monitors. Hence,
                    # by trying to load only the monitors we speed up the reduction process.
                    # besides, we avoid loading events which is useless for transmission.
                    # it may fail, if the input file was not a nexus file, in this case,
                    # it pass the job to the default _load method.
                    # try:
                    # outWs = LoadNexusMonitors(self._data_file, OutputWorkspace=workspace)
                    # self.periods_in_file = 1
                    # self._wksp_name = workspace
                    # except:
                    # self._load(inst, is_can, extra_options)

    def _load(self, inst=None, is_can=False, extra_options=None):
        """
        Load a workspace and read the logs into the passed instrument reference
        @param inst: a reference to the current instrument
        @param iscan: set this to True for can runs
        @param extra_options: arguments to pass on to the Load Algorithm.
        @return: number of periods in the workspace
        """
        if extra_options is None:
            extra_options = dict()
        if self._period != self.UNSET_PERIOD:
            workspace = self._get_workspace_name(self._period)
            if not can_load_as_event_workspace(self._data_file):
                extra_options["EntryNumber"] = self._period
        else:
            workspace = self._get_workspace_name()

        extra_options["OutputWorkspace"] = workspace

        outWs = Load(self._data_file, **extra_options)

        appendix = "_monitors"

        # We need to check if we are dealing with a group workspace which is made up of added event data. Note that
        # we can also have a group workspace which is associated with period data, which don't want to deal with here.
        added_event_data_flag = False
        if isinstance(outWs, WorkspaceGroup) and check_child_ws_for_name_and_type_for_added_eventdata(outWs):
            extract_child_ws_for_added_eventdata(outWs, appendix)
            added_event_data_flag = True
            # Reload the outWs, it has changed from a group workspace to an event workspace
            outWs = mtd[workspace]

        monitor_ws_name = workspace + appendix

        # Handle simple EventWorkspace data
        if not added_event_data_flag:
            if isinstance(outWs, IEventWorkspace):
                try:
                    LoadNexusMonitors(self._data_file, OutputWorkspace=monitor_ws_name)
                except ValueError as details:
                    sanslog.warning(
                        "The file does not contain monitors. \n" + "The normalization might behave differently than you expect.\n"
                        " Further details: " + str(details) + "\n"
                    )
            else:
                if monitor_ws_name in mtd:
                    DeleteWorkspace(monitor_ws_name)

        # Handle Multi-period Event data
        if not added_event_data_flag:
            if isinstance(outWs, WorkspaceGroup) and len(outWs) > 0 and check_child_ws_for_name_and_type_for_added_eventdata(outWs):
                pass
            elif isinstance(outWs, WorkspaceGroup) and len(outWs) > 0 and isinstance(outWs[0], IEventWorkspace):
                load_monitors_for_multiperiod_event_data(workspace=outWs, data_file=self._data_file, monitor_appendix=appendix)

        loader_name = ""
        if isinstance(outWs, WorkspaceGroup):
            historyWs = outWs[0]
        else:
            historyWs = outWs
        try:
            last_algorithm = historyWs.getHistory().lastAlgorithm()
            loader_name = last_algorithm.getProperty("LoaderName").value
        except RuntimeError as details:
            sanslog.warning("Tried to get a loader name. But it seems that there is no loader name. Further info: " + str(details))

        if loader_name == "LoadRaw":
            self._loadSampleDetails(workspace)

        if self._period != self.UNSET_PERIOD and isinstance(outWs, WorkspaceGroup):
            outWs = mtd[self._leaveSinglePeriod(outWs.name(), self._period, appendix)]

        self.periods_in_file = self._find_workspace_num_periods(workspace)

        self._wksp_name = workspace

    def _get_workspace_name(self, entry_num=None):
        """
        Creates a name for the workspace that will contain the raw
        data. If the entry number == 1 it is omitted, unless
        optional_entry_no = False
        @param entry_num: if this argument is set to an integer it will be added to the filename after a p
        """
        run = str(self.shortrun_no)
        if entry_num:
            if entry_num == self.UNSET_PERIOD:
                entry_num = 1
            run += "p" + str(int(entry_num))

        if self._is_trans:
            return run + "_trans_" + self.ext.lower()
        else:
            return run + "_sans_" + self.ext.lower()

    def _loadSampleDetails(self, ws_name):
        ws_pointer = mtd[str(ws_name)]
        if isinstance(ws_pointer, WorkspaceGroup):
            workspaces = [ws for ws in ws_pointer]
        else:
            workspaces = [ws_pointer]
        for ws in workspaces:
            LoadSampleDetailsFromRaw(ws, self._data_file)

    def _loadFromWorkspace(self, reducer):
        """It substitute the work of _assignHelper for workspaces, or, at least,
        prepare the internal attributes, to be processed by the _assignHelper.

        It is executed when the input for the constructor (run_spec) is given a workspace

        If reload is False, it will try to get all information necessary to use the given
        workspace as the one for the post-processing.
        If reload is True, it will try to get all the information necessary to reload this
        workspace from the data file.
        """
        assert isinstance(self._data_file, Workspace)
        ws_pointer = self._data_file

        try:
            _file_path = getFilePathFromWorkspace(ws_pointer)
        except:
            raise RuntimeError("Failed to retrieve information to reload this workspace " + str(self._data_file))
        self._data_file = _file_path
        self.ext = _file_path[-3:]
        if isinstance(ws_pointer, WorkspaceGroup):
            self.shortrun_no = ws_pointer[0].getRunNumber()
        else:
            self.shortrun_no = ws_pointer.getRunNumber()

        if self._reload:
            # give to _assignHelper the responsibility of loading this data.
            return False

        # test if the sample details are already loaded, necessary only for raw files:
        if ".nxs" not in self._data_file[-4:]:
            self._loadSampleDetails(ws_pointer)

        # so, it will try, not to reload the workspace.
        self._wksp_name = ws_pointer.name()
        self.periods_in_file = self._find_workspace_num_periods(self._wksp_name)

        # check that the current workspace has never been moved
        hist_str = self._getHistory(ws_pointer)
        if "Algorithm: Move" in hist_str or "Algorithm: Rotate" in hist_str:
            raise RuntimeError("Moving components needs to be made compatible with not reloading the sample")

        return True

    # Helper function
    def _assignHelper(self, reducer):
        if isinstance(self._data_file, Workspace):
            loaded_flag = self._loadFromWorkspace(reducer)
            if loaded_flag:
                return

        if self._data_file == "" or self._data_file.startswith("."):
            raise RuntimeError("Sample needs to be assigned as run_number.file_type")

        try:
            if reducer.instrument.name() == "":
                raise AttributeError
        except AttributeError:
            raise AttributeError("No instrument has been assign, run SANS2D or LOQ first")

        self._data_file = self._extract_run_details(self._data_file)

        if not self._reload:
            raise NotImplementedError("Raw workspaces must be reloaded, run with reload=True")

        spectrum_limits = dict()
        if self._is_trans:
            if reducer.instrument.name() == "SANS2D" and int(self.shortrun_no) < 568:
                dimension = GetInstrumentDetails(reducer.instrument)[0]
                spec_min = dimension * dimension * 2
                spectrum_limits = {"SpectrumMin": spec_min, "SpectrumMax": spec_min + 4}

        try:
            if self._is_trans and reducer.instrument.name() != "LOQ":
                # Unfortunately, LOQ in transmission acquire 3 monitors the 3 monitor usually
                # is the first spectrum for detector. This causes the following method to fail
                # when it tries to load only monitors. Hence, we are forced to skip this method
                # for LOQ. ticket #8559
                self._load_transmission(reducer.instrument, extra_options=spectrum_limits)
            else:
                # the spectrum_limits is not the default only for transmission data
                self._load(reducer.instrument, extra_options=spectrum_limits)
        except RuntimeError as details:
            sanslog.warning(str(details))
            self._wksp_name = ""
            return

        return

    def _leaveSinglePeriod(self, workspace, period, appendix):
        groupW = mtd[workspace]
        if not isinstance(groupW, WorkspaceGroup):
            logger.warning("Invalid request for getting single period in a non group workspace")
            return workspace
        if len(groupW) < period:
            raise ValueError("Period number " + str(period) + " doesn't exist in workspace " + groupW.name())
        ws_name = groupW[period - 1].name()

        # If we are dealing with event data, then we also want to extract and rename the according monitor data set
        monitor_name = ""
        if isEventWorkspace(groupW[period - 1]):
            # Check if the monitor ws exists and extract it
            expected_mon_name = ws_name + appendix
            expected_mon_group_name = groupW.name() + appendix
            if mtd.doesExist(expected_mon_name):
                monitor_name = expected_mon_name
            if mtd.doesExist(expected_mon_group_name):
                group_mon_ws = mtd[expected_mon_group_name]
                group_mon_ws.remove(expected_mon_name)
                DeleteWorkspace(expected_mon_group_name)

        # remove this workspace from the group
        groupW.remove(ws_name)
        # remove the entire group
        DeleteWorkspace(groupW)

        new_name = self._get_workspace_name(period)
        new_mon_name = new_name + appendix
        if new_name != ws_name:
            RenameWorkspace(ws_name, OutputWorkspace=new_name)
        if monitor_name != "" and new_mon_name != monitor_name:
            RenameWorkspace(monitor_name, OutputWorkspace=new_mon_name)
        return new_name

    def _extract_run_details(self, run_string):
        """
        Takes a run number and file type and generates the filename, workspace name and log name
        @param run_string: either the name of a run file or a run number followed by a dot and then the file type, i.e. file extension
        """
        listOfFiles = FileFinder.findRuns(run_string)
        firstFile = listOfFiles[0]
        self.ext = firstFile[-3:]
        self.shortrun_no = int(re.findall(r"\d+", run_string)[-1])
        return firstFile

    def _find_workspace_num_periods(self, workspace):
        """
        @param workspace: the name of the workspace
        """
        numPeriods = -1
        pWorksp = mtd[workspace]
        if isinstance(pWorksp, WorkspaceGroup):
            # get the number of periods in a group using the fact that each period has a different name
            numPeriods = len(pWorksp)
        else:
            numPeriods = 1
        return numPeriods

    def _getHistory(self, wk_name):
        getWorkspaceReference(wk_name)

        if isinstance(wk_name, Workspace):
            ws_h = wk_name.getHistory()
        else:
            if wk_name not in mtd:
                return ""
            ws_h = mtd[wk_name].getHistory()
        hist_str = str(ws_h)

        return hist_str

    def getCorrospondingPeriod(self, sample_period, reducer):
        """
        Gets the period number that corresponds to the passed sample period number, based on:
        if the workspace has the same number of periods as the sample it gives returns requested
        period, if it contains only one period it returns 1 and everything else is an error
        @param sample_period: the period in the sample that is of interest
        @return: depends on the number of entries in the workspace, could be the same number as passed or 1
        @raise RuntimeError: if there is ambiguity
        """
        if self.periods_in_file == 1:
            # this is a single entry file, don't consider entries
            return 1
        elif self._period != self.UNSET_PERIOD:
            # the user specified a definite period, use it
            return self._period
        elif self.periods_in_file == reducer.get_sample().loader.periods_in_file:
            # use corresponding periods, the same entry as the sample in each case
            return sample_period
        else:
            raise RuntimeError("There is a mismatch in the number of periods (entries) in the file between the sample and another run")


class LoadTransmissions(object):
    """
    Loads the file used to apply the transmission correction to the
    sample or can
    """

    _direct_name = None
    _trans_name = None

    def __init__(self, is_can=False, reload=True):
        """
        Two settings can be set at initialization, if this is for
        can and if the workspaces should be reloaded if they already
        exist
        @param is_can: if this is to correct the can (default false i.e. it's for the sample)
        @param reload: setting this to false will mean the workspaces aren't reloaded if they already exist (default True i.e. reload)
        """
        self.trans = None
        self.direct = None
        self._reload = reload
        self._period_t = -1
        self._period_d = -1
        self.can = is_can

    def set_trans(self, trans, period=-1):
        self._trans_name = trans
        self._period_t = period

    def set_direc(self, direct, period=-1):
        self._direct_name = direct
        self._period_d = period

    def execute(self, reducer, workspace):
        if self._trans_name not in [None, ""]:
            self.trans = LoadRun(self._trans_name, trans=True, reload=self._reload, entry=self._period_t)
            self.trans._assignHelper(reducer)
            if isinstance(self._trans_name, Workspace):
                self._trans_name = self._trans_name.name()
            if not self.trans.wksp_name:
                # do nothing if no workspace was specified
                return "", ""

        if self._direct_name not in [None, ""]:
            self.direct = LoadRun(self._direct_name, trans=True, reload=self._reload, entry=self._period_d)
            self.direct._assignHelper(reducer)
            if isinstance(self._direct_name, Workspace):
                self._direct_name = self._direct_name.name()
            if not self.direct.wksp_name:
                raise RuntimeError("Transmission run set without direct run error")

        # transmission workspaces sometimes have monitor locations, depending on the instrument, load these locations
        reducer.instrument.load_transmission_inst(self.trans.wksp_name, self.direct.wksp_name, reducer.get_beam_center())

        return self.trans.wksp_name, self.direct.wksp_name


class CanSubtraction(ReductionStep):
    """
    Apply the same corrections to the can that were applied to the sample and
    then subtracts this can from the sample.
    """

    def __init__(self):
        super(CanSubtraction, self).__init__()

    def execute(self, reducer, workspace):
        """
        Apply same corrections as for sample workspace then subtract from data
        """
        if reducer.get_can() is None:
            return

        # rename the sample workspace, its name will be restored to the original once the subtraction has been done
        tmp_smp = workspace + "_sam_tmp"
        RenameWorkspace(InputWorkspace=workspace, OutputWorkspace=tmp_smp)

        tmp_can = workspace + "_can_tmp"
        # do same corrections as were done to the sample
        reducer.reduce_can(tmp_can)

        # we now have the can workspace, use it
        Minus(LHSWorkspace=tmp_smp, RHSWorkspace=tmp_can, OutputWorkspace=workspace)
        # Correct the Q resolution entries in the output workspace
        correct_q_resolution_for_can(mtd[tmp_smp], mtd[tmp_can], mtd[workspace])

        # clean up the workspaces ready users to see them if required
        if reducer.to_Q.output_type == "1D":
            StripEndNans()

        self._keep_partial_results(tmp_smp, tmp_can)

    def get_wksp_name(self):
        return self.workspace.wksp_name

    wksp_name = property(get_wksp_name, None, None, None)

    def get_periods_in_file(self):
        return self.workspace.periods_in_file

    def _keep_partial_results(self, sample_name, can_name):
        # user asked to keep these results 8970
        gp_name = "sample_can_reductions"
        if mtd.doesExist(gp_name):
            gpr = mtd[gp_name]
            for wsname in [sample_name, can_name]:
                if not gpr.contains(wsname):
                    gpr.add(wsname)
        else:
            GroupWorkspaces([sample_name, can_name], OutputWorkspace=gp_name)

    periods_in_file = property(get_periods_in_file, None, None, None)

    def _pass_dx_values_to_can_subtracted_if_required(self, original_ws, subtracted_ws):
        """
        We pass the DX values from the original workspace to the subtracted workspace.
        This means we currently do nothing with potential DX values in the can workspace.
        Also that if there are DX values, then they are in all spectra
        """
        if not original_ws.hasDx(0):
            return
        for index in range(0, original_ws.getNumHistograms()):
            subtracted_ws.setDx(index, original_ws.dataDX(index))


class Mask_ISIS(ReductionStep):
    """
    Marks some spectra so that they are not included in the analysis
    Provides ISIS specific mask functionality (e.g. parsing
    MASK commands from user files), inherits from Mask
    """

    def __init__(self, timemask="", timemask_r="", timemask_f="", specmask="", specmask_r="", specmask_f=""):
        self._xml = []

        # these spectra will be masked by the algorithm MaskDetectors
        self.detect_list = []

        # List of pixels to mask
        self.masked_pixels = []

        self.time_mask = timemask
        self.time_mask_r = timemask_r
        self.time_mask_f = timemask_f
        self.spec_mask_r = specmask_r
        self.spec_mask_f = specmask_f

        # as far as I can used to possibly set phi masking
        # not to be applied even though _lim_phi_xml has been set
        self.mask_phi = True
        self.phi_mirror = True
        self._lim_phi_xml = ""
        self.phi_min = -90.0
        self.phi_max = 90.0
        # read only phi (only used in ...)
        # this option seems totally bizarre to me since it allow
        # set_phi_limit to be called but not setting the _lim_phi_xml
        # string.....
        self._readonly_phi = False
        # used to assess if set phi limit has been called just once
        # in which case exactly one phi range has been masked
        # and get_phi_limits
        self._numberOfTimesSetPhiLimitBeenCalled = 0
        self.spec_list = []

        # is set when there is an arm to mask, it's the width in millimetres
        self.arm_width = None
        # when there is an arm to mask this is its angle in degrees
        self.arm_angle = None
        # RMD Mod 24/7/13
        self.arm_x = None
        self.arm_y = None

        ########################## Masking  ################################################
        # Mask the corners and beam stop if radius parameters are given

        self.min_radius = None
        self.max_radius = None

    def add_xml_shape(self, complete_xml_element):
        """
        Add an arbitrary shape to region to be masked
        @param complete_xml_element: description of the shape to add
        """
        if not complete_xml_element.startswith("<"):
            raise ValueError("Excepted xml string but found: " + str(complete_xml_element))
        self._xml.append(complete_xml_element)

    def _infinite_plane(self, id, plane_pt, normal_pt, complement=False):
        """
        Generates xml code for an infinite plane
        @param id: a string to refer to the shape by
        @param plane_pt: a point in the plane
        @param normal_pt: the direction of a normal to the plane
        @param complement: mask in the direction of the normal or away
        @return the xml string
        """
        return (
            '<infinite-plane id="'
            + str(id)
            + '">'
            + '<point-in-plane x="'
            + str(plane_pt[0])
            + '" y="'
            + str(plane_pt[1])
            + '" z="'
            + str(plane_pt[2])
            + '" />'
            + '<normal-to-plane x="'
            + str(normal_pt[0])
            + '" y="'
            + str(normal_pt[1])
            + '" z="'
            + str(normal_pt[2])
            + '" />'
            + "</infinite-plane>\n"
        )

    def _infinite_cylinder(self, centre, radius, axis, id="shape"):
        """
        Generates xml code for an infintely long cylinder
        @param centre: a tuple for a point on the axis
        @param radius: cylinder radius
        @param axis: cylinder orientation
        @param id: a string to refer to the shape by
        @return the xml string
        """
        return (
            '<infinite-cylinder id="'
            + str(id)
            + '">'
            + '<centre x="'
            + str(centre[0])
            + '" y="'
            + str(centre[1])
            + '" z="'
            + str(centre[2])
            + '" />'
            + '<axis x="'
            + str(axis[0])
            + '" y="'
            + str(axis[1])
            + '" z="'
            + str(axis[2])
            + '" />'
            + '<radius val="'
            + str(radius)
            + '" /></infinite-cylinder>\n'
        )

    def _finite_cylinder(self, centre, radius, height, axis, id="shape"):
        """
        Generates xml code for an infintely long cylinder
        @param centre: a tuple for a point on the axis
        @param radius: cylinder radius
        @param height: cylinder height
        @param axis: cylinder orientation
        @param id: a string to refer to the shape by
        @return the xml string
        """
        return (
            '<cylinder id="'
            + str(id)
            + '">'
            + '<centre-of-bottom-base x="'
            + str(centre[0])
            + '" y="'
            + str(centre[1])
            + '" z="'
            + str(centre[2])
            + '" />'
            + '<axis x="'
            + str(axis[0])
            + '" y="'
            + str(axis[1])
            + '" z="'
            + str(axis[2])
            + '" />'
            + '<radius val="'
            + str(radius)
            + '" /><height val="'
            + str(height)
            + '" /></cylinder>\n'
        )

    def add_cylinder(self, radius, xcentre, ycentre, ID="shape"):
        """Mask the inside of an infinite cylinder on the input workspace."""
        self.add_xml_shape(self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0, 0, 1], id=ID) + '<algebra val="' + str(ID) + '"/>')

    def add_outside_cylinder(self, radius, xcentre=0.0, ycentre=0.0, ID="shape"):
        """Mask out the outside of a cylinder or specified radius"""
        self.add_xml_shape(self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0, 0, 1], id=ID) + '<algebra val="#' + str(ID) + '"/>')

    def set_radi(self, _min, _max):
        self.min_radius = float(_min) / 1000.0
        self.max_radius = float(_max) / 1000.0

    def _whichBank(self, instName, specNo):
        """
        Return either 'rear' or 'front' depending on which bank the spectrum number belong to

        @param instName Instrument name. Used for MASK Ssp command to tell what bank it refer to
        @param specNo Spectrum number
        """
        bank = "rear"

        if instName.upper() == "LOQ":
            if 16387 <= specNo <= 17784:
                bank = "front"
        if instName.upper() == "SANS2D":
            if 36873 <= specNo <= 73736:
                bank = "front"

        return bank

    def parse_instruction(self, instName, details):  # noqa: C901
        """
        Parse an instruction line from an ISIS mask file
        @param instName Instrument name. Used for MASK Ssp command to tell what bank it refer to
        @param details Line to parse
        """
        details = details.lstrip()
        details = details.upper()
        if not details.startswith("MASK") and not details.startswith("L/PHI"):
            _issueWarning("Ignoring malformed mask line " + details)
            return

        if "L/PHI" in details:
            phiParts = details.split()
            if len(phiParts) == 3:
                mirror = phiParts[0] != "L/PHI/NOMIRROR"
                phiMin = phiParts[1]
                phiMax = phiParts[2]
                self.set_phi_limit(float(phiMin), float(phiMax), mirror)
                return
            else:
                _issueWarning('Unrecognized L/PHI masking line command "' + details + '"')
                return

        parts = details.split("/")
        # A spectrum mask or mask spectra range with H and V commands
        if len(parts) == 1:  # Command is to type MASK something
            argToMask = details[4:].lstrip().upper()
            bank = "rear"
            # special case for MASK Ssp where try to infer the bank the spectrum number belong to
            if "S" in argToMask:
                if ">" in argToMask:
                    pieces = argToMask.split(">")
                    low = int(pieces[0].lstrip("S"))
                    upp = int(pieces[1].lstrip("S"))
                    bankLow = self._whichBank(instName, low)
                    bankUpp = self._whichBank(instName, upp)
                    if bankLow != bankUpp:
                        _issueWarning(
                            "The spectra in Mask command: " + details + " belong to two different banks. Default to use bank " + bankLow
                        )
                    bank = bankLow
                else:
                    bank = self._whichBank(instName, int(argToMask.lstrip("S")))

            # Default to the rear detector if not MASK Ssp command
            self.add_mask_string(argToMask, detect=bank)
        elif len(parts) == 2:  # Command is to type MASK/ something
            _type = parts[1]  # this is the part of the command following /
            typeSplit = _type.split()  # used for command such as MASK/REAR Hn and MASK/Line w a
            if _type == "CLEAR":  # Command is specifically MASK/CLEAR
                self.spec_mask_r = ""
                self.spec_mask_f = ""
            elif _type.startswith("T"):
                if _type.startswith("TIME"):
                    bin_range = _type[4:].lstrip()
                else:
                    bin_range = _type[1:].lstrip()
                self.time_mask += ";" + bin_range
            elif len(typeSplit) == 2:
                # Commands such as MASK/REAR Hn, where typeSplit[0] then equal 'REAR'
                if "S" in typeSplit[1].upper():
                    _issueWarning("MASK command of type " + details + " deprecated. Please use instead MASK Ssp1[>Ssp2]")
                if typeSplit[0].upper() != "REAR" and instName == "LOQ":
                    _issueWarning(
                        "MASK command of type " + details + " can, until otherwise requested, only be used for the REAR (default)"
                        " Main detector of LOQ. " + "Default to the Main detector of LOQ for this mask command"
                    )
                    self.add_mask_string(mask_string=typeSplit[1], detect="rear")
                else:
                    self.add_mask_string(mask_string=typeSplit[1], detect=typeSplit[0])
            elif _type.startswith("LINE"):
                # RMD mod 24/7/13
                if len(typeSplit) == 5:
                    self.arm_width = float(typeSplit[1])
                    self.arm_angle = float(typeSplit[2])
                    self.arm_x = float(typeSplit[3])
                    self.arm_y = float(typeSplit[4])
                elif len(typeSplit) == 3:
                    self.arm_width = float(typeSplit[1])
                    self.arm_angle = float(typeSplit[2])
                    self.arm_x = 0.0
                    self.arm_y = 0.0
                else:
                    _issueWarning(
                        'Unrecognized line masking command "' + details + '" syntax is MASK/LINE width angle or MASK/LINE width angle x y'
                    )
            else:
                _issueWarning('Unrecognized masking option "' + details + '"')
        elif len(parts) == 3:
            _type = parts[1]
            if _type == "CLEAR":
                self.time_mask = ""
                self.time_mask_r = ""
                self.time_mask_f = ""
            elif _type == "TIME" or _type == "T":
                parts = parts[2].split()
                if len(parts) == 3:
                    detname = parts[0].rstrip()
                    bin_range = parts[1].rstrip() + " " + parts[2].lstrip()
                    if detname.upper() == "FRONT":
                        self.time_mask_f += ";" + bin_range
                    elif detname.upper() == "REAR":
                        self.time_mask_r += ";" + bin_range
                    else:
                        _issueWarning(
                            "Detector '"
                            + detname
                            + "' not found in currently selected instrument "
                            + self.instrument.name()
                            + ". Skipping line."
                        )
                else:
                    _issueWarning('Unrecognized masking line "' + details + '"')
        else:
            _issueWarning('Unrecognized masking line "' + details + '"')

    def add_mask_string(self, mask_string, detect):
        if detect.upper() == "FRONT" or detect.upper() == "HAB":
            self.spec_mask_f += "," + mask_string
        elif detect.upper() == "REAR":
            self.spec_mask_r += "," + mask_string
        else:
            _issueWarning(
                "Detector '" + detect + "' not found in currently selected instrument " + self.instrument.name() + ". Skipping line."
            )

    def _ConvertToSpecList(self, maskstring, detector):
        """
        Convert a mask string to a spectra list
        6/8/9 RKH attempt to add a box mask e.g.  h12+v34 (= one pixel at intersection), h10>h12+v101>v123 (=block 3 wide, 23 tall)

        @param maskstring Is a comma separated list of mask commands for masking spectra using the e.g. the h, s and v commands
        """
        # Compile spectra ID list
        if maskstring == "":
            return ""
        masklist = maskstring.split(",")

        speclist = ""
        for x in masklist:
            x = x.lower()
            if "+" in x:
                bigPieces = x.split("+")
                if ">" in bigPieces[0]:
                    pieces = bigPieces[0].split(">")
                    low = int(pieces[0].lstrip("hv"))
                    upp = int(pieces[1].lstrip("hv"))
                else:
                    low = int(bigPieces[0].lstrip("hv"))
                    upp = low
                if ">" in bigPieces[1]:
                    pieces = bigPieces[1].split(">")
                    low2 = int(pieces[0].lstrip("hv"))
                    upp2 = int(pieces[1].lstrip("hv"))
                else:
                    low2 = int(bigPieces[1].lstrip("hv"))
                    upp2 = low2
                if "h" in bigPieces[0] and "v" in bigPieces[1]:
                    ydim = abs(upp - low) + 1
                    xdim = abs(upp2 - low2) + 1
                    speclist += detector.spectrum_block(low, low2, ydim, xdim) + ","
                elif "v" in bigPieces[0] and "h" in bigPieces[1]:
                    xdim = abs(upp - low) + 1
                    ydim = abs(upp2 - low2) + 1
                    speclist += detector.spectrum_block(low2, low, ydim, xdim) + ","
                else:
                    print("error in mask, ignored:  " + x)
            elif ">" in x:  # Commands: MASK Ssp1>Ssp2, MASK Hn1>Hn2 and MASK Vn1>Vn2
                pieces = x.split(">")
                low = int(pieces[0].lstrip("hvs"))
                upp = int(pieces[1].lstrip("hvs"))
                if "h" in pieces[0]:
                    nstrips = abs(upp - low) + 1
                    speclist += detector.spectrum_block(low, 0, nstrips, "all") + ","
                elif "v" in pieces[0]:
                    nstrips = abs(upp - low) + 1
                    speclist += detector.spectrum_block(0, low, "all", nstrips) + ","
                else:
                    for i in range(low, upp + 1):
                        speclist += str(i) + ","
            elif "h" in x:
                speclist += detector.spectrum_block(int(x.lstrip("h")), 0, 1, "all") + ","
            elif "v" in x:
                speclist += detector.spectrum_block(0, int(x.lstrip("v")), "all", 1) + ","
            elif "s" in x:  # Command MASK Ssp. Although note commands of type MASK Ssp1>Ssp2 handled above
                speclist += x.lstrip("s") + ","
            elif x == "":
                # empty entries are allowed
                pass
            elif len(x.split()) == 4:
                _issueWarning('Box mask entry "%s" ignored. Box masking is not supported by Mantid' % ("mask " + x))
            else:
                raise SyntaxError('Problem reading a mask entry: "%s"' % x)

        # remove any trailing comma
        if speclist.endswith(","):
            speclist = speclist[0 : len(speclist) - 1]

        return speclist

    def _mask_phi(self, id, centre, phimin, phimax, use_mirror=True):
        """
        Mask the detector bank such that only the region specified in the
        phi range is left unmasked
        Purpose of this method is to populate self._lim_phi_xml
        """
        # convert all angles to be between 0 and 360
        while phimax > 360:
            phimax -= 360
        while phimax < 0:
            phimax += 360
        while phimin > 360:
            phimin -= 360
        while phimin < 0:
            phimin += 360
        while phimax < phimin:
            phimax += 360

        # Convert to radians
        phimin = math.pi * phimin / 180.0
        phimax = math.pi * phimax / 180.0

        id = str(id)
        self._lim_phi_xml = self._infinite_plane(
            id + "_plane1", centre, [math.cos(-phimin + math.pi / 2.0), math.sin(-phimin + math.pi / 2.0), 0]
        ) + self._infinite_plane(id + "_plane2", centre, [-math.cos(-phimax + math.pi / 2.0), -math.sin(-phimax + math.pi / 2.0), 0])

        if use_mirror:
            self._lim_phi_xml += (
                self._infinite_plane(id + "_plane3", centre, [math.cos(-phimax + math.pi / 2.0), math.sin(-phimax + math.pi / 2.0), 0])
                + self._infinite_plane(id + "_plane4", centre, [-math.cos(-phimin + math.pi / 2.0), -math.sin(-phimin + math.pi / 2.0), 0])
                + '<algebra val="#(('
                + id
                + "_plane1 "
                + id
                + "_plane2):("
                + id
                + "_plane3 "
                + id
                + '_plane4))" />'
            )
        else:
            # the formula is different for acute verses obtuse angles
            if phimax - phimin > math.pi:
                # to get an obtruse angle, a wedge that's more than half the area, we need to add the semi-inifinite volumes
                self._lim_phi_xml += '<algebra val="#(' + id + "_plane1:" + id + '_plane2)" />'
            else:
                # an acute angle, wedge is more less half the area, we need to use the intesection of those semi-inifinite volumes
                self._lim_phi_xml += '<algebra val="#(' + id + "_plane1 " + id + '_plane2)" />'

    def _mask_line(self, startPoint, length, width, angle):
        """
        Creates the xml to mask a line of the given width and height at the given angle
        into the member _line_xml. The masking object which is used to mask a line of say
        a detector array is a finite cylinder
        @param startPoint: startPoint of line
        @param length: length of line
        @param width: width of line in mm
        @param angle: angle of line in xy-plane in units of degrees
        @return: return xml shape string
        """
        return self._finite_cylinder(
            startPoint, width / 2000.0, length, [math.cos(angle * math.pi / 180.0), math.sin(angle * math.pi / 180.0), 0.0], "arm"
        )

    def get_phi_limits_tag(self):
        """
        Get the values of the lowest and highest boundaries
        Used to append to output workspace name
        @return 'Phi'low'_'high if it has been set
        """
        if self.mask_phi and self._lim_phi_xml != "" and (abs(self.phi_max - self.phi_min) != 180.0):
            return "Phi" + str(self.phi_min) + "_" + str(self.phi_max)
        else:
            return ""

    def set_phi_limit(self, phimin, phimax, phimirror, override=True):
        """
        ... (tx to Richard for changes to this function
             for ticket #)
        @param phimin:
        @param phimax:
        @param phimirror:
        @param override: This one I don't understand. It seem
           dangerous to be allowed to set this one to false.
           Also this option cannot be set from the command interface
        @return: return xml shape string
        """
        if phimirror:
            if phimin > phimax:
                phimin, phimax = phimax, phimin

            if phimax - phimin == 180.0:
                self.phi_min = -90.0
                self.phi_max = 90.0
            else:
                self.phi_min = phimin
                self.phi_max = phimax
        else:
            self.phi_min = phimin
            self.phi_max = phimax

        self.phi_mirror = phimirror

        if override:
            self._readonly_phi = True

        if (not self._readonly_phi) or override:
            self._mask_phi("unique phi", [0, 0, 0], self.phi_min, self.phi_max, self.phi_mirror)

    def execute(self, reducer, workspace):
        instrument = reducer.instrument
        # set up the spectra lists and shape xml to mask
        detector = instrument.cur_detector()
        if detector.isAlias("rear"):
            self.spec_list = self._ConvertToSpecList(self.spec_mask_r, detector)
            # Time mask
            MaskByBinRange(workspace, self.time_mask_r)
            MaskByBinRange(workspace, self.time_mask)

        if detector.isAlias("front"):
            # front specific masking
            self.spec_list = self._ConvertToSpecList(self.spec_mask_f, detector)
            # Time mask
            MaskByBinRange(workspace, self.time_mask_f)
            MaskByBinRange(workspace, self.time_mask)

        # reset the xml, as execute can be run more than once
        self._xml = []

        if (self.min_radius is not None) and (self.min_radius > 0.0):
            self.add_cylinder(self.min_radius, 0, 0, "beam_stop")
        if (self.max_radius is not None) and (self.max_radius > 0.0):
            self.add_outside_cylinder(self.max_radius, 0, 0, "beam_area")
        # now do the masking
        for shape in self._xml:
            MaskDetectorsInShape(Workspace=workspace, ShapeXML=shape)

        if "MaskFiles" in reducer.settings:
            for mask_file in reducer.settings["MaskFiles"].split(","):
                try:
                    mask_file_path, mask_ws_name = getFileAndName(mask_file)
                    mask_ws_name = "__" + mask_ws_name
                    LoadMask(Instrument=instrument.idf_path, InputFile=mask_file_path, OutputWorkspace=mask_ws_name)
                    mask_detectors_with_masking_ws(workspace, mask_ws_name)
                    DeleteWorkspace(Workspace=mask_ws_name)
                except:
                    raise RuntimeError("Invalid input for mask file. (%s)" % mask_file)

        if len(self.spec_list) > 0:
            MaskDetectors(Workspace=workspace, SpectraList=self.spec_list, ForceInstrumentMasking=True)

        if self._lim_phi_xml != "" and self.mask_phi:
            MaskDetectorsInShape(Workspace=workspace, ShapeXML=self._lim_phi_xml)

        if self.arm_width and self.arm_angle:
            # Currently SANS2D and LOQ are supported
            instrument_name = instrument.name()
            if instrument_name == "SANS2D" or instrument_name == "LOQ":
                component_name = "rear-detector" if instrument_name == "SANS2D" else "main-detector-bank"
                ws = mtd[str(workspace)]
                det = ws.getInstrument().getComponentByName(component_name)
                det_Z = det.getPos().getZ()
                start_point = [self.arm_x, self.arm_y, det_Z]
                MaskDetectorsInShape(Workspace=workspace, ShapeXML=self._mask_line(start_point, 100.0, self.arm_width, self.arm_angle))

        _output_ws, detector_list = ExtractMask(InputWorkspace=workspace, OutputWorkspace="__mask")
        _issueInfo("Mask check %s: %g masked pixels" % (workspace, len(detector_list)))

    def view(self, instrum):
        """
        In MantidPlot this opens InstrumentView to display the masked
        detectors in the bank in a different colour
        @param instrum: a reference an instrument object to view
        """
        wksp_name = "CurrentMask"
        instrum.load_empty(wksp_name)

        # apply masking to the current detector
        self.execute(None, wksp_name)

        # now the other detector
        other = instrum.other_detector().name()
        original = instrum.cur_detector().name()
        instrum.setDetector(other)
        self.execute(None, wksp_name)
        # reset the instrument to mask the currecnt detector
        instrum.setDetector(original)

        # Mark up "dead" detectors with error value
        FindDeadDetectors(InputWorkspace=wksp_name, OutputWorkspace=wksp_name, DeadValue=500)

        # opens an instrument showing the contents of the workspace (i.e. the instrument with masked detectors)
        instrum.view(wksp_name)

    def display(self, wksp, reducer):
        """
        Mask detectors in a workspace and display its show instrument
        @param wksp: this named workspace will be masked and displayed
        @param reducer: the reduction chain that contains all the settings
        """
        # apply masking to the current detector
        self.execute(reducer, wksp)

        instrum = reducer.instrument
        # now the other detector
        other = instrum.other_detector().name()
        original = instrum.cur_detector().name()
        instrum.setDetector(other)
        self.execute(reducer, wksp)
        # reset the instrument to mask the current detector
        instrum.setDetector(original)

        # opens an instrument showing the contents of the workspace (i.e. the instrument with masked detectors)
        instrum.view(wksp)

    def __str__(self):
        return (
            "    radius",
            self.min_radius,
            self.max_radius + "\n" + "    rear spectrum mask: ",
            str(self.spec_mask_r) + "\n" + "    front spectrum mask: ",
            str(self.spec_mask_f) + "\n" + "    global time mask: ",
            str(self.time_mask) + "\n" + "    rear time mask: ",
            str(self.time_mask_r) + "\n" + "    front time mask: ",
            str(self.time_mask_f) + "\n",
        )


class LoadSample(LoadRun):
    """
    Handles loading the sample run, this is the main experimental run with data
    about the sample of interest
    """

    def __init__(self, sample=None, reload=True, entry=-1):
        LoadRun.__init__(self, sample, reload=reload, entry=entry)
        self._scatter_sample = None
        self._SAMPLE_RUN = None

        self.maskpt_rmin = None
        # is set to the entry (period) number in the sample to be run
        self.entries = []

    def execute(self, reducer, isSample):
        self._assignHelper(reducer)

        if self.wksp_name == "":
            raise RuntimeError("Unable to load SANS sample run, cannot continue.")

        if self.periods_in_file > 1:
            self.entries = list(range(0, self.periods_in_file))

        # applies on_load_sample for all the workspaces (single or groupworkspace)
        num = 0
        while True:
            if reducer.instrument.name() == "LOQ":
                reducer.instrument.on_load_sample(
                    self.wksp_name,
                    reducer.get_beam_center(),
                    isSample,
                    other_centre=reducer.get_beam_center(reducer.instrument.other_detector().name()),
                )
            else:
                reducer.instrument.on_load_sample(self.wksp_name, reducer.get_beam_center(), isSample)
            reducer.update_beam_center()
            num += 1
            if num == self.periods_in_file:
                break
            self.move2ws(num)
        self.move2ws(0)


class DarkRunSubtraction(object):
    """
    This class handles the subtraction of a dark run from the sample workspace.
    The dark run subtraction does not take place and just passes the workspace through
    if the parameters are not fully specified. This layer knows about some details of the
    ISIS process, eg it knows about the format of Histogram or Event mode reduction and the
    shape of added event files. It does not depend on the reducer itself though!
    """

    # The named tuple contains the information for a dark run subtraction for a single run number ( of
    # a dark run file)
    # The relevant information is the run number, if we use time or uamp, if we use mean or tof, if we
    # apply this to all detectors, if we apply this to monitors and if so to which monitors
    DarkRunSubtractionSettings = namedtuple("DarkRunSettings", "run_number time mean detector mon mon_numbers")

    def __init__(self):
        super(DarkRunSubtraction, self).__init__()
        # This is a list with settings for the dark run subtraction
        # Each element in the list will be struct-like and contain
        # the relevant information for a dark-run subtraction
        self._dark_run_settings = []

        # We have four types of settings: uamp + det, uamp + mon, time + det, time + mon.
        # Richard suggested to limit this for now to these four settings.
        self._dark_run_time_detector_setting = None
        self._dark_run_uamp_detector_setting = None
        self._dark_run_time_monitor_setting = None
        self._dark_run_uamp_monitor_setting = None

        # Keeps a hold on the number of histograms in the monitor workspace associated
        # with the scatter workspace

        self._monitor_list = None

    def add_setting(self, dark_run_setting):
        """
        We add a dark run setting to our list of settings
        @param dark_run_setting
        """
        if not isinstance(dark_run_setting, UserFileParser.DarkRunSettings):
            raise RuntimeError("DarkRunSubtraction: The provided settings " "object is not of type DarkRunSettings")

        # We only add entries where the run number has been specified
        if not dark_run_setting.run_number:
            return
        self._dark_run_settings.append(dark_run_setting)

    def clear_settings(self):
        self._dark_run_settings = []
        self._dark_run_time_detector_setting = None
        self._dark_run_uamp_detector_setting = None
        self._dark_run_time_monitor_setting = None
        self._dark_run_uamp_monitor_setting = None

    # pylint: disable=too-many-arguments
    def execute(self, workspace, monitor_workspace, start_spec_index, end_spec_index, workspace_was_event):
        """
        Load the dark run. Cropt it to the current detector. Find out what kind of subtraction
        to perform and subtract the dark run from the workspace.
        @param workspace: the original workspace
        @param monitor_workspace: the associated monitor workspace
        @param  start_spec_index: the start workspace index to consider
        @param end_spec_index: the end workspace index to consider
        @param workspace_was_event: flag  if the original workspace was derived from histo or event
        @returns a corrected detector and monitor workspace
        """
        # The workspace signature of the execute method exists to make it an easy step
        # to convert to a full-grown reduction step for now we add it when converting
        # the data to histogram
        if not self.has_dark_runs():
            return workspace

        # Set the number of histograms
        self._monitor_list = self._get_monitor_list(monitor_workspace)

        # Get the time-based correction settings for detectors
        setting_time_detectors = self.get_time_based_setting_detectors()
        # Get the uamp-based correction settings for detectors
        setting_uamp_detectors = self.get_uamp_based_setting_detectors()
        # Get the time-based correction settings for monitors
        setting_time_monitors = self.get_time_based_setting_monitors()
        # Get the uamp-based correction settings for monitors
        setting_uamp_monitors = self.get_uamp_based_setting_monitors()

        monitor_settings = [setting_time_monitors, setting_uamp_monitors]
        detector_settings = [setting_time_detectors, setting_uamp_detectors]

        # Subtract the dark runs for the monitors
        for setting in monitor_settings:
            if setting is not None:
                monitor_workspace = self._execute_dark_run_subtraction_monitors(monitor_workspace, setting, workspace_was_event)

        # Subtract the dark runs for the detectors
        for setting in detector_settings:
            if setting is not None:
                workspace = self._execute_dark_run_subtraction_detectors(workspace, setting, start_spec_index, end_spec_index)

        return workspace, monitor_workspace

    def execute_transmission(self, workspace, trans_ids):
        """
        Performs the dark background correction for transmission and direct workspaces
        @param workspace: a transmission workspace (histogram!). We need to have a separate method
                          for transmission since we the format slightly different to the scattering
                          workspaces.
        @param transmission_ids: a list of transmission workspace indexes
        @returns a subtracted transmission workspace
        """
        if not self.has_dark_runs():
            return workspace

        # Set the number of histograms
        self._monitor_list = self._get_monitor_list(workspace)
        # Get the time-based correction settings for detectors
        setting_time_detectors = self.get_time_based_setting_detectors()
        # Get the uamp-based correction settings for detectors
        setting_uamp_detectors = self.get_uamp_based_setting_detectors()

        # Get the time-based correction settings for monitors
        setting_time_monitors = self.get_time_based_setting_monitors()
        # Get the uamp-based correction settings for monitors
        setting_uamp_monitors = self.get_uamp_based_setting_monitors()

        settings = [setting_time_monitors, setting_uamp_monitors, setting_time_detectors, setting_uamp_detectors]

        for setting in settings:
            if setting is not None:
                workspace = self._execute_dark_run_subtraction_for_transmission(workspace, setting, trans_ids)
        return workspace

    def _execute_dark_run_subtraction_for_transmission(self, workspace, setting, trans_ids):
        """
        @param workspace: the transmission workspace
        @param setting: the setting which is to be applied
        @param trans_ids: the detector ids which can be found in the transmission workspace
        @returns a subtracted transmission workspace
        """
        # Get the name and file path to the dark run
        dark_run_name, dark_run_file_path = self._get_dark_run_name_and_path(setting)

        # The transmission contains the monitors and the detectors
        dark_run_ws = self._load_dark_run_transmission(workspace, dark_run_name, dark_run_file_path, trans_ids)

        # Subtract the dark run from the workspace
        return self._subtract_dark_run(workspace, dark_run_ws, setting)

    def has_dark_runs(self):
        """
        Check if there are any dark run settings which are to be applied
        @returns true if there are any dark runs settings which are to be applied
        """
        if not self._dark_run_settings:
            return False
        else:
            return True

    def _load_dark_run_transmission(self, workspace, dark_run_name, dark_run_file_path, trans_ids):
        """
        Loads the dark run in the correct format for transmission correction. The dark run
        files will always be Event workspaces, hence we need load the detector and monitor
        separately and conjoin them.
        @param workspace
        @param dark_run_name
        @param dark_run_file_path
        @param trans_ids: the detector ids which can be found in the transmission workspace
        @returns a dark run workspace
        """
        # Load the monitors
        monitor = self._load_dark_run_monitors(dark_run_name, dark_run_file_path)
        monitor = self._rebin_to_match(workspace, monitor)

        # Load the detectors if the workspace contains detectors at all
        contains_detectors = True
        if len(self._monitor_list) == workspace.getNumberHistograms():
            contains_detectors = False

        out_ws = monitor
        if contains_detectors:
            start_spec_index = len(self._monitor_list) + 1
            end_spec_index = workspace.getNumberHistograms()  # It already contains the +1 offset
            workspace_index_offset = len(self._monitor_list)  # Note that this is needed to be consistent
            # with non-transmission correction
            detector = self._load_workspace(dark_run_name, dark_run_file_path, start_spec_index, end_spec_index, workspace_index_offset)
            detector = self._rebin_to_match(workspace, detector)
            # Conjoin the monitors and the detectors
            out_ws = self._conjoin_monitor_with_detector_workspace(monitor, detector)

        # Extract the spectra which are present in the transmission workspace
        extract_name = workspace.name() + "_extracted"
        alg_extract = AlgorithmManager.createUnmanaged("ExtractSpectra")
        alg_extract.initialize()
        alg_extract.setChild(True)
        alg_extract.setProperty("InputWorkspace", out_ws)
        alg_extract.setProperty("OutputWorkspace", extract_name)
        alg_extract.setProperty("OutputWorkspace", extract_name)
        alg_extract.setProperty("DetectorList", trans_ids)
        alg_extract.execute()
        return alg_extract.getProperty("OutputWorkspace").value

    def _get_monitor_list(self, monitor_workspace):
        """
        This makes use of the fact that the monitor data is always to be found at the front of the data set.
        If we are dealing with a monitor_workspace which originates from an event-based workspace, then all
        elements should be picked upt. If we are dealing with a monitor which originaates from a histo-based
        workspace, then only the first 8 to 10 indices should be found. This knowledge of the workspace layout
        helps to speed up things.
        @param monitor_workspace: the monitor workspace
        @returns a list with monitor spectra
        """
        monitor_indices = []
        for ws_index in range(monitor_workspace.getNumberHistograms()):
            try:
                det = monitor_workspace.getDetector(ws_index)
            except RuntimeError:
                # Skip the rest after finding the first spectra with no detectors,
                # which is a big speed increase for SANS2D.
                break
            if det.isMonitor():
                monitor_indices.append(ws_index)
        return monitor_indices

    def _execute_dark_run_subtraction_detectors(self, workspace, setting, start_spec_index, end_spec_index):
        """
        Apply one dark run setting to a detector workspace
        @param worksapce: the SANS data set with only detector data
        @param setting: a dark run settings tuple
        @param  start_spec_index: the start spec number to consider
        @param end_spec_index: the end spec number to consider
        """
        # Get the dark run name and path
        dark_run_name, dark_run_file_path = self._get_dark_run_name_and_path(setting)
        # Load the dark run workspace is it has not already been loaded
        dark_run_ws = self._load_dark_run_detectors(workspace, dark_run_name, dark_run_file_path, start_spec_index, end_spec_index)

        # Subtract the dark run from the workspace
        return self._subtract_dark_run(workspace, dark_run_ws, setting)

    # pylint: disable=too-many-arguments
    def _load_dark_run_detectors(self, workspace, dark_run_name, dark_run_file_path, start_spec_index, end_spec_index):
        """
        Loads a dark run workspace for detector subtraction if it has not already been loaded
        @param workspace: the scatter workspace
        @param dark_run_name: the name of the dark run workspace
        @param dark_run_file_path: the file path to the dark run
        @param  start_spec_index: the start spec number to consider
        @param end_spec_index: the end spec number to consider
        @returns a dark run workspace
        """
        # Load the dark run workspace
        dark_run = self._load_workspace(dark_run_name, dark_run_file_path, start_spec_index, end_spec_index, len(self._monitor_list))

        # Rebin to match the scatter workspace
        return self._rebin_to_match(workspace, dark_run)

    def _execute_dark_run_subtraction_monitors(self, monitor_workspace, setting, workspace_was_event):
        """
        Apply one dark run setting to a monitor workspace
        @param monitor_workspace: the monitor data set associated with the scatter data
        @param setting: a dark run settings tuple
        @param workspace_was_event: flag  if the original workspace was derived from histo or event
        @returns a monitor for the dark run
        """
        # Get the dark run name and path for the monitor
        dark_run_name, dark_run_file_path = self._get_dark_run_name_and_path(setting)
        # Load the dark run workspace is it has not already been loaded
        monitor_dark_run_ws = self._load_dark_run_monitors(dark_run_name, dark_run_file_path)

        # Check if the original workspace is based on Histo or Event. If it was an event workspace,
        # then the monitor workspace of the dark run should match (note that for event workspaces
        # the monitor workspace is separate). In the case of histo workspaces the monitor data
        # has all the histo data appended.
        out_ws = None
        if workspace_was_event:
            # Rebin to the dark run monitor workspace to the original monitor workspace
            monitor_dark_run_ws = self._rebin_to_match(monitor_workspace, monitor_dark_run_ws)
            out_ws = self._subtract_dark_run(monitor_workspace, monitor_dark_run_ws, setting)
        else:
            out_ws = self._get_monitor_workspace_from_original_histo_input(monitor_workspace, monitor_dark_run_ws, setting)

        return out_ws

    def _get_monitor_workspace_from_original_histo_input(self, monitor_workspace, monitor_dark_run_ws, setting):
        """
        Get the monitor workspace from an original histo input. Note that this is just the original input workspace.
        We will extract the monitor workspaces, do the subtraction and add it back to the original workspace.
        @param monitor_workspace: the monitor workspace (actually the full input)
        @param monitor_dark_run_ws: the dark run workspace
        @param setting: a dark run settings tuple
        @returns the corrected monitor workspace
        """
        # Extract the spectra
        monitor_temp_name = monitor_workspace.name() + "_mon_tmp"
        alg_extract = AlgorithmManager.createUnmanaged("ExtractSpectra")
        alg_extract.initialize()
        alg_extract.setChild(True)
        alg_extract.setProperty("InputWorkspace", monitor_workspace)
        alg_extract.setProperty("OutputWorkspace", monitor_temp_name)
        alg_extract.setProperty("WorkspaceIndexList", self._monitor_list)
        alg_extract.execute()
        ws_mon_tmp = alg_extract.getProperty("OutputWorkspace").value

        # Rebin to match
        monitor_dark_run_ws = self._rebin_to_match(ws_mon_tmp, monitor_dark_run_ws)

        # Subtract the two monitor workapces
        ws_mon_tmp = self._subtract_dark_run(ws_mon_tmp, monitor_dark_run_ws, setting)

        # Stitch back together
        monitor_workspace = self._crop_workspace_at_front(monitor_workspace, len(self._monitor_list))
        return self._conjoin_monitor_with_detector_workspace(ws_mon_tmp, monitor_workspace)

    def _load_dark_run_monitors(self, dark_run_name, dark_run_file_path):
        """
        Loads the monitor dark run workspace.
        @param dark_run_name: the name of the dark run workspace
        @param dark_run_file_path: the file path to the dark run
        @returns a monitor for the dark run
        """
        # This is a bit tricky. There are several possibilities. Of loading the monitor data
        # 1. The input is a standard workspace and we have to load via LoadNexusMonitor
        # 2. The input is a -add file. Then we need to load the file and extract the monitor of the added data set
        monitor_ws = None
        monitors_name = dark_run_name + "_monitors"
        # Check if the name ends with the  -add identifier. Then we know it has to be a group workspace
        if ADD_TAG in dark_run_name:
            alg_load_monitors = AlgorithmManager.createUnmanaged("LoadNexusProcessed")
            alg_load_monitors.initialize()
            alg_load_monitors.setChild(True)
            alg_load_monitors.setProperty("Filename", dark_run_file_path)
            alg_load_monitors.setProperty("EntryNumber", 2)
            alg_load_monitors.setProperty("OutputWorkspace", monitors_name)
            alg_load_monitors.execute()
            monitor_ws = alg_load_monitors.getProperty("OutputWorkspace").value
        else:
            try:
                alg_load_monitors = AlgorithmManager.createUnmanaged("LoadNexusMonitors")
                alg_load_monitors.initialize()
                alg_load_monitors.setChild(True)
                alg_load_monitors.setProperty("Filename", dark_run_file_path)
                alg_load_monitors.setProperty("LoadOnly", "Histogram")
                alg_load_monitors.setProperty("OutputWorkspace", monitors_name)
                alg_load_monitors.execute()
                monitor_ws = alg_load_monitors.getProperty("OutputWorkspace").value
            except RuntimeError as e:
                raise RuntimeError("DarkRunSubtration: Failed to load monitor for the specified dark run: " + e.message)
        return monitor_ws

    def _get_dark_run_name_and_path(self, setting):
        """
        @param settings: a dark run settings tuple
        @returns a dark run workspace name and the dark run path
        @raises RuntimeError: if there is an issue with loading the workspace
        """
        dark_run_ws_name = None
        dark_run_file_path = None
        try:
            dark_run_file_path, dark_run_ws_name = getFileAndName(setting.run_number)
            dark_run_file_path = dark_run_file_path.replace("\\", "/")
        except:
            raise RuntimeError(
                "DarkRunSubtration: The specified dark run file cannot be found or loaded. "
                "Please make sure that that it exists in your search directory."
            )
        return dark_run_ws_name, dark_run_file_path

    # pylint: disable=too-many-arguments
    def _load_workspace(self, dark_run_ws_name, dark_run_file_path, start_spec_index, end_spec_index, workspace_index_offset):
        """
        Loads the dark run workspace
        @param dark_run_file_path: file path to the dark run
        @param dark_run_ws_name: the name of the dark run workspace
        @param  start_spec_index: the start spec number to consider
        @param end_spec_index: the end spec number to consider
        @param workspace_index_offset: the number of monitors
        @returns a dark run
        @raises RuntimeError: if there is an issue with loading the workspace
        """
        dark_run_ws = None
        if ADD_TAG in dark_run_ws_name:
            alg_load = AlgorithmManager.createUnmanaged("LoadNexusProcessed")
            alg_load.initialize()
            alg_load.setChild(True)
            alg_load.setProperty("Filename", dark_run_file_path)
            # We specifically grab the first entry here. When the Nexus file is a Group worspace (due
            # to added files) then we specifically only want the first workspace.
            alg_load.setProperty("EntryNumber", 1)
            alg_load.setProperty("OutputWorkspace", dark_run_ws_name)
            alg_load.execute()
            dark_run_ws = alg_load.getProperty("OutputWorkspace").value
        else:
            try:
                alg_load = AlgorithmManager.createUnmanaged("Load")
                alg_load.initialize()
                alg_load.setChild(True)
                alg_load.setProperty("Filename", dark_run_file_path)
                alg_load.setProperty("OutputWorkspace", dark_run_ws_name)
                alg_load.execute()
                dark_run_ws = alg_load.getProperty("OutputWorkspace").value
            except RuntimeError as e:
                raise RuntimeError("DarkRunSubtration: The specified dark run file failed to load: " + e.message)

        # Crop the workspace if this is required
        if dark_run_ws.getNumberHistograms() != (end_spec_index - start_spec_index + 1):
            start_ws_index, end_ws_index = self._get_start_and_end_ws_index(start_spec_index, end_spec_index, workspace_index_offset)
            # Now crop the workspace to the correct size
            cropped_name = dark_run_ws_name + "_cropped"
            alg_crop = AlgorithmManager.createUnmanaged("CropWorkspace")
            alg_crop.initialize()
            alg_crop.setChild(True)
            alg_crop.setProperty("InputWorkspace", dark_run_ws)
            alg_crop.setProperty("OutputWorkspace", cropped_name)
            alg_crop.setProperty("StartWorkspaceIndex", start_ws_index)
            alg_crop.setProperty("EndWorkspaceIndex", end_ws_index)
            alg_crop.execute()
            dark_run_ws = alg_crop.getProperty("OutputWorkspace").value
        return dark_run_ws

    def _get_start_and_end_ws_index(self, start_spec_index, end_spec_index, workspace_index_offset):
        """
        Get the start and stop index taking into account the monitor offset
        in the original combined workspace. The start_ws_index and end_ws_index
        are based on workspaces which contain the monitor data. Our loaded workspace
        would not contain the monitor data. Also there is an offset of 1 between
        the workspace index and the mon spectrum
        @param start_spec_index: the start spec index
        @param end_spec_index: the end spec index
        @param workspace_index_offset: the workspaec index offset due to the monitors
        """
        # Correct for the spec-ws_index offset and for the monitors
        start_ws_index = start_spec_index - workspace_index_offset - 1
        end_ws_index = end_spec_index - workspace_index_offset - 1
        return start_ws_index, end_ws_index

    def _rebin_to_match(self, workspace, to_rebin):
        """
        Rebin too match the input workspace
        """
        rebinned_name = to_rebin.name() + "_rebinned"
        alg_rebin = AlgorithmManager.createUnmanaged("RebinToWorkspace")
        alg_rebin.initialize()
        alg_rebin.setChild(True)
        alg_rebin.setProperty("WorkspaceToRebin", to_rebin)
        alg_rebin.setProperty("WorkspaceToMatch", workspace)
        alg_rebin.setProperty("PreserveEvents", False)
        alg_rebin.setProperty("OutputWorkspace", rebinned_name)
        alg_rebin.execute()
        return alg_rebin.getProperty("OutputWorkspace").value

    def _subtract_dark_run(self, workspace, dark_run, setting):
        """
        Subtract the dark run from the SANS workspace
        @param worksapce: the SANS data set
        @param dark_run: the dark run workspace
        @param setting: a dark run settings tuple
        """
        dark_run_correction = DarkCorr.DarkRunCorrection()
        dark_run_correction.set_use_mean(setting.mean)
        dark_run_correction.set_use_time(setting.time)
        dark_run_correction.set_use_detectors(setting.detector)
        dark_run_correction.set_use_monitors(setting.mon)
        dark_run_correction.set_mon_numbers(setting.mon_numbers)
        return dark_run_correction.execute(scatter_workspace=workspace, dark_run=dark_run)

    def _crop_workspace_at_front(self, workspace, start_ws_index):
        """
        Creates a cropped workspace
        @param workspace: the workspace to be cropped
        @param start_ws_index: the start index
        @returns a workspace which has the front cropped off
        """
        cropped_temp_name = workspace.name() + "_cropped_tmp"
        alg_cropped = AlgorithmManager.createUnmanaged("ExtractSpectra")
        alg_cropped.initialize()
        alg_cropped.setChild(True)
        alg_cropped.setProperty("InputWorkspace", workspace)
        alg_cropped.setProperty("OutputWorkspace", cropped_temp_name)
        alg_cropped.setProperty("StartWorkspaceIndex", start_ws_index)
        alg_cropped.execute()
        return alg_cropped.getProperty("OutputWorkspace").value

    def _conjoin_monitor_with_detector_workspace(self, monitor, detector):
        """
        Conjoins the monitor data set with the detector data set
        @param monitor: the monitor data set
        @param detector: the detector data set
        @returns a conjoined data set
        """
        alg_conjoined = AlgorithmManager.createUnmanaged("ConjoinWorkspaces")
        alg_conjoined.initialize()
        alg_conjoined.setChild(True)
        alg_conjoined.setProperty("InputWorkspace1", monitor)
        alg_conjoined.setProperty("InputWorkspace2", detector)
        alg_conjoined.execute()
        return alg_conjoined.getProperty("InputWorkspace1").value

    def get_time_based_setting_detectors(self):
        """
        Retrieve the time-based setting for detectors if there is one
        @returns the time-based setting or None
        """
        self._evaluate_settings()
        return self._dark_run_time_detector_setting

    def get_uamp_based_setting_detectors(self):
        """
        Retrieve the uamp-based setting for detectors if there is one
        @returns the uamp-based setting or None
        """
        self._evaluate_settings()
        return self._dark_run_uamp_detector_setting

    def get_time_based_setting_monitors(self):
        """
        Retrieve the time-based setting for monitors if there is one
        @returns the time-based setting or None
        """
        self._evaluate_settings()
        return self._dark_run_time_monitor_setting

    def get_uamp_based_setting_monitors(self):
        """
        Retrieve the uamp-based setting for monitors if there is one
        @returns the uamp-based setting or None
        """
        self._evaluate_settings()
        return self._dark_run_uamp_monitor_setting

    def _evaluate_settings(self):
        """
        Takes the dark run settings and merges the appropriate files into
        a time-based setting and a uamp-based setting each for detectors
        and for monitors.
        @returns the final settings
        @raises RuntimeError: if settings values are inconsistent
        """
        use_mean = []
        use_time = []
        use_mon = []
        mon_number = []
        run_number = []

        for setting in self._dark_run_settings:
            use_mean.append(setting.mean)
            use_time.append(setting.time)
            use_mon.append(setting.mon)
            mon_number.append(setting.mon_number)
            run_number.append(setting.run_number)

        # Get the indices with settings which correspond to the individual settings
        def get_indices(time_flag, mon_flag):
            return [i for i, use_time_i in enumerate(use_time) if use_time_i == time_flag and use_mon[i] == mon_flag]

        indices_time_detector = get_indices(True, False)
        indices_time_monitor = get_indices(True, True)
        indices_uamp_detector = get_indices(False, False)
        indices_uamp_monitor = get_indices(False, True)

        # Check that for each of these settings we only have one run number specified, else raise an error
        def has_max_one_run_number(indices):
            return len(set([run_number[index] for index in indices])) < 2

        if (
            not has_max_one_run_number(indices_time_detector)
            or not has_max_one_run_number(indices_time_monitor)
            or not has_max_one_run_number(indices_uamp_detector)
            or not has_max_one_run_number(indices_uamp_monitor)
        ):
            raise RuntimeError(
                "DarkRunSubtraction: More background correction runs have been "
                "specified than are allowed. "
                "There can be maximally one run number for each time-based detector, "
                "uamp-based detector, time-based monitor and uamp-based monitor settings.\n"
            )

        # Handle detectors
        self._dark_run_time_detector_setting = self._get_final_setting_detectors(run_number, use_mean, use_time, indices_time_detector)
        self._dark_run_uamp_detector_setting = self._get_final_setting_detectors(run_number, use_mean, use_time, indices_uamp_detector)

        # handle monitors
        self._dark_run_time_monitor_setting = self._get_final_setting_monitors(
            run_number, use_mean, use_time, mon_number, indices_time_monitor
        )
        self._dark_run_uamp_monitor_setting = self._get_final_setting_monitors(
            run_number, use_mean, use_time, mon_number, indices_uamp_monitor
        )

    def _get_final_setting_detectors(self, run_number, use_mean, use_time, indices):
        """
        Get the final settings for detectors
        @param run_number: a list of run numbers
        @param use_mean: a list of mean flags
        @param use_time: a list of time flags
        @param indices: a list if indices
        @returns a valid setting for detectors or None
        @raises RuntimeError: if there is more than one index specified
        """
        # We want to make sure that there is only one index here. It might be that the user
        # specified two settings with the same run number. We need to catch this here.
        if len(indices) > 1:
            raise RuntimeError("DarkRunSubtraction: The setting for detectors can only be specified once.")

        detector_runs = [run_number[index] for index in indices]
        detector_mean = [use_mean[index] for index in indices]
        detector_time = [use_time[index] for index in indices]

        if len(detector_runs) == 0 or detector_runs[0] is None:
            return None
        else:
            return DarkRunSubtraction.DarkRunSubtractionSettings(
                run_number=detector_runs[0], time=detector_time[0], mean=detector_mean[0], detector=True, mon=False, mon_numbers=None
            )

    # pylint: disable=too-many-arguments

    def _get_final_setting_monitors(self, run_number, use_mean, use_time, mon_numbers, indices):
        """
        Get the final settings for monitors
        @param run_number: a list of run numbers
        @param use_mean: a list of mean flags
        @param use_time: a list of time flags
        @param indices: a list if indices
        @returns a valid setting for detectors or None
        @raises RuntimeError: if settings are not consistent
        """
        # Note that we can have several mon settings, e.g several mon numbers etc.
        # So we cannot treat this like detectors
        monitor_runs = [run_number[index] for index in indices]
        monitor_mean = [use_mean[index] for index in indices]
        monitor_time = [use_time[index] for index in indices]
        monitor_numbers_to_check = [mon_numbers[index] for index in indices]
        monitor_mon_numbers = []

        # If there is a pure MON setting then we take all Monitors into account, this
        # overrides the Mx settings.
        if len(monitor_numbers_to_check) > 0 and None in monitor_numbers_to_check:
            monitor_mon_numbers = None
        else:
            for index in indices:
                if isinstance(mon_numbers[index], list):
                    monitor_mon_numbers.extend(mon_numbers[index])
                else:
                    monitor_mon_numbers.append(mon_numbers[index])

        # Check if the mean value is identical for all entries
        if len(monitor_mean) > 0:
            if len(set(monitor_mean)) != 1:
                raise RuntimeError(
                    "DarkRunSubtraction: If several monitors are specified for a certain type "
                    "of subtraction, it is required to use either all MEAN or all TOF."
                )

        # If the runs are empty or None then we don't have any settings here
        unique_runs = list(set(monitor_runs))
        if len(unique_runs) == 0 or monitor_runs[0] is None:
            return None
        else:
            return DarkRunSubtraction.DarkRunSubtractionSettings(
                run_number=monitor_runs[0],
                time=monitor_time[0],
                mean=monitor_mean[0],
                detector=False,
                mon=True,
                mon_numbers=monitor_mon_numbers,
            )


class CropDetBank(ReductionStep):
    """
    Takes the spectra range of the current detector from the instrument object
    and crops the input workspace to just those spectra. Supports optionally
    generating the output workspace from a different (sample) workspace
    """

    def __init__(self):
        """
        Sets up the object to either the output or sample workspace
        """
        super(CropDetBank, self).__init__()

    def execute(self, reducer, workspace):
        in_wksp = workspace

        # Get the detector bank that is to be used in this analysis leave the complete workspace
        reducer.instrument.cur_detector().crop_to_detector(in_wksp, workspace)

        # If the workspace requires dark run subtraction for the detectors and monitors, then this will be performed here.
        if reducer.dark_run_subtraction.has_dark_runs():
            # Get the spectrum index range for spectra which are part of the current detector
            start_spec_num = reducer.instrument.cur_detector().get_first_spec_num()
            end_spec_num = reducer.instrument.cur_detector().last_spec_num

            scatter_ws = getWorkspaceReference(workspace)
            scatter_name = scatter_ws.name()

            monitor_ws = reducer.get_sample().get_monitor()
            monitor_name = monitor_ws.name()

            # Run the subtraction
            was_event_workspace = reducer.is_based_on_event()
            scatter_ws, monitor_ws = reducer.dark_run_subtraction.execute(
                scatter_ws, monitor_ws, start_spec_num, end_spec_num, was_event_workspace
            )

            # We need to replace the workspaces in the ADS
            mtd.addOrReplace(scatter_name, scatter_ws)
            mtd.addOrReplace(monitor_name, monitor_ws)


class NormalizeToMonitor(ReductionStep):
    """
    Before normalisation the monitor spectrum's background is removed
    and for LOQ runs also the prompt peak. The input workspace is copied
    and accessible later as prenomed
    """

    NORMALISATION_SPEC_NUMBER = 1
    NORMALISATION_SPEC_INDEX = 0

    def __init__(self, spectrum_number=None):
        super(NormalizeToMonitor, self).__init__()
        self._normalization_spectrum = spectrum_number

        # the result of this calculation that will be used by CalculateNorm() and the ConvertToQ
        self.output_wksp = None

    def execute(self, reducer, workspace):
        self.set_prompt_parameter_if_not_set(reducer)

        normalization_spectrum = self._normalization_spectrum
        if normalization_spectrum is None:
            # the -1 converts from spectrum number to spectrum index
            normalization_spectrum = reducer.instrument.get_incident_mon()

        sanslog.notice("Normalizing to monitor " + str(normalization_spectrum))

        self.output_wksp = str(workspace) + INCIDENT_MONITOR_TAG
        mon = reducer.get_sample().get_monitor(normalization_spectrum - 1)

        # Unwrap the monitors of the scatter workspace
        if reducer.unwrap_monitors:
            wavelength_min, wavelength_max = get_wavelength_min_and_max(reducer)
            temp_mon = UnwrapMonitorsInTOF(InputWorkspace=mon, WavelengthMin=wavelength_min, WavelengthMax=wavelength_max)
            DeleteWorkspace(mon)
            mon = temp_mon

        if reducer.event2hist.scale != 1:
            mon *= reducer.event2hist.scale

        if str(mon) != self.output_wksp:
            RenameWorkspace(mon, OutputWorkspace=self.output_wksp)

        if (
            is_prompt_peak_instrument(reducer)
            and reducer.transmission_calculator.removePromptPeakMin is not None
            and reducer.transmission_calculator.removePromptPeakMax is not None
        ):
            RemoveBins(
                InputWorkspace=self.output_wksp,
                OutputWorkspace=self.output_wksp,
                XMin=reducer.transmission_calculator.removePromptPeakMin,
                XMax=reducer.transmission_calculator.removePromptPeakMax,
                Interpolation="Linear",
            )

        # Remove flat background
        TOF_start, TOF_end = reducer.inst.get_TOFs(normalization_spectrum)

        if TOF_start and TOF_end:
            CalculateFlatBackground(
                InputWorkspace=self.output_wksp, OutputWorkspace=self.output_wksp, StartX=TOF_start, EndX=TOF_end, Mode="Mean"
            )

        # perform the same conversion on the monitor spectrum as was applied to the workspace but with a possibly different rebin
        if reducer.instrument.is_interpolating_norm():
            r_alg = "InterpolatingRebin"
        else:
            r_alg = "Rebin"
        reducer.to_wavelen.execute(reducer, self.output_wksp, bin_alg=r_alg)

    def set_prompt_parameter_if_not_set(self, reducer):
        """
        This method sets default prompt peak values in case the user has not provided some. Currently
        we only use default values for LOQ.
        """
        if reducer.transmission_calculator.removePromptPeakMin is None and reducer.transmission_calculator.removePromptPeakMax is None:
            if reducer.instrument.name() == "LOQ":
                reducer.transmission_calculator.removePromptPeakMin = 19000.0  # Units of micro-seconds
                reducer.transmission_calculator.removePromptPeakMax = 20500.0  # Units of micro-seconds


class TransmissionCalc(ReductionStep):
    """
    Calculates the proportion of neutrons that are transmitted through the sample
    as a function of wavelength. The results are stored as a workspace
    """

    # The different ways of doing a fit, convert the possible ways of specifying this (also the way it is specified
    # in the GUI to the way it can be send to CalculateTransmission
    TRANS_FIT_OPTIONS = {
        "YLOG": "Log",
        "STRAIGHT": "Linear",
        "CLEAR": "Linear",
        # Add Mantid ones as well
        "LOGARITHMIC": "Log",
        "LOG": "Log",
        "LINEAR": "Linear",
        "LIN": "Linear",
        "OFF": "Linear",
        "POLYNOMIAL": "Polynomial",
    }

    # map to restrict the possible values of _trans_type
    CAN_SAMPLE_SUFFIXES = {False: "sample", True: "can"}

    DEFAULT_FIT = "LOGARITHMIC"

    # The y unit label for transmission data
    YUNITLABEL_TRANSMISSION_RATIO = "Transmission"

    def __init__(self, loader=None):
        super(TransmissionCalc, self).__init__()
        # set these variables to None, which means they haven't been set and defaults will be set further down
        self.fit_props = ["lambda_min", "lambda_max", "fit_method", "order"]
        self.fit_settings = dict()
        for prop in self.fit_props:
            self.fit_settings["both::" + prop] = None

        # CalculateTransmission can be given either a monitor detetor ID or a set of detector
        # ID's corresponding to a ROI (region of interest).  The monitor or ROI will specify
        # the *transmission* (not the incident beam).  A monitor is the standard functionality,
        # a ROI is needed for the new "beam stop out" functionality.
        self.trans_mon = None
        self.trans_roi = []

        # Contributions to the region of interest. Note that radius, roi_files add to the region
        # of interest, while mask_files are taboo for the region of interest
        self.radius = None
        self.roi_files = []
        self.mask_files = []

        # use InterpolatingRebin
        self.interpolate = None
        # a custom transmission workspace, if we have this there is much less to do
        self.calculated_samp = ""
        self.calculated_can = None
        # the result of this calculation that will be used by CalculateNorm() and the ConvertToQ
        self.output_wksp = None
        # Use for removing LOQ/LARMOR prompt peak from monitors. Units of micro-seconds
        self.removePromptPeakMin = None
        self.removePromptPeakMax = None

    def set_trans_fit(self, fit_method, min_=None, max_=None, override=True, selector="both"):
        """
        Set how the transmission fraction fit is calculated, the range of wavelengths
        to use and the fit method
        @param min: minimum wavelength to use
        @param max: highest wavelength to use
        @param fit_method: the fit type to pass to CalculateTransmission ('Logarithmic' or 'Linear')or 'Off'
        @param override: if set to False this call won't override the settings set by a previous call (default True)
        @param selector: define if the given settings is valid for SAMPLE, CAN or BOTH transmissions.
        """
        FITMETHOD = "fit_method"
        LAMBDAMIN = "lambda_min"
        LAMBDAMAX = "lambda_max"
        ORDER = "order"
        # processing the selector input
        select = selector.lower()
        if select not in ["both", "can", "sample"]:
            _issueWarning("Invalid selector option (" + selector + "). Fit to transmission skipped")
            return
        select += "::"

        if not override and select + FITMETHOD in self.fit_settings and self.fit_settings[select + FITMETHOD]:
            # it was already configured and this request does not want to override
            return

        if not fit_method:
            # there is not point calling fit_method without fit_method argument
            return

        fit_method = fit_method.upper()
        if "POLYNOMIAL" in fit_method:
            order_str = fit_method[10:]
            fit_method = "POLYNOMIAL"
            self.fit_settings[select + ORDER] = int(order_str)
        if fit_method not in list(self.TRANS_FIT_OPTIONS.keys()):
            _issueWarning(
                "ISISReductionStep.Transmission: Invalid fit mode passed to TransFit, using default method (%s)" % self.DEFAULT_FIT
            )
            fit_method = self.DEFAULT_FIT

        # get variables for this selector
        sel_settings = dict()
        for prop in self.fit_props:
            sel_settings[prop] = (
                self.fit_settings[select + prop] if select + prop in self.fit_settings else self.fit_settings["both::" + prop]
            )

        # copy fit_method
        sel_settings[FITMETHOD] = fit_method

        if min_:
            sel_settings[LAMBDAMIN] = float(min_) if fit_method not in ["OFF", "CLEAR"] else None
        if max_:
            sel_settings[LAMBDAMAX] = float(max_) if fit_method not in ["OFF", "CLEAR"] else None

        # apply the properties to self.fit_settings
        for prop in self.fit_props:
            self.fit_settings[select + prop] = sel_settings[prop]

        # When both is given, it is necessary to clean the specific settings for the individual selectors
        if select == "both::":
            for selector_ in ["sample::", "can::"]:
                for prop_ in self.fit_props:
                    prop_name = selector_ + prop_
                    if prop_name in self.fit_settings:
                        del self.fit_settings[prop_name]

    def isSeparate(self):
        """Returns true if the can or sample was given and false if just both was used"""
        return "sample::fit_method" in self.fit_settings or "can::fit_method" in self.fit_settings

    def setup_wksp(self, inputWS, inst, wavbining, trans_det_ids, reducer):
        """
        Creates a new workspace removing any background from the monitor spectra, converting units
        and re-bins. If the instrument is LOQ it zeros values between the x-values 19900 and 20500
        This method doesn't affect self.
        @param inputWS: contains the monitor spectra
        @param inst: the selected instrument
        @param wavbinning: the re-bin string to use after convert units
        @param trans_det_ids: detector IDs corresponding to the incident monitor and either:
                              a) the transmission monitor, or
                              b) a list of detector that make up a region of interest
                              Together these make up all spectra needed to carry out the
                              CalculateTransmission algorithm.
        @param reducer: the reducer singleton
        @return the name of the workspace created
        """
        # the workspace is forked, below is its new name
        tmpWS = inputWS + "_tmp"

        # A previous implementation of this code had a comment which suggested
        # that we have to exclude unused spectra as the interpolation runs into
        # problems if we don't.
        extract_spectra(mtd[inputWS], trans_det_ids, tmpWS)

        # If the transmission and direct workspaces require an unwrapping of the monitors then do it here
        if reducer.unwrap_monitors:
            wavelength_min, wavelength_max = get_wavelength_min_and_max(reducer)
            UnwrapMonitorsInTOF(InputWorkspace=tmpWS, OutputWorkspace=tmpWS, WavelengthMin=wavelength_min, WavelengthMax=wavelength_max)

        # Perform the a dark run background correction if one was specified
        self._correct_dark_run_background(reducer, tmpWS, trans_det_ids)

        if is_prompt_peak_instrument(reducer) and self.removePromptPeakMin is not None and self.removePromptPeakMax is not None:
            RemoveBins(
                InputWorkspace=tmpWS,
                OutputWorkspace=tmpWS,
                XMin=self.removePromptPeakMin,
                XMax=self.removePromptPeakMax,
                Interpolation="Linear",
            )

        tmp = mtd[tmpWS]
        # We perform a FlatBackground correction. We do this in two parts.
        # First we find the workspace indices which correspond to monitors
        # and perform the correction on these indices.
        # Second we perform the correction on all indices which are not
        # monitors
        if tmp.getNumberHistograms() > 0:
            ws_index = 0
            for ws_index in range(tmp.getNumberHistograms()):
                if tmp.getDetector(ws_index).isMonitor():
                    spectrum_number = tmp.getSpectrum(ws_index).getSpectrumNo()
                    back_start_mon, back_end_mon = inst.get_TOFs(spectrum_number)
                    if back_start_mon and back_end_mon:
                        CalculateFlatBackground(
                            InputWorkspace=tmpWS,
                            OutputWorkspace=tmpWS,
                            StartX=back_start_mon,
                            EndX=back_end_mon,
                            WorkspaceIndexList=ws_index,
                            Mode="Mean",
                        )

            back_start_roi, back_end_roi = inst.get_TOFs_for_ROI()
            if back_start_roi and back_end_roi:
                CalculateFlatBackground(
                    InputWorkspace=tmpWS,
                    OutputWorkspace=tmpWS,
                    StartX=back_start_roi,
                    EndX=back_end_roi,
                    WorkspaceIndexList=ws_index,
                    Mode="Mean",
                    SkipMonitors=True,
                )

            ConvertUnits(InputWorkspace=tmpWS, OutputWorkspace=tmpWS, Target="Wavelength")

            if self.interpolate:
                InterpolatingRebin(InputWorkspace=tmpWS, OutputWorkspace=tmpWS, Params=wavbining)
            else:
                Rebin(InputWorkspace=tmpWS, OutputWorkspace=tmpWS, Params=wavbining)
            return tmpWS
        else:
            raise RuntimeError("The number of histograms in the workspace must be greater than 0")

    def _get_index(self, number):
        """
        Converts spectrum numbers to indices using the simple (minus 1) relationship
        that is true for raw files
        @param number: a spectrum number
        @return: its index
        """
        return number - 1

    def calculate_region_of_interest(self, reducer, workspace):
        """
        Calculate the various contributions to the "region of interest", used in the
        transmission calculation.

        The region of interest can be made up of a circle of detectors (with a given radius)
        around the beam centre, and/or one or more mask files, and/or the main detector bank.
        Note that the mask files wont actually be used for masking, we're just piggy-backing
        on the functionality that they provide. Note that in the case of a radius, we have
        to ensure that we do not use a workspace which already has masked detectors, since
        they would contribute to the ROI.
        """
        if self.radius:
            # Mask out a cylinder with the given radius in a copy of the workspace.
            # The centre position of the Cylinder does not require a shift, as all
            # components have been shifted already, when the workspaces were loaded
            CloneWorkspace(InputWorkspace=workspace, OutputWorkspace="__temp")
            centre_x = 0.0
            centre_y = 0.0
            MaskWithCylinder("__temp", self.radius, centre_x, centre_y, "")

            # Extract the masked detector ID's and then clean up.
            self.trans_roi += get_masked_det_ids(mtd["__temp"])
            DeleteWorkspace(Workspace="__temp")

        if self.roi_files:
            idf_path = reducer.instrument.idf_path
            for roi_file in self.roi_files:
                self.trans_roi += get_masked_det_ids_from_mask_file(roi_file, idf_path)

        masked_ids = []
        if self.mask_files:
            idf_path = reducer.instrument.idf_path
            for mask_file in self.mask_files:
                masked_ids += get_masked_det_ids_from_mask_file(mask_file, idf_path)

        # Detector ids which are not allowed and specified by "masked_ids" need to
        # be removed from the trans_roi list
        # Remove duplicates and sort.
        self.trans_roi = sorted(set(self.trans_roi) - set(masked_ids))

    def execute(self, reducer, workspace):
        """
        Reads in the different settings, without affecting self. Calculates
        or estimates the proportion of neutrons that are transmitted
        through the sample
        """
        # Set the prompt peak default values
        self.set_prompt_parameter_if_not_set(reducer)

        self.output_wksp = None

        # look for run files that contain transmission data
        test1, test2 = self._get_run_wksps(reducer)
        if test1 or test2:
            # we can calculate the transmission from some experimental runs
            if self.calculated_samp:
                raise RuntimeError("Cannot use TransWorkspace() and TransmissionSample() together")

            self.output_wksp = self.calculate(reducer)
        else:
            # they have supplied a transmission file use it
            if reducer.is_can():
                self.output_wksp = self.calculated_can
            else:
                self.output_wksp = self.calculated_samp

    def _get_run_wksps(self, reducer):
        """
        Retrieves the names runs that contain the user specified for calculation
        of the transmission
        @return: post_sample pre_sample workspace names
        """
        return reducer.get_transmissions()

    def calculate(self, reducer):
        LAMBDAMIN = "lambda_min"
        LAMBDAMAX = "lambda_max"
        FITMETHOD = "fit_method"
        ORDER = "order"
        # get the settings required to do the calculation
        trans_raw, direct_raw = self._get_run_wksps(reducer)

        if not trans_raw:
            raise RuntimeError(
                "Attempting transmission correction with no specified transmission %s file" % self.CAN_SAMPLE_SUFFIXES[reducer.is_can()]
            )
        if not direct_raw:
            raise RuntimeError("Attempting transmission correction with no direct file")

        # Calculate the ROI. We use the trans_raw workspace as it does not contain any previous mask.
        self.calculate_region_of_interest(reducer, trans_raw)

        select = "can::" if reducer.is_can() else "direct::"

        # get variables for this selector
        sel_settings = dict()
        for prop in self.fit_props:
            sel_settings[prop] = (
                self.fit_settings[select + prop] if select + prop in self.fit_settings else self.fit_settings["both::" + prop]
            )

        pre_sample = reducer.instrument.incid_mon_4_trans_calc

        # Here we set the det ids. The first entry is the incident monitor. There after we can either have
        # 1. The transmission monitor or
        # 2. A ROI (which are not monitors!) or
        # 3. trans_specs (also monitors)
        trans_det_ids = [pre_sample]
        if self.trans_mon:
            trans_det_ids.append(self.trans_mon)
        elif self.trans_roi:
            trans_det_ids += self.trans_roi
        else:
            trans_det_ids.append(reducer.instrument.default_trans_spec)

        use_instrum_default_range = reducer.full_trans_wav

        # there are a number of settings and defaults that determine the wavelength to use, go through each in
        # order of increasing precedence
        if use_instrum_default_range:
            translambda_min = reducer.instrument.WAV_RANGE_MIN
            translambda_max = reducer.instrument.WAV_RANGE_MAX
        else:
            if sel_settings[LAMBDAMIN]:
                translambda_min = sel_settings[LAMBDAMIN]
            else:
                translambda_min = reducer.to_wavelen.wav_low
            if sel_settings[LAMBDAMAX]:
                translambda_max = sel_settings[LAMBDAMAX]
            else:
                translambda_max = reducer.to_wavelen.wav_high

        wavbin = str(translambda_min)
        wavbin += "," + str(reducer.to_wavelen.wav_step)
        wavbin += "," + str(translambda_max)

        # set up the input workspaces
        trans_tmp_out = self.setup_wksp(trans_raw, reducer.instrument, wavbin, trans_det_ids, reducer)
        direct_tmp_out = self.setup_wksp(direct_raw, reducer.instrument, wavbin, trans_det_ids, reducer)

        # Where a ROI has been specified, it is useful to keep a copy of the
        # summed ROI spectra around for the scientists to look at, so that they
        # may inspect it for any dubious looking spikes, etc.
        if self.trans_roi:
            EXCLUDE_INIT_BEAM = 1
            SumSpectra(InputWorkspace=trans_tmp_out, OutputWorkspace=trans_raw + "_num", StartWorkspaceIndex=EXCLUDE_INIT_BEAM)
            SumSpectra(InputWorkspace=direct_tmp_out, OutputWorkspace=direct_raw + "_den", StartWorkspaceIndex=EXCLUDE_INIT_BEAM)

        fittedtransws, unfittedtransws = self.get_wksp_names(trans_raw, translambda_min, translambda_max, reducer)

        # If no fitting is required just use linear and get unfitted data from CalculateTransmission algorithm
        options = dict()
        if sel_settings[FITMETHOD]:
            options["FitMethod"] = self.TRANS_FIT_OPTIONS[sel_settings[FITMETHOD]]
            if sel_settings[FITMETHOD] == "POLYNOMIAL":
                options["PolynomialOrder"] = sel_settings[ORDER]
        else:
            options["FitMethod"] = self.TRANS_FIT_OPTIONS[self.DEFAULT_FIT]

        calc_trans_alg = AlgorithmManager.create("CalculateTransmission")
        calc_trans_alg.initialize()
        calc_trans_alg.setProperty("SampleRunWorkspace", trans_tmp_out)
        calc_trans_alg.setProperty("DirectRunWorkspace", direct_tmp_out)
        calc_trans_alg.setProperty("OutputWorkspace", fittedtransws)
        calc_trans_alg.setProperty("IncidentBeamMonitor", pre_sample)
        calc_trans_alg.setProperty("RebinParams", reducer.to_wavelen.get_rebin())
        calc_trans_alg.setProperty("OutputUnfittedData", True)
        for name, value in list(options.items()):
            calc_trans_alg.setProperty(name, value)

        if self.trans_mon:
            calc_trans_alg.setProperty("TransmissionMonitor", self.trans_mon)
        elif self.trans_roi:
            calc_trans_alg.setProperty("TransmissionROI", self.trans_roi)
        else:
            calc_trans_alg.setProperty("TransmissionMonitor", reducer.instrument.default_trans_spec)

        calc_trans_alg.execute()

        # Set the y axis label correctly for the transmission ratio data
        try:
            fitted_trans_ws = mtd[fittedtransws]
            unfitted_trans_ws = mtd[unfittedtransws]
        except KeyError as err:
            message = (
                "Something went wrong during the transmission calculation. The error message is "
                "{0}. Check if you have any signal in the monitors which are used to record the "
                "transmission and normalisation signal.".format(str(err))
            )
            raise RuntimeError(message)
        if fitted_trans_ws:
            fitted_trans_ws.setYUnitLabel(self.YUNITLABEL_TRANSMISSION_RATIO)
        if unfitted_trans_ws:
            unfitted_trans_ws.setYUnitLabel(self.YUNITLABEL_TRANSMISSION_RATIO)

        # Remove temporaries
        files2delete = [trans_tmp_out]

        if direct_tmp_out != trans_tmp_out:
            files2delete.append(direct_tmp_out)

        if sel_settings[FITMETHOD] in ["OFF", "CLEAR"]:
            result = unfittedtransws
            files2delete.append(fittedtransws)
        else:
            result = fittedtransws

        reducer.deleteWorkspaces(files2delete)

        return result

    def get_wksp_names(self, raw_name, lambda_min, lambda_max, reducer):
        fitted_name = raw_name.split("_")[0] + "_trans_"
        fitted_name += self.CAN_SAMPLE_SUFFIXES[reducer.is_can()]
        fitted_name += "_" + str(lambda_min) + "_" + str(lambda_max)

        unfitted = fitted_name + "_unfitted"

        return fitted_name, unfitted

    def _get_fit_property(self, selector, property_name):
        if selector + "::" + property_name in self.fit_settings:
            return self.fit_settings[selector + "::" + property_name]
        else:
            return self.fit_settings["both::" + property_name]

    def lambdaMin(self, selector):
        return self._get_fit_property(selector.lower(), "lambda_min")

    def lambdaMax(self, selector):
        return self._get_fit_property(selector.lower(), "lambda_max")

    def fitMethod(self, selector):
        """It will return LINEAR, LOGARITHM, POLYNOMIALx for x in 2,3,4,5"""
        resp = self._get_fit_property(selector.lower(), "fit_method")
        if resp == "POLYNOMIAL":
            resp += str(self._get_fit_property(selector.lower(), "order"))
        if resp in ["LIN", "STRAIGHT"]:
            resp = "LINEAR"
        if resp in ["YLOG", "LOG"]:
            resp = "LOGARITHMIC"
        return resp

    def _correct_dark_run_background(self, reducer, workspace_name, trans_det_ids):
        """
        Subtract the dark run from the transmission workspace
        @param reducer: the reducer object
        @param workspace_name: the name of the workspace to correct
        @param trans_det_ids: the transmission detector ids
        """
        if reducer.dark_run_subtraction.has_dark_runs():
            # We need to grab the workspace from the ADS and place the corrected workspace back into the ADS
            trans_ws = mtd[workspace_name]
            trans_ws = reducer.dark_run_subtraction.execute_transmission(trans_ws, trans_det_ids)
            mtd.addOrReplace(workspace_name, trans_ws)

    def set_prompt_parameter_if_not_set(self, reducer):
        """
        This method sets default prompt peak values in case the user has not provided some. Currently
        we only use default values for LOQ.
        """
        is_prompt_peak_not_set = self.removePromptPeakMin is None and self.removePromptPeakMax is None
        if is_prompt_peak_not_set:
            if reducer.instrument.name() == "LOQ":
                self.removePromptPeakMin = 19000.0  # Units of micro-seconds
                self.removePromptPeakMax = 20500.0  # Units of micro-seconds


class AbsoluteUnitsISIS(ReductionStep):
    DEFAULT_SCALING = 100.0

    def __init__(self):
        # Scaling values [%]
        self.rescale = self.DEFAULT_SCALING

    def execute(self, reducer, workspace):
        scalefactor = self.rescale
        # Data reduced with Mantid is a factor of ~pi higher than Colette.
        # For LOQ only, divide by this until we understand why.
        if reducer.instrument.name() == "LOQ":
            rescaleToColette = math.pi
            scalefactor /= rescaleToColette

        _ws = mtd[workspace]
        _ws *= scalefactor


class CalculateNormISIS(object):
    """
    Note this is not a reduction step, see CalculateNorm

    Generates the normalization workspaces required by Q1D and Qxy for normalization
    produced by other, sometimes optional, reduction_steps or a specified
    workspace
    """

    TMP_ISIS_NAME = "__CalculateNormISIS_loaded_tmp"
    TMP_WORKSPACE_NAME = "__CalculateNorm_loaded_temp"
    WAVE_CORR_NAME = "__Q_WAVE_conversion_temp"
    PIXEL_CORR_NAME = "__Q_pixel_conversion_temp"

    def __init__(self, wavelength_deps=None):
        if wavelength_deps is None:
            wavelength_deps = []
        super(CalculateNormISIS, self).__init__()
        self._wave_steps = wavelength_deps
        self._high_angle_pixel_file = ""
        self._low_angle_pixel_file = ""
        self._pixel_file = ""

    def setPixelCorrFile(self, filename, detector=""):
        """
        For compatibility reason, it still uses the self._pixel_file,
        but, now, we need pixel_file (flood file) for both detectors.
        so, an extra parameter is allowed.

        """
        detector = detector.upper()

        if detector in ("FRONT", "HAB", "FRONT-DETECTOR-BANK"):
            self._high_angle_pixel_file = filename
        if detector in ("REAR", "MAIN", "", "MAIN-DETECTOR-BANK", "DETECTORBENCH"):
            self._low_angle_pixel_file = filename

    def getPixelCorrFile(self, detector):
        """
        For compatibility reason, it still uses the self._pixel_file,
        but, now, we need pixel_file (flood file) for both detectors.
        so, an extra parameter is allowed.

        """
        detector = detector.upper()
        if detector in ("FRONT", "HAB", "FRONT-DETECTOR-BANK", "FRONT-DETECTOR"):
            return self._high_angle_pixel_file
        elif detector in ("REAR", "MAIN", "MAIN-DETECTOR-BANK", "", "REAR-DETECTOR", "DETECTORBENCH"):
            return self._low_angle_pixel_file
        else:
            logger.warning("Request of pixel correction file with unknown detector (" + str(detector) + ")")
            return self._pixel_file

    def _multiplyAll(self, wave_wksps, wksp2match):
        wave_adj = None
        for wksp in wave_wksps:
            # before the workspaces can be combined they all need to match
            RebinToWorkspace(WorkspaceToRebin=wksp, WorkspaceToMatch=wksp2match, OutputWorkspace=self.TMP_WORKSPACE_NAME)

            if not wave_adj:
                wave_adj = self.WAVE_CORR_NAME
                RenameWorkspace(InputWorkspace=self.TMP_WORKSPACE_NAME, OutputWorkspace=wave_adj)
            else:
                Multiply(LHSWorkspace=self.TMP_WORKSPACE_NAME, RHSWorkspace=wave_adj, OutputWorkspace=wave_adj)
        return wave_adj

    def _loadPixelCorrection(self):
        """
        Reads in a pixel correction file if one has been specified.

        @return the name of the workspace, else an empty string if there was no
                correction file.
        """
        if self._pixel_file:
            LoadRKH(Filename=self._pixel_file, OutputWorkspace=self.PIXEL_CORR_NAME, FirstColumnValue="SpectrumNumber")
            return self.PIXEL_CORR_NAME
        return ""

    def _is_point_data(self, wksp):
        """
        Tests if the workspace whose name is passed contains point or histogram data
        The test is if the X and Y array lengths are the same = True, different = false
        @param wksp: name of the workspace to test
        @return True for point data, false for histogram
        """
        handle = mtd[wksp]
        if len(handle.readX(0)) == len(handle.readY(0)):
            return True
        else:
            return False

    def calculate(self, reducer, wave_wks=None):
        """
        Multiplies all the wavelength scalings into one workspace and all the detector
        dependent scalings into another workspace that can be used by ConvertToQ
        @param reducer: settings used for this reduction
        @param wave_wks: additional wavelength dependent correction workspaces to include
        """
        if wave_wks is None:
            wave_wks = []
        # use the instrument's correction file
        corr_file = reducer.instrument.cur_detector().correction_file
        if corr_file:
            LoadRKH(Filename=corr_file, OutputWorkspace=self.TMP_ISIS_NAME, FirstColumnValue="Wavelength")
            wave_wks.append(self.TMP_ISIS_NAME)

            if self._is_point_data(self.TMP_ISIS_NAME):
                ConvertToHistogram(InputWorkspace=self.TMP_ISIS_NAME, OutputWorkspace=self.TMP_ISIS_NAME)
        ## try to redefine self._pixel_file to pass to CalculateNORM method calculate.
        detect_pixel_file = self.getPixelCorrFile(reducer.instrument.cur_detector().name())
        if detect_pixel_file != "":
            self._pixel_file = detect_pixel_file

        for step in self._wave_steps:
            if step.output_wksp:
                wave_wks.append(step.output_wksp)

        wave_adj = self._multiplyAll(wave_wks, reducer.output_wksp)

        pixel_adj = self._loadPixelCorrection()

        if pixel_adj:
            # remove all the pixels that are not present in the sample data (the other detector)
            reducer.instrument.cur_detector().crop_to_detector(pixel_adj, pixel_adj)

        reducer.deleteWorkspaces([self.TMP_ISIS_NAME, self.TMP_WORKSPACE_NAME])

        return wave_adj, pixel_adj


class ConvertToQISIS(ReductionStep):
    """
    Runs the Q1D or Qxy algorithms to convert wavelength data into momentum transfer, Q

    Currently, this allows the wide angle transmission correction.
    """

    # the list of possible Q conversion algorithms to use
    _OUTPUT_TYPES = {"1D": "Q1D", "2D": "Qxy"}
    # defines if Q1D should correct for gravity by default
    _DEFAULT_GRAV = False
    _DEFAULT_EXTRA_LENGTH = 0.0

    def __init__(self, normalizations):
        """
        @param normalizations: CalculateNormISIS object contains the workspace, ReductionSteps or files require
        for the optional normalization arguments
        """
        if not issubclass(normalizations.__class__, CalculateNormISIS):
            raise RuntimeError("Error initializing ConvertToQ, invalid normalization object")
        # contains the normalization optional workspaces to pass to the Q algorithm
        self._norms = normalizations

        # this should be set to 1D or 2D
        self._output_type = "1D"
        # the algorithm that corresponds to the above choice
        self._Q_alg = self._OUTPUT_TYPES[self._output_type]
        # if true gravity is taken into account in the Q1D calculation
        self._use_gravity = self._DEFAULT_GRAV
        # used to implement a default setting for gravity that can be over written but doesn't over write
        self._grav_set = False
        # can be used to add an additional length to the neutron path during the correction for gravity in the Q calcuation
        self._grav_extra_length = self._DEFAULT_EXTRA_LENGTH
        # used to implement a default setting for extra length for gravity; seee _grav_set
        self._grav_extra_length_set = False
        # this should contain the rebin parameters
        self.binning = None

        # The minimum distance in metres from the beam center at which all wavelengths are used in the calculation
        self.r_cut = 0.0
        # The shortest wavelength in angstrom at which counts should be summed from all detector pixels in Angstrom
        self.w_cut = 0.0
        # Whether to output parts when running either Q1D2 or Qxy
        self.outputParts = False
        # Flag if a QResolution workspace should be used
        self.use_q_resolution = False
        # QResolution settings
        self._q_resolution_moderator_file_name = None
        self._q_resolution_delta_r = None
        self._q_resolution_a1 = None
        self._q_resolution_a2 = None
        self._q_resolution_h1 = None
        self._q_resolution_w1 = None
        self._q_resolution_h2 = None
        self._q_resolution_w2 = None
        self._q_resolution_collimation_length = None

    def set_output_type(self, descript):
        """
        Requests the given output from the Q conversion, either 1D or 2D. For
        the 1D calculation it asks the reducer to keep a workspace for error
        estimates
        @param descript: 1D or 2D
        """
        self._Q_alg = self._OUTPUT_TYPES[descript]
        self._output_type = descript

    def get_output_type(self):
        return self._output_type

    output_type = property(get_output_type, set_output_type, None, None)

    def get_gravity(self):
        return self._use_gravity

    def set_gravity(self, flag, override=True):
        """
        Enable or disable including gravity when calculating Q
        @param flag: set to True to enable the gravity correction
        @param override: over write the setting from a previous call to this method (default is True)
        """
        if override:
            self._grav_set = True

        if (not self._grav_set) or override:
            self._use_gravity = bool(flag)
        else:
            msg = "User file can't override previous gravity setting, do gravity correction remains " + str(self._use_gravity)
            print(msg)
            sanslog.warning(msg)

    def get_extra_length(self):
        return self._grav_extra_length

    def set_extra_length(self, extra_length, override=True):
        """
        Add extra length when correcting for gravity when calculating Q
        @param extra_length : additional length for the gravity correction during the calculation of Q
        @param override: over write the setting from a previous call to this method (default is True).
                         This was added because of the way _set_gravity is layed out.
        """
        if override:
            self._grav_extra_length_set = True

        if (not self._grav_extra_length_set) or override:
            self._grav_extra_length = extra_length
        else:
            msg = (
                "User file can't override previous extra length setting for"
                + " gravity correction; extra length remains "
                + str(self._grav_extra_length)
            )
            print(msg)
            sanslog.warning(msg)

    def execute(self, reducer, workspace):
        """
        Calculate the normalization workspaces and then call the chosen Q conversion algorithm.
        """
        wavepixeladj = ""
        if reducer.wide_angle_correction and reducer.transmission_calculator.output_wksp:
            # calculate the transmission wide angle correction
            _issueWarning("sans solid angle correction execution")
            SANSWideAngleCorrection(
                SampleData=workspace, TransmissionData=reducer.transmission_calculator.output_wksp, OutputWorkspace="transmissionWorkspace"
            )
            wavepixeladj = "transmissionWorkspace"
        # create normalization workspaces
        if self._norms:
            # the empty list at the end appears to be needed (the system test SANS2DWaveloops) is this a bug in Python?
            wave_adj, pixel_adj = self._norms.calculate(reducer, [])
        else:
            raise RuntimeError("Normalization workspaces must be created by CalculateNorm() and passed to this step")

        # Create the QResolution workspace, but only if it A) is requested by the user and does not exist
        #                                                  B) is requested by the user, exists, but does not
        #                                                     have the correct binning --> This is currently not implemented,
        #                                                     but should be addressed in an optimization step
        qResolution = self._get_q_resolution_workspace(det_bank_workspace=workspace)

        # Debug output
        if DEBUG:
            sanslog.warning("###############################################")
            sanslog.warning("File : %s" % str(self._q_resolution_moderator_file_name))
            sanslog.warning("A1 : %s" % str(self._q_resolution_a1))
            sanslog.warning("A2 : %s" % str(self._q_resolution_a2))
            sanslog.warning("H1 : %s" % str(self._q_resolution_h1))
            sanslog.warning("H2 : %s" % str(self._q_resolution_h1))
            sanslog.warning("W1 : %s" % str(self._q_resolution_w1))
            sanslog.warning("W2 : %s" % str(self._q_resolution_w2))
            sanslog.warning("LCol: %s" % str(self._q_resolution_collimation_length))
            sanslog.warning("DR : %s" % str(self._q_resolution_delta_r))
            sanslog.warning("Exists: %s" % str(qResolution is not None))

        try:
            if self._Q_alg == "Q1D":
                Q1D(
                    DetBankWorkspace=workspace,
                    OutputWorkspace=workspace,
                    OutputBinning=self.binning,
                    WavelengthAdj=wave_adj,
                    PixelAdj=pixel_adj,
                    AccountForGravity=self._use_gravity,
                    RadiusCut=self.r_cut * 1000.0,
                    WaveCut=self.w_cut,
                    OutputParts=self.outputParts,
                    WavePixelAdj=wavepixeladj,
                    ExtraLength=self._grav_extra_length,
                    QResolution=qResolution,
                )
            elif self._Q_alg == "Qxy":
                Qxy(
                    InputWorkspace=workspace,
                    OutputWorkspace=workspace,
                    MaxQxy=reducer.QXY2,
                    DeltaQ=reducer.DQXY,
                    WavelengthAdj=wave_adj,
                    PixelAdj=pixel_adj,
                    AccountForGravity=self._use_gravity,
                    RadiusCut=self.r_cut * 1000.0,
                    WaveCut=self.w_cut,
                    OutputParts=self.outputParts,
                    ExtraLength=self._grav_extra_length,
                )
                ReplaceSpecialValues(
                    InputWorkspace=workspace,
                    OutputWorkspace=workspace,
                    NaNValue="0",
                    InfinityValue="0",
                    UseAbsolute=False,
                    SmallNumberThreshold=0.0,
                    SmallNumberValue=0.0,
                    SmallNumberError=0.0,
                )
                # We need to correct for special values in the partial outputs. The
                # counts seem to have NANS.
                if self.outputParts:
                    sum_of_counts = workspace + "_sumOfCounts"
                    sum_of_norm = workspace + "_sumOfNormFactors"
                    ReplaceSpecialValues(
                        InputWorkspace=sum_of_counts,
                        OutputWorkspace=sum_of_counts,
                        NaNValue="0",
                        InfinityValue="0",
                        UseAbsolute=False,
                        SmallNumberThreshold=0.0,
                        SmallNumberValue=0.0,
                        SmallNumberError=0.0,
                    )
                    ReplaceSpecialValues(
                        InputWorkspace=sum_of_norm,
                        OutputWorkspace=sum_of_norm,
                        NaNValue="0",
                        InfinityValue="0",
                        UseAbsolute=False,
                        SmallNumberThreshold=0.0,
                        SmallNumberValue=0.0,
                        SmallNumberError=0.0,
                    )
            else:
                raise NotImplementedError("The type of Q reduction has not been set, e.g. 1D or 2D")
        except:
            # when we are all up to Python 2.5 replace the duplicated code below with one finally:
            reducer.deleteWorkspaces([wave_adj, pixel_adj, wavepixeladj])
            raise

        reducer.deleteWorkspaces([wave_adj, pixel_adj, wavepixeladj])

    def _get_q_resolution_workspace(self, det_bank_workspace):
        """
        Calculates the QResolution workspace if this is required
        @param det_bank_workspace: the main workspace which is being reduced
        @returns the QResolution workspace or None
        """
        # Check if the a calculation is asked for by the user
        if self.use_q_resolution is False:
            return None

        # Make sure that all parameters that are needed are available
        self._set_up_q_resolution_parameters()

        # Run a consistency check
        try:
            self.run_consistency_check()
        except RuntimeError as details:
            sanslog.warning(
                "ConverToQISIS: There was something wrong with the Q Resolution"
                " settings. Running the reduction without the Q Resolution"
                " Setting. See details %s" % str(details)
            )
            return None

        # Check if Q Resolution exists in mtd
        exists = mtd.doesExist(QRESOLUTION_WORKSPACE_NAME)

        # Future improvement here: If the binning has not changed and the instrument is
        # the same then we can reuse the existing QResolution workspace if it exists
        if exists:
            # return self._get_existing_q_resolution(det_bank_workspace)
            return self._create_q_resolution(det_bank_workspace=det_bank_workspace)
        else:
            return self._create_q_resolution(det_bank_workspace=det_bank_workspace)

    def _create_q_resolution(self, det_bank_workspace):
        """
        Creates the Q Resolution workspace
        @returns the q resolution workspace
        """
        sigma_moderator = self._get_sigma_moderator_workspace()

        # We need the radius, not the diameter in the TOFSANSResolutionByPixel algorithm
        sample_radius = 0.5 * self.get_q_resolution_a2()
        source_radius = 0.5 * self.get_q_resolution_a1()

        # The radii and the deltaR are expected to be in mm
        TOFSANSResolutionByPixel(
            InputWorkspace=det_bank_workspace,
            OutputWorkspace=QRESOLUTION_WORKSPACE_NAME,
            DeltaR=self.get_q_resolution_delta_r() * 1000.0,
            SampleApertureRadius=sample_radius * 1000.0,
            SourceApertureRadius=source_radius * 1000.0,
            SigmaModerator=sigma_moderator,
            CollimationLength=self.get_q_resolution_collimation_length(),
            AccountForGravity=self._use_gravity,
            ExtraLength=self._grav_extra_length,
        )

        if not mtd.doesExist(QRESOLUTION_WORKSPACE_NAME):
            raise RuntimeError("ConvertTpQIsis: Could not create the q resolution workspace")

        DeleteWorkspace(sigma_moderator)
        return mtd[QRESOLUTION_WORKSPACE_NAME]

    def _get_sigma_moderator_workspace(self):
        """
        Gets the sigma moderator workspace.
        @returns the sigma moderator workspace
        """
        moderator_ws = LoadRKH(Filename=self.get_q_resolution_moderator(), FirstColumnValue="Wavelength")
        moderator_histogram_ws = ConvertToHistogram(InputWorkspace=moderator_ws)
        DeleteWorkspace(moderator_ws)
        return moderator_histogram_ws

    def _get_existing_q_resolution(self, det_bank_workspace):
        """
        If the existing Q Resolution workspace has the correct binning,
        then we use it, else we have to create it
        @det_bank_workspace: the main workspace
        """
        if self._has_matching_binning(det_bank_workspace):
            return mtd[QRESOLUTION_WORKSPACE_NAME]
        else:
            return self._create_q_resolution(det_bank_workspace=det_bank_workspace)

    def _has_matching_binning(self, det_bank_workspace):
        """
        Check if the binning of the q resolution workspace
        and the main workspace do not match
        @det_bank_workspace: the main workspace
        """
        # Here we need to check if the binning has changed, ie if the
        # existing
        raise RuntimeError("The QResolution optimization has not been implemented yet")

    def set_q_resolution_moderator(self, file_name):
        """
        Sets the moderator file name for Q Resolution
        @param file_name: the name of the moderator file
        """
        try:
            q_res_file_path, dummy_suggested_name = getFileAndName(file_name)
        except:
            raise RuntimeError("Invalid input for mask file. (%s)" % str(file_name))
        q_res_file_path = q_res_file_path.replace("\\", "/")
        self._q_resolution_moderator_file_name = q_res_file_path

    def get_q_resolution_moderator(self):
        return self._q_resolution_moderator_file_name

    def set_q_resolution_a1(self, a1):
        self._q_resolution_a1 = a1

    def get_q_resolution_a1(self):
        return self._q_resolution_a1

    def set_q_resolution_a2(self, a2):
        self._q_resolution_a2 = a2

    def get_q_resolution_a2(self):
        return self._q_resolution_a2

    def set_q_resolution_delta_r(self, delta_r):
        self._q_resolution_delta_r = delta_r

    def get_q_resolution_delta_r(self):
        return self._q_resolution_delta_r

    def set_q_resolution_h1(self, h1):
        self._q_resolution_h1 = h1

    def get_q_resolution_h1(self):
        return self._q_resolution_h1

    def set_q_resolution_h2(self, h2):
        self._q_resolution_h2 = h2

    def get_q_resolution_h2(self):
        return self._q_resolution_h2

    def set_q_resolution_w1(self, w1):
        self._q_resolution_w1 = w1

    def get_q_resolution_w1(self):
        return self._q_resolution_w1

    def set_q_resolution_w2(self, w2):
        self._q_resolution_w2 = w2

    def get_q_resolution_w2(self):
        return self._q_resolution_w2

    def set_q_resolution_collimation_length(self, collimation_length):
        self._q_resolution_collimation_length = collimation_length

    def get_q_resolution_collimation_length(self):
        return self._q_resolution_collimation_length

    def set_use_q_resolution(self, enabled):
        self.use_q_resolution = enabled

    def get_use_q_resolution(self):
        return self.use_q_resolution

    def run_consistency_check(self):
        """
        Provides the consistency check for the ConvertTOQISIS
        """
        # Make sure that everything for the QResolution calculation is setup correctly
        if self.use_q_resolution:
            self._check_q_settings_complete()

    def _check_q_settings_complete(self):
        """
        Check that the q resolution settings are complete.
        We need a moderator file path. And the other settings have to be self consistent
        """
        try:
            _dummy_file_path, _dummy_suggested_name = getFileAndName(self._q_resolution_moderator_file_name)
        except:
            raise RuntimeError("The specified moderator file is not valid. Please make sure that that it exists in your search directory.")

        # If A1 is set, then A2 should be set and vice versa
        if (self.get_q_resolution_a1() is None and self.get_q_resolution_a2() is not None) or (
            self.get_q_resolution_a2() is None and self.get_q_resolution_a1() is not None
        ):
            raise RuntimeError("Both, A1 and A2, need to be specified.")

    def _set_up_q_resolution_parameters(self):
        """
        Prepare the parameters which need preparing
        """
        # If we have values for H1 and W1 then set A1 to the correct value
        if self._q_resolution_h1 and self._q_resolution_w1 and self._q_resolution_h2 and self._q_resolution_w2:
            self._q_resolution_a1 = self._set_up_diameter(self._q_resolution_h1, self._q_resolution_w1)
            self._q_resolution_a2 = self._set_up_diameter(self._q_resolution_h2, self._q_resolution_w2)

    def _set_up_diameter(self, h, w):
        """
        Prepare the diameter parameter. If there are corresponding H and W values, then
        use them instead. Richard provided the formula: A = 2*sqrt((H^2 + W^2)/6)
        @param h: the height
        @param w: the width
        @returns the new diameter
        """
        return 2 * math.sqrt((h * h + w * w) / 6)

    def reset_q_settings(self):
        """
        Reset of the q resolution settings
        """
        self.use_q_resolution = False
        self._q_resolution_moderator_file_name = None
        self._q_resolution_delta_r = None
        self._q_resolution_a1 = None
        self._q_resolution_a2 = None
        self._q_resolution_h1 = None
        self._q_resolution_w1 = None
        self._q_resolution_h2 = None
        self._q_resolution_w2 = None
        self._q_resolution_collimation_length = None


class UnitsConvert(ReductionStep):
    """
    Executes ConvertUnits and then Rebin on the same workspace. If no re-bin limits are
    set for the x-values of the final workspace the range of the first spectrum is used.
    """

    def __init__(self, units, rebin="Rebin", bin_alg=None):
        """
        @param bin_alg: the name of the Mantid re-bin algorithm to use
        """
        super(UnitsConvert, self).__init__()
        self._units = units
        self.wav_low = None
        self.wav_high = None
        self.wav_step = None
        # currently there are two possible re-bin algorithms, the other is InterpolatingRebin
        self.rebin_alg = rebin
        self._bin_alg = bin_alg

    # TODO: consider how to remove the extra argument after workspace
    def execute(self, reducer, workspace, bin_alg=None):
        """
        Runs the ConvertUnits() and a rebin algorithm on the specified
        workspace
        @param reducer:
        @param workspace: the name of the workspace to convert
        @param workspace: the name of the workspace to convert
        """
        ConvertUnits(InputWorkspace=workspace, OutputWorkspace=workspace, Target=self._units)

        low_wav = self.wav_low
        high_wav = self.wav_high

        if low_wav is None and high_wav is None:
            low_wav = min(mtd[workspace].readX(0))
            high_wav = max(mtd[workspace].readX(0))

        if not bin_alg:
            bin_alg = self.rebin_alg

        rebin_com = bin_alg + '(workspace, "' + self._get_rebin(low_wav, self.wav_step, high_wav) + '", OutputWorkspace=workspace)'
        eval(rebin_com)

    def _get_rebin(self, low, step, high):
        """
        Convert the range limits and step into a form passable to re-bin
        @param low: first number in the Rebin string, the first bin boundary
        @param step: bin width
        @param high: high bin boundary
        """
        return str(low) + ", " + str(step) + ", " + str(high)

    def get_rebin(self):
        """
        Get the string that is passed as the "param" property to Rebin
        @return the string that is passed to Rebin
        """
        return self._get_rebin(self.wav_low, self.wav_step, self.wav_high)

    def set_rebin(self, w_low=None, w_step=None, w_high=None, override=True):
        """
        Set the parameters that are passed to Rebin
        @param w_low: first number in the Rebin string, the first bin boundary
        @param w_step: bin width
        @param w_high: high bin boundary
        """
        if w_low is not None:
            if self.wav_low is None or override:
                self.wav_low = float(w_low)
        if w_step is not None:
            if self.wav_step is None or override:
                self.wav_step = float(w_step)
        if w_high is not None:
            if self.wav_high is None or override:
                self.wav_high = float(w_high)

    def get_range(self):
        """
        Get the values of the highest and lowest boundaries
        @return low'_'high
        """
        return str(self.wav_low) + "_" + str(self.wav_high)

    def set_range(self, w_low=None, w_high=None):
        """
        Set the highest and lowest bin boundary values
        @param w_low: first number in the Rebin string, the first bin boundary
        @param w_high: high bin boundary
        """
        self.set_rebin(w_low, None, w_high)

    def __str__(self):
        return "    Wavelength range: " + self.get_rebin()


class SliceEvent(ReductionStep):
    def __init__(self):
        super(SliceEvent, self).__init__()
        self.scale = 1

    def execute(self, reducer, workspace):
        ws_pointer = getWorkspaceReference(workspace)

        # it applies only for event workspace
        if not isinstance(ws_pointer, IEventWorkspace):
            self.scale = 1
            return

        # If a sample data set is converted then we want to be able to slice
        # If a can data set is being converted, the slice limits should not be applied
        # but rather the full data set should be used. -1 is the no limit signal
        if not reducer.is_can():
            start, stop = reducer.getCurrSliceLimit()
        else:
            start = -1
            stop = -1

        _monitor = reducer.get_sample().get_monitor()

        if "events.binning" in reducer.settings:
            binning = reducer.settings["events.binning"]
        else:
            binning = ""
        _hist, (_tot_t, tot_c, _part_t, part_c) = slice2histogram(ws_pointer, start, stop, _monitor, binning)
        self.scale = part_c / tot_c


class BaseBeamFinder(ReductionStep):
    """
    Base beam finder. Holds the position of the beam center
    and the algorithm for calculates it using the beam's
    displacement under gravity
    """

    def __init__(self, beam_center_x=None, beam_center_y=None):
        """
        Initial beam center is given in pixel coordinates
        @param beam_center_x: pixel position of the beam in x
        @param beam_center_y: pixel position of the beam in y
        """
        super(BaseBeamFinder, self).__init__()
        self._beam_center_x = beam_center_x
        self._beam_center_y = beam_center_y
        self._beam_radius = None
        self._datafile = None
        self._persistent = True

    def set_persistent(self, persistent):
        self._persistent = persistent
        return self

    def get_beam_center(self):
        """
        Returns the beam center
        """
        return [self._beam_center_x, self._beam_center_y]

    def execute(self, reducer, workspace=None):
        return "Beam Center set at: %s %s" % (str(self._beam_center_x), str(self._beam_center_y))

    def update_beam_center(self, beam_center_x, beam_center_y):
        """
        Update the beam center position of the BeamBaseFinder
        @param beam_center_x: The first position
        @param beam_center_y: The second position
        """
        self._beam_center_x = beam_center_x
        self._beam_center_y = beam_center_y


class UserFile(ReductionStep):
    """
    Reads an ISIS SANS mask file of the format described here mantidproject.org/SANS_User_File_Commands
    """

    def __init__(self, file=None):
        """
        Optionally sets the location of the file and initialise the reader
        """
        super(UserFile, self).__init__()
        self.filename = file
        self._incid_monitor_lckd = False
        self.executed = False

        # maps the keywords that the file can contains to the functions that read them
        self.key_functions = {
            "BACK/": self._read_back_line,
            "TRANS/": self._read_trans_line,
            "MON/": self._read_mon_line,
            "TUBECALIBFILE": self._read_calibfile_line,
            "MASKFILE": self._read_maskfile_line,
            "QRESOL/": self._read_q_resolution_line,
            "UNWRAP": self._read_unwrap_monitors_line,
        }

    def __deepcopy__(self, memo):
        """Called when a deep copy is requested"""
        fresh = UserFile(self.filename)
        fresh._incid_monitor_lckd = self._incid_monitor_lckd
        fresh.executed = self.executed
        fresh.key_functions = {
            "BACK/": fresh._read_back_line,
            "TRANS/": fresh._read_trans_line,
            "MON/": fresh._read_mon_line,
            "TUBECALIBFILE": self._read_calibfile_line,
            "MASKFILE": self._read_maskfile_line,
            "QRESOL/": self._read_q_resolution_line,
            "UNWRAP": self._read_unwrap_monitors_line,
        }
        return fresh

    def execute(self, reducer, workspace=None):
        if self.filename is None:
            raise AttributeError("The user file must be set, use the function MaskFile")
        user_file = self.filename

        # Check that the format is valid, ie txt or 099AA else raise
        if not is_valid_user_file_extension(user_file):
            raise RuntimeError("UseFile: The user file does not seem to be of the correct file type.")

        # Check that the file exists.
        if not os.path.isfile(user_file):
            user_file = os.path.join(reducer.user_file_path, self.filename)
            if not os.path.isfile(user_file):
                user_file = FileFinder.getFullPath(self.filename)
                if not os.path.isfile(user_file):
                    raise RuntimeError("Cannot read mask. File path '%s' does not exist or is not in the user path." % self.filename)

        reducer.user_file_path = os.path.dirname(user_file)
        # Re-initializes default values
        self._initialize_mask(reducer)
        reducer.prep_normalize.setPixelCorrFile("", "REAR")
        reducer.prep_normalize.setPixelCorrFile("", "FRONT")

        file_handle = open(user_file, "r")
        for line in file_handle:
            try:
                self.read_line(line, reducer)
            except IOError:
                # Close the handle
                file_handle.close()
                raise RuntimeError(
                    "%s was specified in the MASK file (%s) but the file cannot be found." % (line.rsplit()[0], file_handle.name)
                )

        # Check if one of the efficiency files hasn't been set and assume the other is to be used
        reducer.instrument.copy_correction_files()

        # Run a consistency check
        reducer.perform_consistency_check()

        self.executed = True
        return self.executed

    def read_line(self, line, reducer):  # noqa: C901
        # This is so that I can be sure all EOL characters have been removed
        line = line.lstrip().rstrip()
        upper_line = line.upper()

        # check for a recognised command
        for keyword in list(self.key_functions.keys()):
            if upper_line.startswith(keyword):
                # remove the keyword as it has already been parsed
                params = line[len(keyword) :]
                # call the handling function for that keyword
                error = self.key_functions[keyword](params, reducer)

                if error:
                    _issueWarning(error + line)

                return

        if upper_line.startswith("L/"):
            self.readLimitValues(line, reducer)

        elif upper_line.startswith("MASK"):
            if len(upper_line[5:].strip().split()) == 4:
                _issueInfo('Box masks can only be defined using the V and H syntax, not "mask x1 y1 x2 y2"')
            else:
                reducer.mask.parse_instruction(reducer.instrument.name(), upper_line)

        elif upper_line.startswith("SET CENTRE"):
            # SET CENTRE accepts the following properties:
            # SET CENTRE X Y
            # SET CENTRE/MAIN X Y
            # SET CENTRE/HAB X Y
            main_str_pos = upper_line.find("MAIN")
            hab_str_pos = upper_line.find("HAB")
            x_pos = 0.0
            y_pos = 0.0
            # use the scale factors supplied in the parameter file
            XSF = reducer.inst.beam_centre_scale_factor1
            YSF = reducer.inst.beam_centre_scale_factor2

            if main_str_pos > 0:
                values = upper_line[main_str_pos + 5 :].split()  # remov the SET CENTRE/MAIN
                x_pos = float(values[0]) / XSF
                y_pos = float(values[1]) / YSF
            elif hab_str_pos > 0:
                values = upper_line[hab_str_pos + 4 :].split()  # remove the SET CENTRE/HAB
                print(" convert values ", values)
                x_pos = float(values[0]) / XSF
                y_pos = float(values[1]) / YSF
            else:
                values = upper_line.split()
                x_pos = float(values[2]) / XSF
                y_pos = float(values[3]) / YSF
            if hab_str_pos > 0:
                print("Front values = ", x_pos, y_pos)
                reducer.set_beam_finder(BaseBeamFinder(x_pos, y_pos), "front")
            else:
                reducer.set_beam_finder(BaseBeamFinder(x_pos, y_pos))

        elif upper_line.startswith("SET SCALES"):
            values = upper_line.split()
            reducer._corr_and_scale.rescale = float(values[2]) * reducer._corr_and_scale.DEFAULT_SCALING

        elif upper_line.startswith("SAMPLE/OFFSET"):
            values = upper_line.split()
            reducer.instrument.set_sample_offset(values[1])

        elif upper_line.startswith("DET/"):
            det_specif = upper_line[4:]
            if det_specif.startswith("CORR"):
                self._readDetectorCorrections(upper_line[8:], reducer)
            elif det_specif.startswith("RESCALE") or det_specif.startswith("SHIFT"):
                self._readFrontRescaleShiftSetup(det_specif, reducer)
            elif any(it == det_specif.strip() for it in ["FRONT", "REAR", "BOTH", "MERGE", "MERGED", "MAIN", "HAB"]):
                # for /DET/FRONT, /DET/REAR, /DET/BOTH, /DET/MERGE and /DET/MERGED commands
                # we also accommodate DET/MAIN and DET/HAB here which are specifically for LOQ
                det_specif = det_specif.strip()
                if det_specif == "MERGE":
                    det_specif = "MERGED"
                reducer.instrument.setDetector(det_specif)
            elif det_specif.startswith("OVERLAP"):
                self.readFrontMergeRange(det_specif, reducer)
            else:
                _issueWarning("Incorrectly formatted DET line, %s, line ignored" % upper_line)

        # There are two entries for Gravity: 1. ON/OFF (TRUE/FALSE)
        #                                    2. LEXTRA=xx.xx
        elif upper_line.startswith("GRAVITY"):
            grav = upper_line[8:].strip()
            if grav == "ON" or grav == "TRUE":
                reducer.to_Q.set_gravity(True, override=False)
            elif grav == "OFF" or grav == "FALSE":
                reducer.to_Q.set_gravity(False, override=False)
            elif grav.startswith("LEXTRA"):
                extra_length = grav[7:].strip()
                reducer.to_Q.set_extra_length(float(extra_length), override=False)
            else:
                _issueWarning("Gravity flag incorrectly specified, disabling gravity correction")
                reducer.to_Q.set_gravity(False, override=False)
                reducer.to_Q.set_extra_length(0.0, override=False)

        elif upper_line.startswith("FIT/TRANS/"):
            # check if the selector is passed:
            selector = "BOTH"
            if "SAMPLE" in upper_line:
                selector = "SAMPLE"
                params = upper_line[17:].split()  # remove FIT/TRANS/SAMPLE/
            elif "CAN" in upper_line:
                selector = "CAN"
                params = upper_line[14:].split()  # remove FIT/TRANS/CAN/
            else:
                params = upper_line[10:].split()  # remove FIT/TRANS/

            try:
                nparams = len(params)
                if nparams == 1:
                    fit_type = params[0]
                    lambdamin = lambdamax = None
                elif nparams == 3:
                    fit_type, lambdamin, lambdamax = params
                else:
                    raise IOError
                reducer.transmission_calculator.set_trans_fit(
                    min_=lambdamin, max_=lambdamax, fit_method=fit_type, override=True, selector=selector
                )
            except IOError:
                _issueWarning("Incorrectly formatted FIT/TRANS line, %s, line ignored" % upper_line)

        elif upper_line.startswith("FIT/MONITOR"):
            params = upper_line.split()
            nparams = len(params)
            if nparams == 3 and is_prompt_peak_instrument(reducer):
                reducer.transmission_calculator.removePromptPeakMin = float(params[1])
                reducer.transmission_calculator.removePromptPeakMax = float(params[2])
            else:
                if reducer.instrument.name() == "LOQ":
                    _issueWarning("Incorrectly formatted FIT/MONITOR line, %s, line ignored" % upper_line)
                else:
                    _issueWarning("FIT/MONITOR line specific to LOQ instrument. Line ignored")

        elif upper_line == "SANS2D" or upper_line == "LOQ" or upper_line == "LARMOR":
            self._check_instrument(upper_line, reducer)

        elif upper_line.startswith("PRINT "):
            _issueInfo(upper_line[6:])

        elif upper_line.startswith("SAMPLE/PATH"):
            flag = upper_line[12:].strip()
            if flag == "ON" or flag == "TRUE":
                reducer.wide_angle_correction = True
            else:
                reducer.wide_angle_correction = False

        elif line.startswith("!") or not line:
            # this is a comment or empty line, these are allowed
            pass

        else:
            _issueWarning("Unrecognized line in user file the line %s, ignoring" % upper_line)

    def _initialize_mask(self, reducer):
        self._restore_defaults(reducer)

        reducer.CENT_FIND_RMIN = None
        reducer.CENT_FIND_RMAX = None

        reducer.QXY = None
        reducer.DQY = None
        reducer.to_Q.r_cut = 0
        reducer.to_Q.w_cut = 0

        reducer._corr_and_scale.rescale = 100.0

    # Read a limit line of a mask file
    def readLimitValues(self, limit_line, reducer):  # noqa: C901
        limits = limit_line.split("L/")
        if len(limits) != 2:
            _issueWarning('Incorrectly formatted limit line ignored "' + limit_line + '"')
            return
        limits = limits[1]
        limit_type = ""

        if limits.startswith("SP "):
            # We don't use the L/SP line
            _issueWarning("L/SP lines are ignored")
            return

        if limits.upper().startswith("Q/RCUT"):
            limits = limits.upper().split("RCUT")
            if len(limits) != 2:
                _issueWarning("Badly formed L/Q/RCUT line")
            else:
                # When read from user file the unit is in mm but stored here it units of meters
                reducer.to_Q.r_cut = float(limits[1]) / 1000.0
            return
        if limits.upper().startswith("Q/WCUT"):
            limits = limits.upper().split("WCUT")
            if len(limits) != 2:
                _issueWarning("Badly formed L/Q/WCUT line")
            else:
                reducer.to_Q.w_cut = float(limits[1])
            return

        rebin_str = None
        if "," not in limit_line:
            # Split with no arguments defaults to any whitespace character and in particular
            # multiple spaces are include
            elements = limits.split()
            if len(elements) == 4:
                limit_type, minval, maxval, step = elements[0], elements[1], elements[2], elements[3]
                step_details = step.split("/")
                if len(step_details) == 2:
                    step_size = step_details[0]
                    step_type = step_details[1]
                    if step_type.upper() == "LOG":
                        step_type = "-"
                    else:
                        step_type = ""
                else:
                    step_size = step_details[0]
                    step_type = ""
            elif len(elements) == 3:
                limit_type, minval, maxval = elements[0], elements[1], elements[2]
            else:
                _issueWarning('Incorrectly formatted limit line ignored "' + limit_line + '"')
                return
        else:
            blocks = limits.split()
            limit_type = blocks[0].lstrip().rstrip()
            try:
                rebin_str = limits.split(limit_type)[1]
            # pylint: disable=bare-except
            except:
                _issueWarning('Incorrectly formatted limit line ignored "' + limit_line + '"')
                return

            minval = maxval = step_type = step_size = None

        if limit_type.upper() == "WAV":
            if rebin_str:
                _issueWarning('General wave re-bin lines are not implemented, line ignored "' + limit_line + '"')
                return
            else:
                reducer.to_wavelen.set_rebin(minval, step_type + step_size, maxval, override=False)
        elif limit_type.upper() == "Q":
            if rebin_str:
                reducer.to_Q.binning = rebin_str
            else:
                reducer.to_Q.binning = minval + "," + step_type + step_size + "," + maxval
        elif limit_type.upper() == "QXY":
            reducer.QXY2 = float(maxval)
            reducer.DQXY = float(step_type + step_size)
        elif limit_type.upper() == "R":
            reducer.mask.set_radi(minval, maxval)
            reducer.CENT_FIND_RMIN = float(minval) / 1000.0
            reducer.CENT_FIND_RMAX = float(maxval) / 1000.0
        elif (limit_type.upper() == "PHI") or (limit_type.upper() == "PHI/NOMIRROR"):
            mirror = limit_type.upper() != "PHI/NOMIRROR"
            if maxval.endswith("/NOMIRROR"):
                maxval = maxval.split("/NOMIRROR")[0]
                mirror = False
            reducer.mask.set_phi_limit(float(minval), float(maxval), mirror, override=False)
        elif limit_type.upper() == "EVENTSTIME":
            if rebin_str:
                reducer.settings["events.binning"] = rebin_str
            else:
                reducer.settings["events.binning"] = minval + "," + step_type + step_size + "," + maxval
        else:
            _issueWarning('Error in user file after L/, "%s" is not a valid limit line' % limit_type.upper())

    def _read_mon_line(self, details, reducer):  # noqa: C901
        # MON/LENGTH, MON/SPECTRUM and MON/TRANS all accept the INTERPOLATE option
        interpolate = False
        interPlace = details.upper().find("/INTERPOLATE")
        if interPlace != -1:
            interpolate = True
            details = details[0:interPlace]

        if details.upper().startswith("SPECTRUM"):
            reducer.set_monitor_spectrum(int(details.split("=")[1]), interpolate, override=False)
            self._incid_monitor_lckd = True

        elif details.upper().startswith("LENGTH"):
            details = details.split("=")[1]
            options = details.split()
            spectrum = int(options[1])
            #            reducer.instrument.monitor_zs[spectrum] = options[0]

            # the settings here are overridden by MON/SPECTRUM
            if not self._incid_monitor_lckd:
                reducer.set_monitor_spectrum(spectrum, interpolate, override=False)

        elif details.upper().startswith("TRANS"):
            parts = details.split("=")
            if len(parts) < 2 or parts[0].upper() != "TRANS/SPECTRUM":
                return "Unable to parse MON/TRANS line, needs MON/TRANS/SPECTRUM=... not: "
            reducer.set_trans_spectrum(int(parts[1]), interpolate, override=False)

        elif "DIRECT" in details.upper() or details.upper().startswith("FLAT"):
            parts = details.split("=")
            if len(parts) == 2:
                filepath = parts[1].rstrip()
                # for VMS compatibility ignore anything in "[]", those are normally VMS drive specifications
                if "[" in filepath:
                    idx = filepath.rfind("]")
                    filepath = filepath[idx + 1 :]
                if not os.path.isabs(filepath):
                    filepath = os.path.join(reducer.user_file_path, filepath)

                # If a filepath has been provided, then it must exist to continue.
                if filepath and not os.path.isfile(filepath):
                    raise RuntimeError("The following MON/DIRECT datafile does not exist: %s" % filepath)

                _type = parts[0]
                parts = _type.split("/")
                if len(parts) == 1:
                    if parts[0].upper() == "DIRECT":
                        reducer.instrument.cur_detector().correction_file = filepath
                        reducer.instrument.other_detector().correction_file = filepath
                    elif parts[0].upper() == "HAB":
                        try:
                            reducer.instrument.getDetector("HAB").correction_file = filepath
                        except AttributeError:
                            raise AttributeError("Detector HAB does not exist for the current instrument, set the instrument to LOQ first")
                    elif parts[0].upper() == "FLAT":
                        reducer.prep_normalize.setPixelCorrFile(filepath, "REAR")
                    else:
                        pass
                elif len(parts) == 2:
                    detname = parts[1]
                    if detname.upper() == "REAR":
                        if parts[0].upper() == "FLAT":
                            reducer.prep_normalize.setPixelCorrFile(filepath, "REAR")
                        else:
                            reducer.instrument.getDetector("REAR").correction_file = filepath
                    elif detname.upper() == "FRONT" or detname.upper() == "HAB":
                        if parts[0].upper() == "FLAT":
                            reducer.prep_normalize.setPixelCorrFile(filepath, "FRONT")
                        else:
                            reducer.instrument.getDetector("FRONT").correction_file = filepath
                    else:
                        return "Incorrect detector specified for efficiency file: "
                else:
                    return "Unable to parse monitor line: "
            else:
                return "Unable to parse monitor line: "
        else:
            return "Unable to parse monitor line: "

    def _readDetectorCorrections(self, details, reducer):
        """
        Handle user commands of the type DET/CORR/FRONT/RADIUS x
        @param details: the contents of the line after DET/CORR
        @param reducer: the object that contains all the settings
        """
        if details[0] == "/":
            details = details.lstrip("/")
        values = details.split()
        if "/" in values[0]:
            # assume notation is e.g. FRONT/RADIUS x
            values2 = values[0].split("/")
            det_name = values2[0]
            det_axis = values2[1]
            shift = float(values[1])
        else:
            # assume notation is e.g. FRONT RADIUS x
            det_name = values[0]
            det_axis = values[1]
            shift = float(values[2])

        detector = reducer.instrument.getDetector(det_name)
        if det_axis == "X":
            detector.x_corr = shift
        elif det_axis == "Y":
            detector.y_corr = shift
        elif det_axis == "Z":
            detector.z_corr = shift
        elif det_axis == "ROT":
            detector.rot_corr = shift
        # 21/3/12 RKH added 2 variables
        elif det_axis == "RADIUS":
            detector.radius_corr = shift
        elif det_axis == "SIDE":
            detector.side_corr = shift
        # 10/03/15 RKH add 2 more variables
        elif det_axis == "XTILT":
            detector.x_tilt = shift
        elif det_axis == "YTILT":
            detector.y_tilt = shift
        else:
            raise NotImplementedError('Detector correction on "' + det_axis + '" is not supported')

    def _readFrontRescaleShiftSetup(self, details, reducer):
        """
        Handle user commands of the type DET/RESCALE r and DET/RESCALE/FIT q1 q2
        which are used to scale+constant background shift front detector so that
        data from the front and rear detectors can be merged

        @param details: the contents of the line after DET/
        @param reducer: the object that contains all the settings
        """
        values = details.split()
        rAnds = reducer.instrument.getDetector("FRONT").rescaleAndShift
        rAnds.qRangeUserSelected = False
        if details.startswith("RESCALE"):
            if "FIT" in details:
                if len(values) == 1:
                    rAnds.fitScale = True
                elif len(values) == 3:
                    rAnds.fitScale = True
                    rAnds.qMin = float(values[1])
                    rAnds.qMax = float(values[2])
                    rAnds.qRangeUserSelected = True
                else:
                    _issueWarning('Command: "DET/' + details + '" not valid. Expected format is /DET/RESCALE/FIT [q1 q2]')
            else:
                if len(values) == 2:
                    rAnds.scale = float(values[1])
                else:
                    _issueWarning('Command: "DET/' + details + '" not valid. Expected format is /DET/RESCALE r')
        elif details.startswith("SHIFT"):
            if "FIT" in details:
                if len(values) == 1:
                    rAnds.fitShift = True
                elif len(values) == 3:
                    rAnds.fitShift = True
                    rAnds.qMin = float(values[1])
                    rAnds.qMax = float(values[2])
                    rAnds.qRangeUserSelected = True
                else:
                    _issueWarning('Command: "DET/' + details + '" not valid. Expected format is /DET/SHIFT/FIT [q1 q2]')
            else:
                if len(values) == 2:
                    rAnds.shift = float(values[1])
                else:
                    _issueWarning('Command: "DET/' + details + '" not valid. Expected format is /DET/RESCALE r')

    def readFrontMergeRange(self, details, reducer):
        """
        Handle user commands of the type DET/OVERLAP [Q1 Q2] which are used to specify the range to merge

        @param details: the contents of the line after DET/
        @param reducer: the object that contains all the settings
        """
        values = details.split()
        rAnds = reducer.instrument.getDetector("FRONT").mergeRange
        rAnds.q_merge_range = False
        if len(values) == 3:
            rAnds.q_merge_range = True
            rAnds.q_min = float(values[1])
            rAnds.q_max = float(values[2])
        else:
            _issueWarning('Command: "DET/' + details + '" not valid. Expected format is /DET/OVERLAP q1 q2')

    def _read_back_line(self, arguments, reducer):
        """
        Parses a line from the settings file
        @param arguments: the contents of the line after the first keyword
        @param reducer: the object that contains all the settings
        @return any errors encountered or ''
        """
        # Check with a BACK/ User file parser if the desired keywords are in here
        # else handle in a standard way
        back_parser = UserFileParser.BackCommandParser()
        if back_parser.can_attempt_to_parse(arguments):
            dark_run_setting = back_parser.parse_and_set(arguments)
            reducer.add_dark_run_setting(dark_run_setting)
        else:
            # a list of the key words this function can read and the functions it calls in response
            keys = ["MON/TIMES", "M", "TRANS"]
            funcs = [self._read_default_back_region, self._read_back_region, self._read_back_trans_roi]
            self._process(keys, funcs, arguments, reducer)

    def _read_back_region(self, arguments, reducer):
        """
        Parses a line of the form BACK/M... to sets the default TOF
        window for the background region for a specific monitor, or
        turning off if of the format BACK/M3/OFF.
        @param arguments: the contents of the line after the first keyword
        @param reducer: the object that contains all the settings
        @return any errors encountered or ''
        """
        try:
            # check first if what to turn of a background for a specific
            # monitor using 'BACK/M2/OFF'.
            parts = arguments.split("/OFF")
            if len(parts) == 2:
                # set specific monitor to OFF
                reducer.inst.set_TOFs(None, None, int(parts[0]))
                return ""

            # assume a line of the form BACK/M1/TIMES
            parts = arguments.split("/TIMES")
            if len(parts) == 2:
                times = parts[1].split()
            else:
                # try the other possibility, something like, BACK/M2
                parts = arguments.split()
                times = [parts[1], parts[2]]

            monitor = int(parts[0])

            # parse the words after 'TIME' as first the start time and then the end
            reducer.inst.set_TOFs(int(times[0]), int(times[1]), monitor)
            return ""
        except Exception as reason:
            # return a description of any problems and then continue to read the next line
            return str(reason) + " on line: "

    def _read_default_back_region(self, arguments, reducer):
        """
        Parses a line of the form BACK/MON/TIMES form and sets the default TOF
        window for the background region assumed for the current instrument
        @param arguments: the contents of the line after the first keyword
        @param reducer: the object that contains all the settings
        @return any errors encountered or ''
        """
        times = arguments.split()
        if len(times) == 2:
            reducer.inst.set_TOFs(int(times[0]), int(times[1]))
        else:
            reducer.inst.set_TOFs(None, None)
            return "Only monitor specific backgrounds will be applied, no default is set due to incorrectly formatted background line:"

    def _read_back_trans_roi(self, arguments, reducer):
        """
        Parses a line of the form BACK/TRANS to set the background for region of interest (ROI) data
        @param arguments: the contents of the line after the first keyword
        @param reducer: the object that contains all the settings
        @return any errors encountered or ''
        """
        try:
            # Get everything after TRANS. This should be two numbers essentially (start and end time)
            arguments.strip()
            times = arguments.split()
            times = [t.strip() for t in times]
            if len(times) == 2:
                reducer.inst.set_TOFs_for_ROI(int(times[0]), int(times[1]))
                return ""
            raise ValueError("Expected two times for BACK/TRANS")
        except ValueError as reason:
            # return a description of any problems and then continue to read the next line
            return str(reason) + " on line: "

    def _read_trans_line(self, arguments, reducer):
        try:
            if arguments.startswith("RADIUS"):
                # Convert the input (mm) into the correct units (m)
                reducer.transmission_calculator.radius = float(arguments.split("=")[1]) / 1000.0
                return
            elif arguments.startswith("ROI"):
                reducer.transmission_calculator.roi_files += [arguments.split("=")[1]]
                return
            elif arguments.startswith("MASK"):
                reducer.transmission_calculator.mask_files += [arguments.split("=")[1]]
                return
        except Exception as e:
            return 'Problem parsing TRANS line "' + arguments + '":\n' + str(e)

        # a list of the key words this function can read and the functions it calls in response
        keys = ["TRANSPEC", "SAMPLEWS", "CANWS"]
        funcs = [self._read_transpec, self._read_trans_samplews, self._read_trans_canws]
        return self._process(keys, funcs, arguments, reducer)

    def _process(self, keys, funcs, params, reducer):
        # go through the list of recognised commands
        for i in range(0, len(keys)):
            if params.startswith(keys[i]):
                # remove the keyword as it has already been parsed
                params = params[len(keys[i]) :]
                # call the handling function for that keyword returning any error
                return funcs[i](params, reducer)
        return "Unrecognised line: "

    def _read_transpec(self, arguments, reducer):
        arguments = arguments.split("/")

        # check if there is an optional shift specification
        if len(arguments) == 2:
            # deal with the shift specification first
            shift = arguments[1]
            terms = shift.split("=")
            if len(terms) < 2:
                return "Bad TRANS/TRANSPEC= / line: "
            reducer.instrument.monitor_4_offset = float(terms[1])

        # now remove any shift specification and parse the first argument
        arguments = arguments[0]
        arguments = arguments.split("=")
        if len(arguments) == 1:
            raise RuntimeError('An "=" is required after TRANSPEC')

        reducer.transmission_calculator.trans_mon = int(arguments[1])

    def _read_trans_samplews(self, arguments, reducer):
        if arguments.find("=") > -1:
            arguments = arguments.split("=")
        else:
            arguments = arguments.split()

        if len(arguments) != 2:
            return "Unrecognised line: "

        reducer.transmission_calculator.calculated_samp = arguments[1]

    def _read_trans_canws(self, arguments, reducer):
        if arguments.find("=") > -1:
            arguments = arguments.split("=")
        else:
            arguments = arguments.split()

        if len(arguments) != 2:
            return "Unrecognised line: "

        reducer.transmission_calculator.calculated_can = arguments[1]

    def _read_q_resolution_line(self, arguments, reducer):
        """
        Parses the input for QResolution
        @param arguments: the arguments of a QResolution line
        @param reducer: a reducer object
        """
        if arguments.find("=") == -1:
            return self._read_q_resolution_line_on_off(arguments, reducer)

        # Split and remove the white spaces
        arguments = arguments.split("=")
        arguments = [element.strip() for element in arguments]

        # Check if it is the moderator file name, if so add it and return
        if arguments[0].upper().startswith("MODERATOR"):
            # pylint: disable=bare-except
            try:
                reducer.to_Q.set_q_resolution_moderator(file_name=arguments[1])
            except:
                sanslog.error(
                    "The specified moderator file could not be found. Please specify a file which exists in the search directories."
                )
            return

        # All arguments need to be convertible to a float
        if not is_convertible_to_float(arguments[1]):
            return "Value not a float in line: "

        # Now check for the actual key
        if arguments[0].startswith("DELTAR"):
            reducer.to_Q.set_q_resolution_delta_r(delta_r=float(arguments[1]) / 1000.0)
        elif arguments[0].startswith("A1"):
            # Input is in mm but we need m later on
            reducer.to_Q.set_q_resolution_a1(a1=float(arguments[1]) / 1000.0)
        elif arguments[0].startswith("A2"):
            # Input is in mm but we need m later on
            reducer.to_Q.set_q_resolution_a2(a2=float(arguments[1]) / 1000.0)
        elif arguments[0].startswith("LCOLLIM"):
            # Input is in m and we need it to be in m later on
            reducer.to_Q.set_q_resolution_collimation_length(collimation_length=float(arguments[1]))
        elif arguments[0].startswith("H1"):
            # Input is in mm but we need m later on
            reducer.to_Q.set_q_resolution_h1(h1=float(arguments[1]) / 1000.0)
        elif arguments[0].startswith("W1"):
            # Input is in mm but we need m later on
            reducer.to_Q.set_q_resolution_w1(w1=float(arguments[1]) / 1000.0)
        elif arguments[0].startswith("H2"):
            # Input is in mm but we need m later on
            reducer.to_Q.set_q_resolution_h2(h2=float(arguments[1]) / 1000.0)
        elif arguments[0].startswith("W2"):
            # Input is in mm but we need m later on
            reducer.to_Q.set_q_resolution_w2(w2=float(arguments[1]) / 1000.0)
        else:
            return "Unrecognised line: "

    def _read_q_resolution_line_on_off(self, arguments, reducer):
        """
        Handles the ON/OFF setting for QResolution
        @param arguments: the line arguments
        @param reducer: a reducer object
        """
        # Remove white space
        on_off = "".join(arguments.split())

        # We expect only ON or OFF
        if on_off == "ON":
            reducer.to_Q.set_use_q_resolution(enabled=True)
        elif on_off == "OFF":
            reducer.to_Q.set_use_q_resolution(enabled=False)
        else:
            return "Unrecognised line: "

    def _check_instrument(self, inst_name, reducer):
        if reducer.instrument is None:
            raise RuntimeError("Use SANS2D() or LOQ() to set the instrument before Maskfile()")
        if inst_name != reducer.instrument.name():
            raise RuntimeError("User settings file not compatible with the selected instrument " + reducer.instrument.name())

    def _restore_defaults(self, reducer):
        reducer.mask.parse_instruction(reducer.instrument.name(), "MASK/CLEAR")
        reducer.mask.parse_instruction(reducer.instrument.name(), "MASK/CLEAR/TIME")

        reducer.CENT_FIND_RMIN = reducer.CENT_FIND_RMAX
        reducer.QXY = None
        reducer.DQY = None

        reducer.to_Q.binning = None

        # Scaling values
        reducer._corr_and_scale.rescale = 100.0  # percent

        reducer.inst.reset_TOFs()

    def _read_calibfile_line(self, arguments, reducer):
        # remove the equals from the beginning and any space around.
        parts = re.split(r"\s?=\s?", arguments)
        if len(parts) != 2:
            return "Invalid input for TUBECALIBFILE" + str(arguments) + ". Expected TUBECALIBFILE = file_path"
        path2file = parts[1]

        try:
            file_path, suggested_name = getFileAndName(path2file)
            __calibrationWs = Load(file_path, OutputWorkspace=suggested_name)
            reducer.instrument.setCalibrationWorkspace(__calibrationWs)
        except:
            # If we throw a runtime here, then we cannot execute 'Load Data'.
            raise RuntimeError(
                "Invalid input for tube calibration file (" + path2file + " ).\n"
                "Please do not run a reduction as it will not successfully complete.\n"
            )

    def _read_maskfile_line(self, line, reducer):
        try:
            _, value = re.split(r"\s?=\s?", line)
        except ValueError:
            return 'Invalid input: "%s".  Expected "MASKFILE = path to file".' % line

        reducer.settings["MaskFiles"] = value

    def _read_unwrap_monitors_line(self, arguments, reducer):
        """
        Checks if the montiors should be unwrapped. The arguments can be either ON or OFF. We don't care here about
        any preceding slash
        Args:
            arguments: the arguments string
            reducer: a handle to the reducer (is not used)
        """
        # Remove the slash if
        if arguments.find("/") == -1:
            arguments.replace("/", "")
        # Remove white space
        on_off = "".join(arguments.split())
        if on_off == "ON":
            reducer.unwrap_monitors = True
        elif on_off == "OFF":
            reducer.unwrap_monitors = False
        else:
            reducer.unwrap_monitors = False
            return "Unknown setting {0} UNWRAP command in line: ".format(on_off)
        return ""


class GetOutputName(ReductionStep):
    def __init__(self):
        """
        Reads a SANS mask file
        """
        super(GetOutputName, self).__init__()
        self.name_holder = ["problem_setting_name"]

    def execute(self, reducer, workspace=None):
        """
        Generates the name of the sample workspace and changes the
        loaded workspace to that.
        @param reducer the reducer object that called this step
        @param workspace un-used
        """
        reducer.output_wksp = reducer.get_out_ws_name()


class ReplaceErrors(ReductionStep):
    def __init__(self):
        super(ReplaceErrors, self).__init__()
        self.name = None

    def execute(self, reducer, workspace):
        ReplaceSpecialValues(
            InputWorkspace=workspace,
            OutputWorkspace=workspace,
            NaNValue="0",
            InfinityValue="0",
            UseAbsolute=False,
            SmallNumberThreshold=0.0,
            SmallNumberValue=0.0,
            SmallNumberError=0.0,
        )


def _padRunNumber(run_no, field_width):
    nchars = len(run_no)
    digit_end = 0
    for i in range(0, nchars):
        if run_no[i].isdigit():
            digit_end += 1
        else:
            break

    if digit_end == nchars:
        filebase = run_no.rjust(field_width, "0")
        return filebase, run_no
    else:
        filebase = run_no[:digit_end].rjust(field_width, "0")
        return filebase + run_no[digit_end:], run_no[:digit_end]


class StripEndNans(ReductionStep):
    # ISIS only
    def __init__(self):
        super(StripEndNans, self).__init__()

    def _isNan(self, val):
        """
        Can replaced by isNaN in Python 2.6
        @param val: float to check
        """
        if val != val:
            return True
        else:
            return False

    def _isInf(self, val):
        """
        Check if the value is inf or not
        @param val: float to check
        @returns true if value is inf
        """
        return math.isinf(val)

    def execute(self, reducer, workspace):
        """
        Trips leading and trailing Nan values from workspace
        @param reducer: unused
        @param workspace: the workspace to convert
        """
        result_ws = mtd[workspace]
        if result_ws.getNumberHistograms() != 1:
            # Strip zeros is only possible on 1D workspaces
            return

        y_vals = result_ws.readY(0)
        length = len(y_vals)
        # Find the first non-zero value
        start = 0
        for i in range(0, length):
            if not self._isNan(y_vals[i]) and not self._isInf(y_vals[i]):
                start = i
                break
        # Now find the last non-zero value
        stop = 0
        length -= 1
        for j in range(length, 0, -1):
            if not self._isNan(y_vals[j]) and not self._isInf(y_vals[j]):
                stop = j
                break
        # Find the appropriate X values and call CropWorkspace
        x_vals = result_ws.readX(0)
        startX = x_vals[start]
        # Make sure we're inside the bin that we want to crop
        endX = 1.001 * x_vals[stop + 1]
        CropWorkspace(InputWorkspace=workspace, OutputWorkspace=workspace, XMin=startX, XMax=endX)


class GetSampleGeom(ReductionStep):
    """
    Loads, stores, retrieves, etc. data about the geometry of the sample
    On initialisation this class will return default geometry values (compatible with the Colette software)
    There are functions to override these settings
    On execute if there is geometry information in the workspace this will override any unset attributes

    ISIS only
    ORNL only divides by thickness, in the absolute scaling step

    """

    # IDs for each shape as used by the Colette software
    _shape_ids = {1: "cylinder-axis-up", 2: "cuboid", 3: "cylinder-axis-along"}
    _default_shape = "cylinder-axis-along"

    def __init__(self):
        super(GetSampleGeom, self).__init__()

        # string specifies the sample's shape
        self._shape = None
        # sample's width
        self._width = None
        self._thickness = None
        self._height = None

        self._use_wksp_shape = True
        self._use_wksp_width = True
        self._use_wksp_thickness = True
        self._use_wksp_height = True

    def _get_default(self, attrib):
        if attrib == "shape":
            return self._default_shape
        elif attrib == "width" or attrib == "thickness" or attrib == "height":
            return 1.0

    def set_shape(self, new_shape):
        """
        Sets the sample's shape from a string or an ID. If the ID is not
        in the list of allowed values the shape is set to the default but
        shape strings not in the list are not checked
        """
        try:
            # deal with ID numbers as arguments
            new_shape = self._shape_ids[int(new_shape)]
        except ValueError:
            # means that we weren't passed an ID number, the code below treats it as a shape name
            pass
        except KeyError:
            _issueWarning("Warning: Invalid geometry type for sample: " + str(new_shape) + ". Setting default to " + self._default_shape)
            new_shape = self._default_shape

        self._shape = new_shape
        self._use_wksp_shape = False

        # check that the dimensions that we have make sense for our new shape
        if self._width:
            self.width = self._width
        if self._thickness:
            self.thickness = self._thickness

    def get_shape(self):
        if self._shape is None:
            return self._get_default("shape")
        else:
            return self._shape

    def set_width(self, width):
        self._width = float(width)
        self._use_wksp_width = False
        # For a disk the height=width
        if self._shape and self._shape.startswith("cylinder"):
            self._height = self._width
            self._use_wksp_height = False

    def get_width(self):
        self.raise_if_zero(self._width, "width")
        if self._width is None:
            return self._get_default("width")
        else:
            return self._width

    def set_height(self, height):
        self._height = float(height)
        self._use_wksp_height = False

        # For a cylinder and sphere the height=width=radius
        if (self._shape is not None) and (self._shape.startswith("cylinder")):
            self._width = self._height
        self._use_wksp_width = False

    def get_height(self):
        self.raise_if_zero(self._height, "height")
        if self._height is None:
            return self._get_default("height")
        else:
            return self._height

    def set_thickness(self, thickness):
        """
        Simply sets the variable _thickness to the value passed
        """
        # as only cuboids use the thickness the warning below may be informative
        # if (not self._shape is None) and (not self._shape == 'cuboid'):
        #    mantid.sendLogMessage('::SANS::Warning: Can\'t set thickness for shape "'+self._shape+'"')
        self._thickness = float(thickness)
        self._use_wksp_thickness = False

    def get_thickness(self):
        self.raise_if_zero(self._thickness, "thickness")
        if self._thickness is None:
            return self._get_default("thickness")
        else:
            return self._thickness

    def raise_if_zero(self, value, name):
        if value == 0.0:
            message = "Please set the sample geometry %s so that it is not zero."
            raise RuntimeError(message % name)

    shape = property(get_shape, set_shape, None, None)
    width = property(get_width, set_width, None, None)
    height = property(get_height, set_height, None, None)
    thickness = property(get_thickness, set_thickness, None, None)

    def execute(self, reducer, workspace):
        """
        Reads the geometry information stored in the workspace
        but doesn't replace values that have been previously set
        """
        _ = reducer
        wksp = mtd[workspace]
        if isinstance(wksp, WorkspaceGroup):
            wksp = wksp[0]
        sample_details = wksp.sample()

        if self._use_wksp_shape:
            self.shape = sample_details.getGeometryFlag()
        if self._use_wksp_thickness:
            self.thickness = sample_details.getThickness()
        if self._use_wksp_width:
            self.width = sample_details.getWidth()
        if self._use_wksp_height:
            self.height = sample_details.getHeight()

    def __str__(self):
        return (
            "-- Sample Geometry --\n"
            + "    Shape: "
            + self.shape
            + "\n"
            + "    Width: "
            + str(self.width)
            + "\n"
            + "    Height: "
            + str(self.height)
            + "\n"
            + "    Thickness: "
            + str(self.thickness)
            + "\n"
        )


class SampleGeomCor(ReductionStep):
    """
    Correct the neutron count rates for the size of the sample

    ISIS only
    ORNL only divides by thickness, in the absolute scaling step

    """

    def __init__(self):
        self.volume = 1.0

    def calculate_volume(self, reducer):
        geo = reducer.get_sample().geometry
        assert issubclass(geo.__class__, GetSampleGeom)

        try:
            if geo.shape == "cylinder-axis-up":
                # Volume = circle area * height
                # Factor of four comes from radius = width/2
                volume = geo.height * math.pi
                volume *= math.pow(geo.width, 2) / 4.0
            elif geo.shape == "cuboid":
                # Flat plate sample
                volume = geo.width
                volume *= geo.height * geo.thickness
            elif geo.shape == "cylinder-axis-along":
                # Factor of four comes from radius = width/2
                # Disc - where height is not used
                volume = geo.thickness * math.pi
                volume *= math.pow(geo.width, 2) / 4.0
            else:
                raise NotImplementedError('Shape "' + geo.shape + '" is not in the list of supported shapes')
        except TypeError:
            raise TypeError(
                "Error calculating sample volume with width="
                + str(geo.width)
                + " height="
                + str(geo.height)
                + "and thickness="
                + str(geo.thickness)
            )

        return volume

    def execute(self, reducer, workspace):
        """
        Divide the counts by the volume of the sample
        """
        if not reducer.is_can():
            # it calculates the volume for the sample and may or not apply to the can as well.
            self.volume = self.calculate_volume(reducer)

        _ws = mtd[str(workspace)]
        _ws /= self.volume
