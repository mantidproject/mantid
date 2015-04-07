#pylint: disable=invalid-name
"""
    This file defines what happens in each step in the data reduction, it's
    the guts of the reduction. See ISISReducer for order the steps are run
    in and the names they are given to identify them

    Most of this code is a copy-paste from SANSReduction.py, organized to be used with
    ReductionStep objects. The guts needs refactoring.
"""
import os
import math
import copy
import re
import traceback

from mantid.kernel import Logger
sanslog = Logger("SANS")

from mantid.simpleapi import *
from mantid.api import WorkspaceGroup, Workspace, IEventWorkspace
from SANSUtility import (GetInstrumentDetails, MaskByBinRange,
                         isEventWorkspace, getFilePathFromWorkspace,
                         getWorkspaceReference, slice2histogram, getFileAndName,
                         mask_detectors_with_masking_ws)
import isis_instrument
import isis_reducer
from reducer_singleton import ReductionStep

def _issueWarning(msg):
    """
        Prints a message to the log marked as warning
        @param msg: message to be issued
    """
    print msg
    sanslog.warning(msg)

def _issueInfo(msg):
    """
        Prints a message to the log
        @param msg: message to be issued
    """
    print msg
    sanslog.notice(msg)


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
        #entry number of the run inside the run file that will be analysed, as requested by the caller
        self._period = int(entry)
        self._index_of_group = 0

        #set to the total number of periods in the file
        self.periods_in_file = None
        self.ext = ''
        self.shortrun_no = -1
        #the name of the loaded workspace in Mantid
        self._wksp_name = ''

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

    def _load_transmission(self, inst=None, is_can=False, extra_options=dict()):
        if '.raw' in self._data_file or '.RAW' in self._data_file:
            self._load(inst, is_can, extra_options)
            return

        # the intension of the code below is a good idea. Hence the reason why
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
        #try:
        # outWs = LoadNexusMonitors(self._data_file, OutputWorkspace=workspace)
        # self.periods_in_file = 1
        # self._wksp_name = workspace
        #except:
        # self._load(inst, is_can, extra_options)

    def _load(self, inst = None, is_can=False, extra_options=dict()):
        """
            Load a workspace and read the logs into the passed instrument reference
            @param inst: a reference to the current instrument
            @param iscan: set this to True for can runs
            @param extra_options: arguments to pass on to the Load Algorithm.
            @return: number of periods in the workspace
        """
        if self._period != self.UNSET_PERIOD:
            workspace = self._get_workspace_name(self._period)
            extra_options['EntryNumber'] = self._period
        else:
            workspace = self._get_workspace_name()

        extra_options['OutputWorkspace'] = workspace

        outWs = Load(self._data_file, **extra_options)

        if isinstance(outWs, WorkspaceGroup) and '-add' in outWs.getName():
            if len(outWs) != 2:
                raise RuntimeError("Incorrect number of child workspaces. Make sure that the grouped workspace was created within the Add Runs tab.")
            if check_child_ws_for_name_and_type_for_eventdata(outWs):
                raise RuntimeError("Incorrect format of the group workspace. We expect an EventWorkspace for the data and a MatrixWorkspace for the monitors.")
            if self._period != self.UNSET_PERIOD:
                raise RuntimeError("Trying to use multiperiod and added eventdata. This is currently not supported.")

        monitor_ws_name = workspace + "_monitors"

        if isinstance(outWs, IEventWorkspace):
            LoadNexusMonitors(self._data_file, OutputWorkspace=monitor_ws_name)
        else:
            if monitor_ws_name in mtd:
                DeleteWorkspace(monitor_ws_name)

        loader_name = outWs.getHistory().lastAlgorithm().getProperty('LoaderName').value

        if loader_name == 'LoadRaw':
            self._loadSampleDetails(workspace)

        if self._period != self.UNSET_PERIOD and isinstance(outWs, WorkspaceGroup):
            outWs = mtd[self._leaveSinglePeriod(outWs.name(), self._period)]

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
            run += 'p'+str(int(entry_num))

        if self._is_trans:
            return run + '_trans_' + self.ext.lower()
        else:
            return run + '_sans_' + self.ext.lower()


    def _loadSampleDetails(self, ws_name):
        ws_pointer = mtd[str(ws_name)]
        if isinstance(ws_pointer, WorkspaceGroup):
            workspaces = [ws for ws in ws_pointer]
        else:
            workspaces = [ws_pointer]
        for ws in workspaces:
            LoadSampleDetailsFromRaw(ws, self._data_file)


    def _loadFromWorkspace(self, reducer):
        """ It substitute the work of _assignHelper for workspaces, or, at least,
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

        #test if the sample details are already loaded, necessary only for raw files:
        if '.nxs' not in self._data_file[-4:]:
            self._loadSampleDetails(ws_pointer)

        # so, it will try, not to reload the workspace.
        self._wksp_name = ws_pointer.name()
        self.periods_in_file = self._find_workspace_num_periods(self._wksp_name)

        #check that the current workspace has never been moved
        hist_str = self._getHistory(ws_pointer)
        if 'Algorithm: Move' in hist_str or 'Algorithm: Rotate' in hist_str:
            raise RuntimeError('Moving components needs to be made compatible with not reloading the sample')

        return True


    # Helper function
    def _assignHelper(self, reducer):
        if isinstance(self._data_file, Workspace):
            loaded_flag= self._loadFromWorkspace(reducer)
            if loaded_flag:
                return

        if self._data_file == '' or self._data_file.startswith('.'):
            raise RuntimeError('Sample needs to be assigned as run_number.file_type')

        try:
            if reducer.instrument.name() == "":
                raise AttributeError
        except AttributeError:
            raise AttributeError('No instrument has been assign, run SANS2D or LOQ first')

        self._data_file = self._extract_run_details(self._data_file)

        if not self._reload:
            raise NotImplementedError('Raw workspaces must be reloaded, run with reload=True')

        spectrum_limits = dict()
        if self._is_trans:
            if reducer.instrument.name() == 'SANS2D' and int(self.shortrun_no) < 568:
                dimension = GetInstrumentDetails(reducer.instrument)[0]
                spec_min = dimension*dimension*2
                spectrum_limits = {'SpectrumMin':spec_min, 'SpectrumMax':spec_min + 4}

        try:
            if self._is_trans and reducer.instrument.name() != 'LOQ':
                # Unfortunatelly, LOQ in transmission acquire 3 monitors the 3 monitor usually
                # is the first spectrum for detector. This causes the following method to fail
                # when it tries to load only monitors. Hence, we are forced to skip this method
                # for LOQ. ticket #8559
                self._load_transmission(reducer.instrument, extra_options=spectrum_limits)
            else:
                # the spectrum_limits is not the default only for transmission data
                self._load(reducer.instrument, extra_options=spectrum_limits)
        except RuntimeError, details:
            sanslog.warning(str(details))
            self._wksp_name = ''
            return

        return

    def _leaveSinglePeriod(self, workspace, period):
        groupW = mtd[workspace]
        if not isinstance(groupW, WorkspaceGroup):
            logger.warning("Invalid request for getting single period in a non group workspace")
            return workspace
        if len(groupW) < period:
            raise ValueError('Period number ' + str(period) + ' doesn\'t exist in workspace ' + groupW.getName())
        ws_name = groupW[period].name()

        # remove this workspace from the group
        groupW.remove(ws_name)
        # remove the entire group
        DeleteWorkspace(groupW)

        new_name = self._get_workspace_name(period)
        if new_name != ws_name:
            RenameWorkspace(ws_name, OutputWorkspace=new_name)
        return new_name

    def _extract_run_details(self, run_string):
        """
            Takes a run number and file type and generates the filename, workspace name and log name
            @param run_string: either the name of a run file or a run number followed by a dot and then the file type, i.e. file extension
        """
        listOfFiles = FileFinder.findRuns(run_string)
        firstFile = listOfFiles[0]
        self.ext = firstFile[-3:]
        self.shortrun_no = int(re.findall(r'\d+',run_string)[-1])
        return firstFile

    def _find_workspace_num_periods(self, workspace):
        """
            @param workspace: the name of the workspace
        """
        numPeriods = -1
        pWorksp = mtd[workspace]
        if isinstance(pWorksp, WorkspaceGroup) :
            #get the number of periods in a group using the fact that each period has a different name
            numPeriods = len(pWorksp)
        else :
            numPeriods = 1
        return numPeriods

    def _getHistory(self, wk_name):
        ws = getWorkspaceReference(wk_name)

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
            #this is a single entry file, don't consider entries
            return 1
        elif self._period != self.UNSET_PERIOD:
            #the user specified a definite period, use it
            return self._period
        elif self.periods_in_file == reducer.get_sample().loader.periods_in_file:
            #use corresponding periods, the same entry as the sample in each case
            return sample_period
        else:
            raise RuntimeError('There is a mismatch in the number of periods (entries) in the file between the sample and another run')


class LoadTransmissions():
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
        if self._trans_name not in [None, '']:
            self.trans = LoadRun(self._trans_name, trans=True, reload=self._reload, entry=self._period_t)
            self.trans._assignHelper(reducer)
            if isinstance(self._trans_name, Workspace):
                self._trans_name = self._trans_name.name()
            if not self.trans.wksp_name:
                # do nothing if no workspace was specified
                return '', ''

        if self._direct_name not in [None, '']:
            self.direct = LoadRun(self._direct_name, trans=True, reload=self._reload, entry=self._period_d)
            self.direct._assignHelper(reducer)
            if isinstance(self._direct_name, Workspace):
                self._direct_name = self._direct_name.name()
            if not self.direct.wksp_name:
                raise RuntimeError('Transmission run set without direct run error')

        #transmission workspaces sometimes have monitor locations, depending on the instrument, load these locations
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

        #rename the sample workspace, its name will be restored to the original once the subtraction has been done
        tmp_smp = workspace+"_sam_tmp"
        RenameWorkspace(InputWorkspace=workspace,OutputWorkspace= tmp_smp)

        tmp_can = workspace+"_can_tmp"
        #do same corrections as were done to the sample
        reducer.reduce_can(tmp_can)

        #we now have the can workspace, use it
        Minus(LHSWorkspace=tmp_smp,RHSWorkspace= tmp_can,OutputWorkspace= workspace)

        #clean up the workspaces ready users to see them if required
        if reducer.to_Q.output_type == '1D':
            rem_nans = StripEndNans()

        self._keep_partial_results(tmp_smp, tmp_can)


    def get_wksp_name(self):
        return self.workspace.wksp_name

    wksp_name = property(get_wksp_name, None, None, None)

    def get_periods_in_file(self):
        return self.workspace.periods_in_file

    def _keep_partial_results(self, sample_name, can_name):
        # user asked to keep these results 8970
        gp_name = 'sample_can_reductions'
        if mtd.doesExist(gp_name):
            gpr = mtd[gp_name]
            for wsname in [sample_name, can_name]:
                if not gpr.contains(wsname):
                    gpr.add(wsname)
        else:
            GroupWorkspaces([sample_name, can_name], OutputWorkspace=gp_name)

    periods_in_file = property(get_periods_in_file, None, None, None)

class Mask_ISIS(ReductionStep):
    """
        Marks some spectra so that they are not included in the analysis
        Provides ISIS specific mask functionality (e.g. parsing
        MASK commands from user files), inherits from Mask
    """
    def __init__(self, timemask='', timemask_r='', timemask_f='',
                 specmask='', specmask_r='', specmask_f=''):
        self._xml = []

        #these spectra will be masked by the algorithm MaskDetectors
        self.detect_list = []

        # List of pixels to mask
        self.masked_pixels = []

        self.time_mask=timemask
        self.time_mask_r=timemask_r
        self.time_mask_f=timemask_f
        self.spec_mask_r=specmask_r
        self.spec_mask_f=specmask_f

        # as far as I can used to possibly set phi masking
        # not to be applied even though _lim_phi_xml has been set
        self.mask_phi = True
        self.phi_mirror = True
        self._lim_phi_xml = ''
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

        #is set when there is an arm to mask, it's the width in millimetres
        self.arm_width = None
        #when there is an arm to mask this is its angle in degrees
        self.arm_angle = None
        #RMD Mod 24/7/13
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
        if not complete_xml_element.startswith('<') :
            raise ValueError('Excepted xml string but found: ' + str(complete_xml_element))
        self._xml.append(complete_xml_element)

    def _infinite_plane(self, id, plane_pt, normal_pt, complement = False):
        """
            Generates xml code for an infinte plane
            @param id: a string to refer to the shape by
            @param plane_pt: a point in the plane
            @param normal_pt: the direction of a normal to the plane
            @param complement: mask in the direction of the normal or away
            @return the xml string
        """
        if complement:
            addition = '#'
        else:
            addition = ''
        return '<infinite-plane id="' + str(id) + '">' + \
            '<point-in-plane x="' + str(plane_pt[0]) + '" y="' + str(plane_pt[1]) + '" z="' + str(plane_pt[2]) + '" />' + \
            '<normal-to-plane x="' + str(normal_pt[0]) + '" y="' + str(normal_pt[1]) + '" z="' + str(normal_pt[2]) + '" />'+ \
            '</infinite-plane>\n'

    def _infinite_cylinder(self, centre, radius, axis, id='shape'):
        """
            Generates xml code for an infintely long cylinder
            @param centre: a tupple for a point on the axis
            @param radius: cylinder radius
            @param axis: cylinder orientation
            @param id: a string to refer to the shape by
            @return the xml string
        """
        return '<infinite-cylinder id="' + str(id) + '">' + \
            '<centre x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + '" />' + \
            '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
            '<radius val="' + str(radius) + '" /></infinite-cylinder>\n'

    def _finite_cylinder(self, centre, radius, height, axis, id='shape'):
        """
            Generates xml code for an infintely long cylinder
            @param centre: a tupple for a point on the axis
            @param radius: cylinder radius
            @param height: cylinder height
            @param axis: cylinder orientation
            @param id: a string to refer to the shape by
            @return the xml string
        """
        return '<cylinder id="' + str(id) + '">' + \
            '<centre-of-bottom-base x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + '" />' + \
            '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
            '<radius val="' + str(radius) + '" /><height val="' + str(height) + '" /></cylinder>\n'

    def add_cylinder(self, radius, xcentre, ycentre, ID='shape'):
        '''Mask the inside of an infinite cylinder on the input workspace.'''
        self.add_xml_shape(
            self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0,0,1], id=ID)+'<algebra val="' + str(ID) + '"/>')


    def add_outside_cylinder(self, radius, xcentre = 0.0, ycentre = 0.0, ID='shape'):
        '''Mask out the outside of a cylinder or specified radius'''
        self.add_xml_shape(
            self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0,0,1], id=ID)+'<algebra val="#' + str(ID) + '"/>')

    def set_radi(self, min, max):
        self.min_radius = float(min)/1000.
        self.max_radius = float(max)/1000.

    def _whichBank(self, instName, specNo):
        """
            Return either 'rear' or 'front' depending on which bank the spectrum number belong to

            @param instName Instrument name. Used for MASK Ssp command to tell what bank it refer to
            @param specNo Spectrum number
        """
        bank = 'rear'

        if instName.upper() == 'LOQ':
            if 16387 <= specNo <= 17784:
                bank = 'front'
        if instName.upper() == 'SANS2D':
            if 36873 <= specNo <= 73736:
                bank = 'front'

        return bank

    def parse_instruction(self, instName, details):
        """
            Parse an instruction line from an ISIS mask file
            @param instName Instrument name. Used for MASK Ssp command to tell what bank it refer to
            @param details Line to parse
        """
        details = details.lstrip()
        details = details.upper()
        if not details.startswith('MASK') and not details.startswith('L/PHI'):
            _issueWarning('Ignoring malformed mask line ' + details)
            return

        if 'L/PHI' in details:
            phiParts = details.split()
            if len(phiParts) == 3:
                mirror = phiParts[0] != 'L/PHI/NOMIRROR'
                phiMin = phiParts[1]
                phiMax = phiParts[2]
                self.set_phi_limit(float(phiMin), float(phiMax), mirror)
                return
            else:
                _issueWarning('Unrecognized L/PHI masking line command "' + details + '"')
                return

        parts = details.split('/')
        # A spectrum mask or mask spectra range with H and V commands
        if len(parts) == 1:     # Command is to type MASK something
            argToMask = details[4:].lstrip().upper()
            bank = 'rear'
            # special case for MASK Ssp where try to infer the bank the spectrum number belong to
            if 'S' in argToMask:
                if '>' in argToMask:
                    pieces = argToMask.split('>')
                    low = int(pieces[0].lstrip('S'))
                    upp = int(pieces[1].lstrip('S'))
                    bankLow = self._whichBank(instName, low)
                    bankUpp = self._whichBank(instName, upp)
                    if bankLow != bankUpp:
                        _issueWarning('The spectra in Mask command: ' + details +
                                      ' belong to two different banks. Default to use bank ' +
                                      bankLow)
                    bank = bankLow
                else:
                    bank = self._whichBank(instName, int(argToMask.lstrip('S')))

            #Default to the rear detector if not MASK Ssp command
            self.add_mask_string(argToMask, detect=bank)
        elif len(parts) == 2:   # Command is to type MASK/ something
            type = parts[1]   # this is the part of the command following /
            typeSplit = type.split()  # used for command such as MASK/REAR Hn and MASK/Line w a
            if type == 'CLEAR':    # Command is specifically MASK/CLEAR
                self.spec_mask_r = ''
                self.spec_mask_f = ''
            elif type.startswith('T'):
                if type.startswith('TIME'):
                    bin_range = type[4:].lstrip()
                else:
                    bin_range = type[1:].lstrip()
                self.time_mask += ';' + bin_range
            elif len(typeSplit) == 2:
                # Commands such as MASK/REAR Hn, where typeSplit[0] then equal 'REAR'
                if 'S' in typeSplit[1].upper():
                    _issueWarning('MASK command of type ' + details +
                                  ' deprecated. Please use instead MASK Ssp1[>Ssp2]')
                if 'REAR' != typeSplit[0].upper() and instName == 'LOQ':
                    _issueWarning('MASK command of type ' + details +
                                  ' can, until otherwise requested, only be used for the REAR (default) Main detector of LOQ. ' +
                                  'Default to the Main detector of LOQ for this mask command')
                    self.add_mask_string(mask_string=typeSplit[1],detect='rear')
                else:
                    self.add_mask_string(mask_string=typeSplit[1],detect=typeSplit[0])
            elif type.startswith('LINE'):
                # RMD mod 24/7/13
                if len(typeSplit) == 5:
                    self.arm_width = float(typeSplit[1])
                    self.arm_angle = float(typeSplit[2])
                    self.arm_x = float(typeSplit[3])
                    self.arm_y = float(typeSplit[4])
                elif len(typeSplit) == 3:
                    self.arm_width = float(typeSplit[1])
                    self.arm_angle = float(typeSplit[2])
                    self.arm_x=0.0
                    self.arm_y=0.0
                else:
                    _issueWarning('Unrecognized line masking command "' + details + '" syntax is MASK/LINE width angle or MASK/LINE width angle x y')
            else:
                _issueWarning('Unrecognized masking option "' + details + '"')
        elif len(parts) == 3:
            type = parts[1]
            if type == 'CLEAR':
                self.time_mask = ''
                self.time_mask_r = ''
                self.time_mask_f = ''
            elif type == 'TIME' or type == 'T':
                parts = parts[2].split()
                if len(parts) == 3:
                    detname = parts[0].rstrip()
                    bin_range = parts[1].rstrip() + ' ' + parts[2].lstrip()
                    if detname.upper() == 'FRONT':
                        self.time_mask_f += ';' + bin_range
                    elif detname.upper() == 'REAR':
                        self.time_mask_r += ';' + bin_range
                    else:
                        _issueWarning('Detector \'' + detname + '\' not found in currently selected instrument ' + self.instrument.name() + '. Skipping line.')
                else:
                    _issueWarning('Unrecognized masking line "' + details + '"')
        else:
            _issueWarning('Unrecognized masking line "' + details + '"')

    def add_mask_string(self, mask_string, detect):
        if detect.upper() == 'FRONT' or detect.upper() == 'HAB':
            self.spec_mask_f += ',' + mask_string
        elif detect.upper() == 'REAR':
            self.spec_mask_r += ',' + mask_string
        else:
            _issueWarning('Detector \'' + detect + '\' not found in currently selected instrument ' + self.instrument.name() + '. Skipping line.')

    def _ConvertToSpecList(self, maskstring, detector):
        '''
            Convert a mask string to a spectra list
            6/8/9 RKH attempt to add a box mask e.g.  h12+v34 (= one pixel at intersection), h10>h12+v101>v123 (=block 3 wide, 23 tall)

            @param maskstring Is a comma separated list of mask commands for masking spectra using the e.g. the h, s and v commands
        '''
        #Compile spectra ID list
        if maskstring == '':
            return ''
        masklist = maskstring.split(',')

        speclist = ''
        for x in masklist:
            x = x.lower()
            if '+' in x:
                bigPieces = x.split('+')
                if '>' in bigPieces[0]:
                    pieces = bigPieces[0].split('>')
                    low = int(pieces[0].lstrip('hv'))
                    upp = int(pieces[1].lstrip('hv'))
                else:
                    low = int(bigPieces[0].lstrip('hv'))
                    upp = low
                if '>' in bigPieces[1]:
                    pieces = bigPieces[1].split('>')
                    low2 = int(pieces[0].lstrip('hv'))
                    upp2 = int(pieces[1].lstrip('hv'))
                else:
                    low2 = int(bigPieces[1].lstrip('hv'))
                    upp2 = low2
                if 'h' in bigPieces[0] and 'v' in bigPieces[1]:
                    ydim=abs(upp-low)+1
                    xdim=abs(upp2-low2)+1
                    speclist += detector.spectrum_block(low, low2,ydim, xdim) + ','
                elif 'v' in bigPieces[0] and 'h' in bigPieces[1]:
                    xdim=abs(upp-low)+1
                    ydim=abs(upp2-low2)+1
                    speclist += detector.spectrum_block(low2, low,ydim, xdim)+ ','
                else:
                    print "error in mask, ignored:  " + x
            elif '>' in x:  # Commands: MASK Ssp1>Ssp2, MASK Hn1>Hn2 and MASK Vn1>Vn2
                pieces = x.split('>')
                low = int(pieces[0].lstrip('hvs'))
                upp = int(pieces[1].lstrip('hvs'))
                if 'h' in pieces[0]:
                    nstrips = abs(upp - low) + 1
                    speclist += detector.spectrum_block(low, 0,nstrips, 'all')  + ','
                elif 'v' in pieces[0]:
                    nstrips = abs(upp - low) + 1
                    speclist += detector.spectrum_block(0,low, 'all', nstrips)  + ','
                else:
                    for i in range(low, upp + 1):
                        speclist += str(i) + ','
            elif 'h' in x:
                speclist += detector.spectrum_block(int(x.lstrip('h')), 0,1, 'all') + ','
            elif 'v' in x:
                speclist += detector.spectrum_block(0,int(x.lstrip('v')), 'all', 1) + ','
            elif 's' in x:   # Command MASK Ssp. Although note commands of type MASK Ssp1>Ssp2 handled above
                speclist += x.lstrip('s') + ','
            elif x == '':
                #empty entries are allowed
                pass
            elif len(x.split()) == 4:
                _issueWarning('Box mask entry "%s" ignored. Box masking is not supported by Mantid'%('mask '+x))
            else:
                raise SyntaxError('Problem reading a mask entry: "%s"' % x)

        #remove any trailing comma
        if speclist.endswith(','):
            speclist = speclist[0:len(speclist)-1]

        return speclist

    def _mask_phi(self, id, centre, phimin, phimax, use_mirror=True):
        '''
            Mask the detector bank such that only the region specified in the
            phi range is left unmasked
            Purpose of this method is to populate self._lim_phi_xml
        '''
        # convert all angles to be between 0 and 360
        while phimax > 360 : phimax -= 360
        while phimax < 0 : phimax += 360
        while phimin > 360 : phimin -= 360
        while phimin < 0 : phimin += 360
        while phimax<phimin : phimax += 360

        #Convert to radians
        phimin = math.pi*phimin/180.0
        phimax = math.pi*phimax/180.0

        id = str(id)
        self._lim_phi_xml = \
            self._infinite_plane(id+'_plane1',centre, [math.cos(-phimin + math.pi/2.0),math.sin(-phimin + math.pi/2.0),0]) \
            + self._infinite_plane(id+'_plane2',centre, [-math.cos(-phimax + math.pi/2.0),-math.sin(-phimax + math.pi/2.0),0])

        if use_mirror:
            self._lim_phi_xml += self._infinite_plane(id+'_plane3',centre, [math.cos(-phimax + math.pi/2.0),math.sin(-phimax + math.pi/2.0),0]) \
            + self._infinite_plane(id+'_plane4',centre, [-math.cos(-phimin + math.pi/2.0),-math.sin(-phimin + math.pi/2.0),0]) \
            + '<algebra val="#(('+id+'_plane1 '+id+'_plane2):('+id+'_plane3 '+id+'_plane4))" />'
        else:
            #the formula is different for acute verses obtuse angles
            if phimax-phimin > math.pi :
              # to get an obtruse angle, a wedge that's more than half the area, we need to add the semi-inifinite volumes
                self._lim_phi_xml += '<algebra val="#('+id+'_plane1:'+id+'_plane2)" />'
            else :
              # an acute angle, wedge is more less half the area, we need to use the intesection of those semi-inifinite volumes
                self._lim_phi_xml += '<algebra val="#('+id+'_plane1 '+id+'_plane2)" />'

    def _mask_line(self, startPoint, length, width, angle):
        '''
            Creates the xml to mask a line of the given width and height at the given angle
            into the member _line_xml. The masking object which is used to mask a line of say
            a detector array is a finite cylinder
            @param startPoint: startPoint of line
            @param length: length of line
            @param width: width of line in mm
            @param angle: angle of line in xy-plane in units of degrees
            @return: return xml shape string
        '''
        return self._finite_cylinder(startPoint, width/2000.0, length,\
                                               [math.cos(angle*math.pi/180.0),math.sin(angle*math.pi/180.0),0.0], "arm")


    def get_phi_limits_tag(self):
        """
            Get the values of the lowest and highest boundaries
            Used to append to output workspace name
            @return 'Phi'low'_'high if it has been set
        """
        if self.mask_phi and self._lim_phi_xml != '' and (abs(self.phi_max - self.phi_min) != 180.0):
            return 'Phi'+str(self.phi_min)+'_'+str(self.phi_max)
        else:
            return ''

    def set_phi_limit(self, phimin, phimax, phimirror, override=True):
        '''
            ... (tx to Richard for changes to this function
                 for ticket #)
            @param phimin:
            @param phimax:
            @param phimirror:
            @param override: This one I don't understand. It seem
               dangerous to be allowed to set this one to false.
               Also this option cannot be set from the command interface
            @return: return xml shape string
        '''
        if phimirror :
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
            self._mask_phi(
                'unique phi', [0,0,0], self.phi_min,self.phi_max,self.phi_mirror)

    def execute(self, reducer, workspace):
        instrument = reducer.instrument
        #set up the spectra lists and shape xml to mask
        detector = instrument.cur_detector()
        if detector.isAlias('rear'):
            self.spec_list = self._ConvertToSpecList(self.spec_mask_r, detector)
            #Time mask
            MaskByBinRange (workspace,self.time_mask_r)
            MaskByBinRange(workspace,self.time_mask)

        if detector.isAlias('front'):
            #front specific masking
            self.spec_list = self._ConvertToSpecList(self.spec_mask_f, detector)
            #Time mask
            MaskByBinRange(workspace,self.time_mask_f)
            MaskByBinRange(workspace,self.time_mask)

        #reset the xml, as execute can be run more than once
        self._xml = []

        if ( not self.min_radius is None ) and ( self.min_radius > 0.0 ):
            self.add_cylinder(self.min_radius, 0, 0, 'beam_stop')
        if ( not self.max_radius is None ) and ( self.max_radius > 0.0 ):
            self.add_outside_cylinder(self.max_radius, 0, 0, 'beam_area')
        #now do the masking
        for shape in self._xml:
            MaskDetectorsInShape(Workspace=workspace, ShapeXML=shape)

        if "MaskFiles" in reducer.settings:
            for mask_file in reducer.settings["MaskFiles"].split(","):
                try:
                    mask_file_path, mask_ws_name = getFileAndName(mask_file)
                    mask_ws_name = "__" + mask_ws_name
                    LoadMask(Instrument=instrument.idf_path,
                             InputFile=mask_file_path,
                             OutputWorkspace=mask_ws_name)
                    mask_detectors_with_masking_ws(workspace, mask_ws_name)
                    DeleteWorkspace(Workspace=mask_ws_name)
                except:
                    raise RuntimeError("Invalid input for mask file. (%s)" % mask_file)

        if len(self.spec_list)>0:
            MaskDetectors(Workspace=workspace, SpectraList = self.spec_list)

        if self._lim_phi_xml != '' and self.mask_phi:
            MaskDetectorsInShape(Workspace=workspace,ShapeXML= self._lim_phi_xml)

        if self.arm_width and self.arm_angle:
            if instrument.name() == "SANS2D":
                ws = mtd[str(workspace)]
                det = ws.getInstrument().getComponentByName('rear-detector')
                det_Z = det.getPos().getZ()
                start_point = [self.arm_x, self.arm_y, det_Z]
                MaskDetectorsInShape(Workspace=workspace,ShapeXML=\
                                 self._mask_line(start_point, 1e6, self.arm_width, self.arm_angle))

        output_ws, detector_list = ExtractMask(InputWorkspace=workspace, OutputWorkspace="__mask")
        _issueInfo("Mask check %s: %g masked pixels" % (workspace, len(detector_list)))

    def view(self, instrum):
        """
            In MantidPlot this opens InstrumentView to display the masked
            detectors in the bank in a different colour
            @param instrum: a reference an instrument object to view
        """
        wksp_name = 'CurrentMask'
        instrum.load_empty(wksp_name)

        #apply masking to the current detector
        self.execute(None, wksp_name, instrum)

        #now the other detector
        other = instrum.other_detector().name()
        original = instrum.cur_detector().name()
        instrum.setDetector(other)
        self.execute(None, wksp_name, instrum)
        #reset the instrument to mask the currecnt detector
        instrum.setDetector(original)

        # Mark up "dead" detectors with error value
        FindDeadDetectors(InputWorkspace=wksp_name,OutputWorkspace= wksp_name, DeadValue=500)

        #opens an instrument showing the contents of the workspace (i.e. the instrument with masked detectors)
        instrum.view(wksp_name)

    def display(self, wksp, reducer, counts=None):
        """
            Mask detectors in a workspace and display its show instrument
            @param wksp: this named workspace will be masked and displayed
            @param reducer: the reduction chain that contains all the settings
            @param counts: optional workspace containing neutron counts data that the mask will be supperimposed on to
        """
        #apply masking to the current detector
        self.execute(reducer, wksp)

        instrum = reducer.instrument
        #now the other detector
        other = instrum.other_detector().name()
        original = instrum.cur_detector().name()
        instrum.setDetector(other)
        self.execute(reducer, wksp)
        #reset the instrument to mask the current detector
        instrum.setDetector(original)

        if counts:
            Power(InputWorkspace=counts,OutputWorkspace= 'ones',Exponent= 0)
            Plus(LHSWorkspace=wksp,RHSWorkspace= 'ones',OutputWorkspace= wksp)

        # Mark up "dead" detectors with error value
        FindDeadDetectors(InputWorkspace=wksp,OutputWorkspace= wksp, LiveValue = 0, DeadValue=1)

        #check if we have a workspace to superimpose the mask on to
        if counts:
            #the code below is a proto-type for the ISIS SANS group, to make it perminent it should be improved

            #create a workspace where the masked spectra have a value
            flags = mtd[wksp]
            #normalise that value to the data in the workspace
            vals = mtd[counts]
            maxval = 0
            Xs = []
            Ys = []
            Es = []
            for i in range(0, flags.getNumberHistograms()):
                Xs.append(flags.readX(i)[0])
                Xs.append(flags.readX(i)[1])
                Ys.append(flags.readY(i)[0])
                Es.append(0)

                if vals.readY(i)[0] > maxval:
                    #don't include masked or monitors
                    if (flags.readY(i)[0] == 0) and (vals.readY(i)[0] < 5000):
                        maxval = vals.readY(i)[0]

            #now normalise to the max/5
            maxval /= 5.0
            for i in range(0, len(Ys)):
                if Ys[i] != 0:
                    Ys[i] = maxval*Ys[i] + vals.readY(i)[0]

            CreateWorkspace(OutputWorkspace=wksp,DataX= Xs,DataY= Ys,DataE= Es,NSpec= len(Ys), UnitX='TOF')
            #change the units on the workspace so it is compatible with the workspace containing counts data
            Multiply(LHSWorkspace='ones',RHSWorkspace= wksp,OutputWorkspace= 'units')
            #do the super-position and clean up
            Minus(LHSWorkspace=counts,RHSWorkspace= 'units',OutputWorkspace= wksp)
            reducer.deleteWorkspaces(['ones', 'units'])

        #opens an instrument showing the contents of the workspace (i.e. the instrument with masked detectors)
        instrum.view(wksp)

    def __str__(self):
        return '    radius', self.min_radius, self.max_radius+'\n'+\
            '    rear spectrum mask: ', str(self.spec_mask_r)+'\n'+\
            '    front spectrum mask: ', str(self.spec_mask_f)+'\n'+\
            '    global time mask: ', str(self.time_mask)+'\n'+\
            '    rear time mask: ', str(self.time_mask_r)+'\n'+\
            '    front time mask: ', str(self.time_mask_f)+'\n'


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
        #is set to the entry (period) number in the sample to be run
        self.entries = []

    def execute(self, reducer, isSample):
        self._assignHelper(reducer)

        if self.wksp_name == '':
            raise RuntimeError('Unable to load SANS sample run, cannot continue.')

        if self.periods_in_file > 1:
            self.entries = range(0, self.periods_in_file)

        # applies on_load_sample for all the workspaces (single or groupworkspace)
        num = 0
        while True:
            reducer.instrument.on_load_sample(self.wksp_name, reducer.get_beam_center(), isSample)
            num += 1
            if num == self.periods_in_file:
                break
            self.move2ws(num)
        self.move2ws(0)


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

        #the result of this calculation that will be used by CalculateNorm() and the ConvertToQ
        self.output_wksp = None

    def execute(self, reducer, workspace):
        normalization_spectrum = self._normalization_spectrum
        if normalization_spectrum is None:
            #the -1 converts from spectrum number to spectrum index
            normalization_spectrum = reducer.instrument.get_incident_mon()

        sanslog.notice('Normalizing to monitor ' + str(normalization_spectrum))

        self.output_wksp = str(workspace) + '_incident_monitor'
        mon = reducer.get_sample().get_monitor(normalization_spectrum-1)
        if reducer.event2hist.scale != 1:
            mon *= reducer.event2hist.scale

        if str(mon) != self.output_wksp:
            RenameWorkspace(mon, OutputWorkspace=self.output_wksp)

        if reducer.instrument.name() == 'LOQ':
            RemoveBins(InputWorkspace=self.output_wksp,OutputWorkspace= self.output_wksp,XMin= reducer.transmission_calculator.loq_removePromptPeakMin,XMax=
                       reducer.transmission_calculator.loq_removePromptPeakMax, Interpolation="Linear")

        # Remove flat background
        TOF_start, TOF_end = reducer.inst.get_TOFs(normalization_spectrum)

        if TOF_start and TOF_end:
            CalculateFlatBackground(InputWorkspace=self.output_wksp,OutputWorkspace= self.output_wksp, StartX=TOF_start, EndX=TOF_end, Mode='Mean')

        #perform the same conversion on the monitor spectrum as was applied to the workspace but with a possibly different rebin
        if reducer.instrument.is_interpolating_norm():
            r_alg = 'InterpolatingRebin'
        else :
            r_alg = 'Rebin'
        reducer.to_wavelen.execute(reducer, self.output_wksp, bin_alg=r_alg)

class TransmissionCalc(ReductionStep):
    """
        Calculates the proportion of neutrons that are transmitted through the sample
        as a function of wavelength. The results are stored as a workspace
    """

    # The different ways of doing a fit, convert the possible ways of specifying this (also the way it is specified in the GUI to the way it can be send to CalculateTransmission
    TRANS_FIT_OPTIONS = {
        'YLOG' : 'Log',
        'STRAIGHT' : 'Linear',
        'CLEAR' : 'Linear',
        # Add Mantid ones as well
        'LOGARITHMIC' : 'Log',
        'LOG' : 'Log',
        'LINEAR' : 'Linear',
        'LIN' : 'Linear',
        'OFF' : 'Linear',
        'POLYNOMIAL':'Polynomial'}

    #map to restrict the possible values of _trans_type
    CAN_SAMPLE_SUFFIXES = {
        False : 'sample',
        True : 'can'}

    DEFAULT_FIT = 'LOGARITHMIC'

    def __init__(self, loader=None):
        super(TransmissionCalc, self).__init__()
        #set these variables to None, which means they haven't been set and defaults will be set further down
        self.fit_props = ['lambda_min', 'lambda_max', 'fit_method', 'order']
        self.fit_settings = dict()
        for prop in self.fit_props:
            self.fit_settings['both::'+prop] = None
        # this contains the spectrum number of the monitor that comes after the sample from which the transmission calculation is done
        self._trans_spec = None
        # use InterpolatingRebin
        self.interpolate = None
        # a custom transmission workspace, if we have this there is much less to do
        self.calculated_samp = ''
        self.calculated_can = None
        #the result of this calculation that will be used by CalculateNorm() and the ConvertToQ
        self.output_wksp = None
        # Use for removing LOQ prompt peak from monitors. Units of micro-seconds
        self.loq_removePromptPeakMin = 19000.0
        self.loq_removePromptPeakMax = 20500.0


    def set_trans_fit(self, fit_method, min_=None, max_=None, override=True, selector='both'):
        """
            Set how the transmission fraction fit is calculated, the range of wavelengths
            to use and the fit method
            @param min: minimum wavelength to use
            @param max: highest wavelength to use
            @param fit_method: the fit type to pass to CalculateTransmission ('Logarithmic' or 'Linear')or 'Off'
            @param override: if set to False this call won't override the settings set by a previous call (default True)
            @param selector: define if the given settings is valid for SAMPLE, CAN or BOTH transmissions.
        """
        FITMETHOD = 'fit_method'
        LAMBDAMIN = 'lambda_min'
        LAMBDAMAX = 'lambda_max'
        ORDER = 'order'
        # processing the selector input
        select = selector.lower()
        if select not in ['both', 'can', 'sample']:
            _issueWarning('Invalid selector option ('+selector+'). Fit to transmission skipped')
            return
        select += "::"

        if not override and self.fit_settings.has_key(select + FITMETHOD) and self.fit_settings[select + FITMETHOD]:
            #it was already configured and this request does not want to override
            return

        if not fit_method:
            # there is not point calling fit_method without fit_method argument
            return

        fit_method = fit_method.upper()
        if 'POLYNOMIAL' in fit_method:
            order_str = fit_method[10:]
            fit_method = 'POLYNOMIAL'
            self.fit_settings[select+ORDER] = int(order_str)
        if fit_method not in self.TRANS_FIT_OPTIONS.keys():
            _issueWarning('ISISReductionStep.Transmission: Invalid fit mode passed to TransFit, using default method (%s)' % self.DEFAULT_FIT)
            fit_method = self.DEFAULT_FIT

        # get variables for this selector
        sel_settings = dict()
        for prop in self.fit_props:
            sel_settings[prop] = self.fit_settings[select+prop] if self.fit_settings.has_key(select+prop) else self.fit_settings['both::'+prop]

        # copy fit_method
        sel_settings[FITMETHOD] = fit_method

        if min_:
            sel_settings[LAMBDAMIN] = float(min_) if fit_method not in ['OFF', 'CLEAR'] else None
        if max_:
            sel_settings[LAMBDAMAX] = float(max_) if fit_method not in ['OFF', 'CLEAR'] else None

        # apply the propertis to self.fit_settings
        for prop in self.fit_props:
            self.fit_settings[select+prop] = sel_settings[prop]

        # When both is given, it is necessary to clean the specific settings for the individual selectors
        if select == 'both::':
            for selector_ in ['sample::','can::']:
                for prop_ in self.fit_props:
                    prop_name = selector_+prop_
                    if self.fit_settings.has_key(prop_name):
                        del self.fit_settings[prop_name]

    def isSeparate(self):
        """ Returns true if the can or sample was given and false if just both was used"""
        return self.fit_settings.has_key('sample::fit_method') or self.fit_settings.has_key('can::fit_method')

    def setup_wksp(self, inputWS, inst, wavbining, pre_monitor, post_monitor):
        """
            Creates a new workspace removing any background from the monitor spectra, converting units
            and re-bins. If the instrument is LOQ it zeros values between the x-values 19900 and 20500
            This method doesn't affect self.
            @param inputWS: contains the monitor spectra
            @param inst: the selected instrument
            @param wavbinning: the re-bin string to use after convert units
            @param pre_monitor: DETECTOR ID of the incident monitor
            @param post_monitor: DETECTOR ID of the transmission monitor
            @return the name of the workspace created
        """
        #the workspace is forked, below is its new name
        tmpWS = inputWS + '_tmp'

        #exclude unused spectra because the sometimes empty/sometimes not spectra can cause errors with interpolate
        spectrum1 = min(pre_monitor, post_monitor)
        spectrum2 = max(pre_monitor, post_monitor)
        CropWorkspace(InputWorkspace=inputWS,OutputWorkspace= tmpWS,\
            StartWorkspaceIndex=self._get_index(spectrum1),\
            EndWorkspaceIndex=self._get_index(spectrum2))

        if inst.name() == 'LOQ':
            RemoveBins(InputWorkspace=tmpWS,OutputWorkspace= tmpWS,XMin= self.loq_removePromptPeakMin,XMax= self.loq_removePromptPeakMax,
                       Interpolation='Linear')

        for spectra_number in [pre_monitor, post_monitor]:
            back_start, back_end = inst.get_TOFs(spectra_number)
            if back_start and back_end:
                index = spectra_number - spectrum1
                CalculateFlatBackground(InputWorkspace=tmpWS,OutputWorkspace= tmpWS, StartX=back_start, EndX=back_end,\
                               WorkspaceIndexList=index, Mode='Mean')

        ConvertUnits(InputWorkspace=tmpWS,OutputWorkspace= tmpWS,Target="Wavelength")

        if self.interpolate:
            InterpolatingRebin(InputWorkspace=tmpWS,OutputWorkspace= tmpWS,Params= wavbining)
        else :
            Rebin(InputWorkspace=tmpWS,OutputWorkspace= tmpWS,Params= wavbining)

        return tmpWS

    def _get_index(self, number):
        """
            Converts spectrum numbers to indices using the simple (minus 1) relationship
            that is true for raw files
            @param number: a spectrum number
            @return: its index
        """
        return number - 1

    def execute(self, reducer, workspace):
        """
            Reads in the different settings, without affecting self. Calculates
            or estimates the proportion of neutrons that are transmitted
            through the sample
        """
        self.output_wksp = None
        #look for run files that contain transmission data
        test1, test2 = self._get_run_wksps(reducer)
        if test1 or test2:
            #we can calculate the transmission from some experimental runs
            if self.calculated_samp:
                raise RuntimeError('Cannot use TransWorkspace() and TransmissionSample() together')

            self.output_wksp = self.calculate(reducer)
        else:
            #they have supplied a transmission file use it
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
        LAMBDAMIN = 'lambda_min'
        LAMBDAMAX = 'lambda_max'
        FITMETHOD = 'fit_method'
        ORDER = 'order'
        #get the settings required to do the calculation
        trans_raw, direct_raw = self._get_run_wksps(reducer)

        if not trans_raw:
            raise RuntimeError('Attempting transmission correction with no specified transmission %s file' % self.CAN_SAMPLE_SUFFIXES[reducer.is_can()])
        if not direct_raw:
            raise RuntimeError('Attempting transmission correction with no direct file')

        select = 'can::' if reducer.is_can() else 'direct::'

        # get variables for this selector
        sel_settings = dict()
        for prop in self.fit_props:
            sel_settings[prop] = self.fit_settings[select+prop] if self.fit_settings.has_key(select+prop) else self.fit_settings['both::'+prop]

        if self._trans_spec:
            post_sample = self._trans_spec
        else:
            post_sample = reducer.instrument.default_trans_spec

        pre_sample = reducer.instrument.incid_mon_4_trans_calc

        use_instrum_default_range = reducer.full_trans_wav

        #there are a number of settings and defaults that determine the wavelength to use, go through each in order of increasing precedence
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
        wavbin +=','+str(reducer.to_wavelen.wav_step)
        wavbin +=','+str(translambda_max)

        #set up the input workspaces
        trans_tmp_out = self.setup_wksp(trans_raw, reducer.instrument,\
            wavbin, pre_sample, post_sample)
        direct_tmp_out = self.setup_wksp(direct_raw, reducer.instrument,\
            wavbin, pre_sample, post_sample)

        fittedtransws, unfittedtransws = self.get_wksp_names(\
                    trans_raw, translambda_min, translambda_max, reducer)

        # If no fitting is required just use linear and get unfitted data from CalculateTransmission algorithm
        options = dict()
        if sel_settings[FITMETHOD]:
            options['FitMethod'] = self.TRANS_FIT_OPTIONS[sel_settings[FITMETHOD]]
            if sel_settings[FITMETHOD] == "POLYNOMIAL":
                options['PolynomialOrder'] = sel_settings[ORDER]
        else:
            options['FitMethod'] = self.TRANS_FIT_OPTIONS[self.DEFAULT_FIT]

        CalculateTransmission(SampleRunWorkspace=trans_tmp_out, DirectRunWorkspace=direct_tmp_out,
                              OutputWorkspace=fittedtransws, IncidentBeamMonitor=pre_sample,
                              TransmissionMonitor=post_sample,
                              RebinParams=reducer.to_wavelen.get_rebin(),
                              OutputUnfittedData=True, **options) # options FitMethod, PolynomialOrder if present

        # Remove temporaries
        files2delete = [trans_tmp_out]

        if direct_tmp_out != trans_tmp_out:
            files2delete.append(direct_tmp_out)

        if sel_settings[FITMETHOD] in ['OFF', 'CLEAR']:
            result = unfittedtransws
            files2delete.append(fittedtransws)
        else:
            result = fittedtransws

        reducer.deleteWorkspaces(files2delete)

        return result

    def get_trans_spec(self):
        return self._trans_spec

    def set_trans_spec(self, value):
        """
            Allows setting the which transmission monitor that is passed the sample
            if the new value is an integer
        """
        self._trans_spec = int(value)

    trans_spec = property(get_trans_spec, set_trans_spec, None, None)

    def get_wksp_names(self, raw_name, lambda_min, lambda_max, reducer):
        fitted_name = raw_name.split('_')[0] + '_trans_'
        fitted_name += self.CAN_SAMPLE_SUFFIXES[reducer.is_can()]
        fitted_name += '_'+str(lambda_min)+'_'+str(lambda_max)

        unfitted = fitted_name + "_unfitted"

        return fitted_name, unfitted

    def _get_fit_property(self,selector, property_name):
        if self.fit_settings.has_key(selector+'::' + property_name):
            return self.fit_settings[selector+'::' + property_name]
        else:
            return self.fit_settings['both::'+property_name]


    def lambdaMin(self, selector):
        return self._get_fit_property(selector.lower(), 'lambda_min')
    def lambdaMax(self, selector):
        return self._get_fit_property(selector.lower(), 'lambda_max')
    def fitMethod(self, selector):
        """It will return LINEAR, LOGARITHM, POLYNOMIALx for x in 2,3,4,5"""
        resp = self._get_fit_property(selector.lower(), 'fit_method')
        if 'POLYNOMIAL' == resp:
            resp += str(self._get_fit_property(selector.lower(), 'order'))
        if resp  in ['LIN','STRAIGHT'] :
            resp = 'LINEAR'
        if resp in ['YLOG','LOG']:
            resp = 'LOGARITHMIC'
        return resp

class AbsoluteUnitsISIS(ReductionStep):
    DEFAULT_SCALING = 100.0
    def __init__(self):
        # Scaling values [%]
        self.rescale= self.DEFAULT_SCALING

    def execute(self, reducer, workspace):
        scalefactor = self.rescale
        # Data reduced with Mantid is a factor of ~pi higher than Colette.
        # For LOQ only, divide by this until we understand why.
        if reducer.instrument.name() == 'LOQ':
            rescaleToColette = math.pi
            scalefactor /= rescaleToColette

        ws = mtd[workspace]
        ws *= scalefactor


class CalculateNormISIS(object):
    """
        Note this is not a reduction step, see CalculateNorm

        Generates the normalization workspaces required by Q1D and Qxy for normalization
        produced by other, sometimes optional, reduction_steps or a specified
        workspace
    """
    TMP_ISIS_NAME = '__CalculateNormISIS_loaded_tmp'
    TMP_WORKSPACE_NAME = '__CalculateNorm_loaded_temp'
    WAVE_CORR_NAME = '__Q_WAVE_conversion_temp'
    PIXEL_CORR_NAME = '__Q_pixel_conversion_temp'

    def  __init__(self, wavelength_deps=[]):
        super(CalculateNormISIS, self).__init__()
        self._wave_steps = wavelength_deps
        self._high_angle_pixel_file = ""
        self._low_angle_pixel_file = ""
        self._pixel_file = ""


    def setPixelCorrFile(self, filename, detector = ""):
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

    def getPixelCorrFile(self, detector ):
        """
          For compatibility reason, it still uses the self._pixel_file,
          but, now, we need pixel_file (flood file) for both detectors.
          so, an extra parameter is allowed.

        """
        detector = detector.upper()
        if detector in ("FRONT", "HAB", "FRONT-DETECTOR-BANK", "FRONT-DETECTOR"):
            return self._high_angle_pixel_file
        elif detector in ("REAR","MAIN", "MAIN-DETECTOR-BANK", "", "REAR-DETECTOR", "DETECTORBENCH"):
            return self._low_angle_pixel_file
        else :
            logger.warning("Request of pixel correction file with unknown detector ("+ str(detector)+")")
            return self._pixel_file


    def _multiplyAll(self, wave_wksps, wksp2match):
        wave_adj = None
        for wksp in wave_wksps:
            #before the workspaces can be combined they all need to match
            RebinToWorkspace(WorkspaceToRebin=wksp, WorkspaceToMatch=wksp2match,
                             OutputWorkspace=self.TMP_WORKSPACE_NAME)

            if not wave_adj:
                wave_adj = self.WAVE_CORR_NAME
                RenameWorkspace(InputWorkspace=self.TMP_WORKSPACE_NAME, OutputWorkspace=wave_adj)
            else:
                Multiply(LHSWorkspace=self.TMP_WORKSPACE_NAME, RHSWorkspace=wave_adj,
                         OutputWorkspace= wave_adj)
        return wave_adj

    def _loadPixelCorrection(self):
        '''
        Reads in a pixel correction file if one has been specified.

        @return the name of the workspace, else an empty string if there was no
                correction file.
        '''
        if self._pixel_file:
            LoadRKH(Filename=self._pixel_file,
                    OutputWorkspace=self.PIXEL_CORR_NAME,
                    FirstColumnValue="SpectrumNumber")
            return self.PIXEL_CORR_NAME
        return ''

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

    def calculate(self, reducer, wave_wks=[]):
        """
            Multiplies all the wavelength scalings into one workspace and all the detector
            dependent scalings into another workspace that can be used by ConvertToQ
            @param reducer: settings used for this reduction
            @param wave_wks: additional wavelength dependent correction workspaces to include
        """
        #use the instrument's correction file
        corr_file = reducer.instrument.cur_detector().correction_file
        if corr_file:
            LoadRKH(Filename=corr_file,OutputWorkspace= self.TMP_ISIS_NAME,FirstColumnValue= "Wavelength")
            wave_wks.append(self.TMP_ISIS_NAME)

            if self._is_point_data(self.TMP_ISIS_NAME):
                ConvertToHistogram(InputWorkspace=self.TMP_ISIS_NAME,OutputWorkspace= self.TMP_ISIS_NAME)
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
            #remove all the pixels that are not present in the sample data (the other detector)
            reducer.instrument.cur_detector().crop_to_detector(pixel_adj, pixel_adj)

        reducer.deleteWorkspaces([self.TMP_ISIS_NAME, self.TMP_WORKSPACE_NAME])

        return wave_adj, pixel_adj

class ConvertToQISIS(ReductionStep):
    """
    Runs the Q1D or Qxy algorithms to convert wavelength data into momentum transfer, Q

    Currently, this allows the wide angle transmission correction.
    """
    # the list of possible Q conversion algorithms to use
    _OUTPUT_TYPES = {'1D' : 'Q1D',
                     '2D' : 'Qxy'}
    # defines if Q1D should correct for gravity by default
    _DEFAULT_GRAV = False
    def __init__(self, normalizations):
        """
            @param normalizations: CalculateNormISIS object contains the workspace, ReductionSteps or files require for the optional normalization arguments
        """
        if not issubclass(normalizations.__class__,  CalculateNormISIS):
            raise RuntimeError('Error initializing ConvertToQ, invalid normalization object')
        #contains the normalization optional workspaces to pass to the Q algorithm
        self._norms = normalizations

        #this should be set to 1D or 2D
        self._output_type = '1D'
        #the algorithm that corresponds to the above choice
        self._Q_alg = self._OUTPUT_TYPES[self._output_type]
        #if true gravity is taken into account in the Q1D calculation
        self._use_gravity = self._DEFAULT_GRAV
        #used to implement a default setting for gravity that can be over written but doesn't over write
        self._grav_set = False
        #this should contain the rebin parameters
        self.binning = None

        #The minimum distance in metres from the beam center at which all wavelengths are used in the calculation
        self.r_cut = 0.0
        #The shortest wavelength in angstrom at which counts should be summed from all detector pixels in Angstrom
        self.w_cut = 0.0
        # Whether to output parts when running either Q1D2 or Qxy
        self.outputParts = False

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
            print msg
            sanslog.warning(msg)

    def execute(self, reducer, workspace):
        """
        Calculate the normalization workspaces and then call the chosen Q conversion algorithm.
        """
        wavepixeladj = ""
        if reducer.wide_angle_correction and reducer.transmission_calculator.output_wksp:
            #calculate the transmission wide angle correction
            _issueWarning("sans solid angle correction execution")
            SANSWideAngleCorrection(SampleData=workspace,\
                                     TransmissionData = reducer.transmission_calculator.output_wksp,\
                                     OutputWorkspace='transmissionWorkspace')
            wavepixeladj = 'transmissionWorkspace'
        #create normalization workspaces
        if self._norms:
            # the empty list at the end appears to be needed (the system test SANS2DWaveloops) is this a bug in Python?
            wave_adj, pixel_adj = self._norms.calculate(reducer, [])
        else:
            raise RuntimeError('Normalization workspaces must be created by CalculateNorm() and passed to this step')

        try:
            if self._Q_alg == 'Q1D':
                Q1D(DetBankWorkspace=workspace,OutputWorkspace= workspace, OutputBinning=self.binning, WavelengthAdj=wave_adj, PixelAdj=pixel_adj, AccountForGravity=self._use_gravity, RadiusCut=self.r_cut*1000.0, WaveCut=self.w_cut, OutputParts=self.outputParts, WavePixelAdj = wavepixeladj)
            elif self._Q_alg == 'Qxy':
                Qxy(InputWorkspace=workspace,OutputWorkspace= workspace,MaxQxy= reducer.QXY2,DeltaQ= reducer.DQXY, WavelengthAdj=wave_adj, PixelAdj=pixel_adj, AccountForGravity=self._use_gravity, RadiusCut=self.r_cut*1000.0, WaveCut=self.w_cut, OutputParts=self.outputParts)
                ReplaceSpecialValues(InputWorkspace=workspace,OutputWorkspace= workspace, NaNValue="0", InfinityValue="0")
            else:
                raise NotImplementedError('The type of Q reduction has not been set, e.g. 1D or 2D')
        except:
            #when we are all up to Python 2.5 replace the duplicated code below with one finally:
            reducer.deleteWorkspaces([wave_adj, pixel_adj, wavepixeladj])
            raise

        reducer.deleteWorkspaces([wave_adj, pixel_adj, wavepixeladj])

class UnitsConvert(ReductionStep):
    """
        Executes ConvertUnits and then Rebin on the same workspace. If no re-bin limits are
        set for the x-values of the final workspace the range of the first spectrum is used.
    """
    def __init__(self, units, rebin = 'Rebin', bin_alg=None):
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

    #TODO: consider how to remove the extra argument after workspace
    def execute(self, reducer, workspace, bin_alg=None):
        """
            Runs the ConvertUnits() and a rebin algorithm on the specified
            workspace
            @param reducer:
            @param workspace: the name of the workspace to convert
            @param workspace: the name of the workspace to convert
        """
        ConvertUnits(InputWorkspace=workspace,OutputWorkspace= workspace,Target= self._units)

        low_wav = self.wav_low
        high_wav = self.wav_high

        if low_wav is None and high_wav is None:
            low_wav = min(mtd[workspace].readX(0))
            high_wav = max(mtd[workspace].readX(0))


        if not bin_alg:
            bin_alg = self.rebin_alg

        rebin_com = bin_alg+'(workspace, "'+\
            self._get_rebin(low_wav, self.wav_step, high_wav)+'", OutputWorkspace=workspace)'
        eval(rebin_com)

    def _get_rebin(self, low, step, high):
        """
            Convert the range limits and step into a form passable to re-bin
            @param low: first number in the Rebin string, the first bin boundary
            @param step: bin width
            @param high: high bin boundary
        """
        return str(low)+', ' + str(step) + ', ' + str(high)

    def get_rebin(self):
        """
            Get the string that is passed as the "param" property to Rebin
            @return the string that is passed to Rebin
        """
        return self._get_rebin(self.wav_low, self.wav_step, self.wav_high)

    def set_rebin(self, w_low = None, w_step = None, w_high = None, override=True):
        """
            Set the parameters that are passed to Rebin
            @param w_low: first number in the Rebin string, the first bin boundary
            @param w_step: bin width
            @param w_high: high bin boundary
        """
        if not w_low is None:
            if self.wav_low is None or override:
                self.wav_low = float(w_low)
        if not w_step is None:
            if self.wav_step is None or override:
                self.wav_step = float(w_step)
        if not w_high is None:
            if self.wav_high is None or override:
                self.wav_high = float(w_high)

    def get_range(self):
        """
            Get the values of the highest and lowest boundaries
            @return low'_'high
        """
        return str(self.wav_low)+'_'+str(self.wav_high)

    def set_range(self, w_low = None, w_high = None):
        """
            Set the highest and lowest bin boundary values
            @param w_low: first number in the Rebin string, the first bin boundary
            @param w_high: high bin boundary
        """
        self.set_rebin(w_low, None, w_high)

    def __str__(self):
        return '    Wavelength range: ' + self.get_rebin()

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
        start, stop = reducer.getCurrSliceLimit()

        _monitor = reducer.get_sample().get_monitor()

        if "events.binning" in reducer.settings:
            binning = reducer.settings["events.binning"]
        else:
            binning = ""
        hist, (tot_t, tot_c, part_t, part_c) = slice2histogram(ws_pointer, start, stop, _monitor, binning)
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
            'BACK/' : self._read_back_line,
            'TRANS/': self._read_trans_line,
            'MON/' : self._read_mon_line,
            'TUBECALIBFILE': self._read_calibfile_line,
            'MASKFILE': self._read_maskfile_line}

    def __deepcopy__(self, memo):
        """Called when a deep copy is requested
        """
        fresh = UserFile(self.filename)
        fresh._incid_monitor_lckd = self._incid_monitor_lckd
        fresh.executed = self.executed
        fresh.key_functions = {
            'BACK/' : fresh._read_back_line,
            'TRANS/': fresh._read_trans_line,
            'MON/' : fresh._read_mon_line,
            'TUBECALIBFILE': self._read_calibfile_line,
            'MASKFILE': self._read_maskfile_line
            }
        return fresh

    def execute(self, reducer, workspace=None):
        if self.filename is None:
            raise AttributeError('The user file must be set, use the function MaskFile')
        user_file = self.filename

        #Check that the file exists.
        if not os.path.isfile(user_file):
            user_file = os.path.join(reducer.user_file_path, self.filename)
            if not os.path.isfile(user_file):
                user_file = FileFinder.getFullPath(self.filename)
                if not os.path.isfile(user_file):
                    raise RuntimeError, "Cannot read mask. File path '%s' does not exist or is not in the user path." % self.filename

        reducer.user_file_path = os.path.dirname(user_file)
        # Re-initializes default values
        self._initialize_mask(reducer)
        reducer.prep_normalize.setPixelCorrFile('','REAR')
        reducer.prep_normalize.setPixelCorrFile('','FRONT')

        file_handle = open(user_file, 'r')
        for line in file_handle:
            try:
                self.read_line(line, reducer)
            except:
                # Close the handle
                file_handle.close()
                raise RuntimeError("%s was specified in the MASK file (%s) but the file cannot be found." % (line.rsplit()[0], file_handle.name))

        # Check if one of the efficency files hasn't been set and assume the other is to be used
        reducer.instrument.copy_correction_files()

        self.executed = True
        return self.executed

    def read_line(self, line, reducer):
        # This is so that I can be sure all EOL characters have been removed
        line = line.lstrip().rstrip()
        upper_line = line.upper()

        #check for a recognised command
        for keyword in self.key_functions.keys():
            if upper_line.startswith(keyword):
                #remove the keyword as it has already been parsed
                params = line[len(keyword):]
                #call the handling function for that keyword
                error = self.key_functions[keyword](params, reducer)

                if error:
                    _issueWarning(error+line)

                return

        if upper_line.startswith('L/'):
            self.readLimitValues(line, reducer)

        elif upper_line.startswith('MASK'):
            if len(upper_line[5:].strip().split()) == 4:
                _issueInfo('Box masks can only be defined using the V and H syntax, not "mask x1 y1 x2 y2"')
            else:
                reducer.mask.parse_instruction(reducer.instrument.name(), upper_line)

        elif upper_line.startswith('SET CENTRE'):
            # SET CENTRE accepts the following properties:
            # SET CENTRE X Y
            # SET CENTRE/MAIN X Y
            # SET CENTRE/HAB X Y
            main_str_pos = upper_line.find('MAIN')
            hab_str_pos = upper_line.find('HAB')
            x_pos = 0.0
            y_pos = 0.0
            # use the scale factors supplied in the parameter file
            XSF = reducer.inst.beam_centre_scale_factor1
            YSF = reducer.inst.beam_centre_scale_factor2

            if main_str_pos > 0:
                values = upper_line[main_str_pos+5:].split() #remov the SET CENTRE/MAIN
                x_pos = float(values[0])/XSF
                y_pos = float(values[1])/YSF
            elif hab_str_pos > 0:
                values = upper_line[hab_str_pos+4:].split() # remove the SET CENTRE/HAB
                print ' convert values ',values
                x_pos = float(values[0])/XSF
                y_pos = float(values[1])/YSF
            else:
                values = upper_line.split()
                x_pos = float(values[2])/XSF
                y_pos = float(values[3])/YSF
            if hab_str_pos > 0:
                print 'Front values = ',x_pos,y_pos
                reducer.set_beam_finder(BaseBeamFinder(x_pos, y_pos),'front')
            else:
                reducer.set_beam_finder(BaseBeamFinder(x_pos, y_pos))

        elif upper_line.startswith('SET SCALES'):
            values = upper_line.split()
            reducer._corr_and_scale.rescale = \
                float(values[2])*reducer._corr_and_scale.DEFAULT_SCALING

        elif upper_line.startswith('SAMPLE/OFFSET'):
            values = upper_line.split()
            reducer.instrument.set_sample_offset(values[1])

        elif upper_line.startswith('DET/'):
            det_specif = upper_line[4:]
            if det_specif.startswith('CORR'):
                self._readDetectorCorrections(upper_line[8:], reducer)
            elif det_specif.startswith('RESCALE') or det_specif.startswith('SHIFT'):
                self._readFrontRescaleShiftSetup(det_specif, reducer)
            else:
                # for /DET/FRONT and /DET/REAR commands
                reducer.instrument.setDetector(det_specif)

        elif upper_line.startswith('GRAVITY'):
            flag = upper_line[8:].strip()
            if flag == 'ON' or flag == 'TRUE':
                reducer.to_Q.set_gravity(True, override=False)
            elif flag == 'OFF' or flag == 'FALSE':
                reducer.to_Q.set_gravity(False, override=False)
            else:
                _issueWarning("Gravity flag incorrectly specified, disabling gravity correction")
                reducer.to_Q.set_gravity(False, override=False)

        elif upper_line.startswith('FIT/TRANS/'):
            #check if the selector is passed:
            selector = 'BOTH'
            if 'SAMPLE' in upper_line:
                selector = 'SAMPLE'
                params = upper_line[17:].split() # remove FIT/TRANS/SAMPLE/
            elif 'CAN' in upper_line:
                selector = 'CAN'
                params = upper_line[14:].split() # remove FIT/TRANS/CAN/
            else:
                params = upper_line[10:].split() # remove FIT/TRANS/

            try:
                nparams = len(params)
                if nparams == 1:
                    fit_type = params[0]
                    lambdamin = lambdamax = None
                elif nparams == 3:
                    fit_type, lambdamin, lambdamax = params
                else:
                    raise 1
                reducer.transmission_calculator.set_trans_fit(min_=lambdamin, max_=lambdamax,
                                                              fit_method=fit_type, override=True,
                                                              selector=selector)
            except:
                _issueWarning('Incorrectly formatted FIT/TRANS line, %s, line ignored' % upper_line)

        elif upper_line.startswith('FIT/MONITOR'):
            params = upper_line.split()
            nparams = len(params)
            if nparams == 3 and reducer.instrument.name() == 'LOQ':
                reducer.transmission_calculator.loq_removePromptPeakMin = float(params[1])
                reducer.transmission_calculator.loq_removePromptPeakMax = float(params[2])
            else:
                if reducer.instrument.name() == 'LOQ':
                    _issueWarning('Incorrectly formatted FIT/MONITOR line, %s, line ignored' % upper_line)
                else:
                    _issueWarning('FIT/MONITOR line specific to LOQ instrument. Line ignored')

        elif upper_line == 'SANS2D' or upper_line == 'LOQ' or upper_line == 'LARMOR':
            self._check_instrument(upper_line, reducer)

        elif upper_line.startswith('PRINT '):
            _issueInfo(upper_line[6:])

        elif upper_line.startswith('SAMPLE/PATH'):
            flag = upper_line[12:].strip()
            if flag == 'ON' or flag == 'TRUE':
                reducer.wide_angle_correction = True
            else:
                reducer.wide_angle_correction = False

        elif line.startswith('!') or not line:
            # this is a comment or empty line, these are allowed
            pass

        else:
            _issueWarning('Unrecognized line in user file the line %s, ignoring' % upper_line)

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
    def readLimitValues(self, limit_line, reducer):
        limits = limit_line.split('L/')
        if len(limits) != 2:
            _issueWarning("Incorrectly formatted limit line ignored \"" + limit_line + "\"")
            return
        limits = limits[1]
        limit_type = ''

        if limits.startswith('SP '):
            # We don't use the L/SP line
            _issueWarning("L/SP lines are ignored")
            return

        if limits.upper().startswith('Q/RCUT'):
            limits = limits.upper().split('RCUT')
            if len(limits) != 2:
                _issueWarning("Badly formed L/Q/RCUT line")
            else:
                # When read from user file the unit is in mm but stored here it units of meters
                reducer.to_Q.r_cut = float(limits[1]) / 1000.0
            return
        if limits.upper().startswith('Q/WCUT'):
            limits = limits.upper().split('WCUT')
            if len(limits) != 2:
                _issueWarning("Badly formed L/Q/WCUT line")
            else:
                reducer.to_Q.w_cut = float(limits[1])
            return

        rebin_str = None
        if not ',' in limit_line:
            # Split with no arguments defaults to any whitespace character and in particular
            # multiple spaces are include
            elements = limits.split()
            if len(elements) == 4:
                limit_type, minval, maxval, step = elements[0], elements[1], elements[2], elements[3]
                step_details = step.split('/')
                if len(step_details) == 2:
                    step_size = step_details[0]
                    step_type = step_details[1]
                    if step_type.upper() == 'LOG':
                        step_type = '-'
                    else:
                        step_type = ''
                else:
                    step_size = step_details[0]
                    step_type = ''
            elif len(elements) == 3:
                limit_type, minval, maxval = elements[0], elements[1], elements[2]
            else:
                _issueWarning("Incorrectly formatted limit line ignored \"" + limit_line + "\"")
                return
        else:
            blocks = limits.split()
            limit_type = blocks[0].lstrip().rstrip()
            try:
                rebin_str = limits.split(limit_type)[1]
            except:
                _issueWarning("Incorrectly formatted limit line ignored \"" + limit_line + "\"")
                return

            minval = maxval = step_type = step_size = None

        if limit_type.upper() == 'WAV':
            if rebin_str:
                _issueWarning("General wave re-bin lines are not implemented, line ignored \"" + limit_line + "\"")
                return
            else:
                reducer.to_wavelen.set_rebin(\
                        minval, step_type + step_size, maxval, override=False)
        elif limit_type.upper() == 'Q':
            if rebin_str:
                reducer.to_Q.binning = rebin_str
            else:
                reducer.to_Q.binning = minval + "," + step_type + step_size + "," + maxval
        elif limit_type.upper() == 'QXY':
            reducer.QXY2 = float(maxval)
            reducer.DQXY = float(step_type + step_size)
        elif limit_type.upper() == 'R':
            reducer.mask.set_radi(minval, maxval)
            reducer.CENT_FIND_RMIN = float(minval)/1000.
            reducer.CENT_FIND_RMAX = float(maxval)/1000.
        elif (limit_type.upper() == 'PHI') or (limit_type.upper() == 'PHI/NOMIRROR'):
            mirror = limit_type.upper() != 'PHI/NOMIRROR'
            if maxval.endswith('/NOMIRROR'):
                maxval = maxval.split('/NOMIRROR')[0]
                mirror = False
            reducer.mask.set_phi_limit(
                float(minval), float(maxval), mirror, override=False)
        elif limit_type.upper() == 'EVENTSTIME':
            if rebin_str:
                reducer.settings["events.binning"] = rebin_str
            else:
                reducer.settings["events.binning"] = minval + "," + step_type + step_size + "," + maxval
        else:
            _issueWarning('Error in user file after L/, "%s" is not a valid limit line' % limit_type.upper())

    def _read_mon_line(self, details, reducer):

        #MON/LENTH, MON/SPECTRUM and MON/TRANS all accept the INTERPOLATE option
        interpolate = False
        interPlace = details.upper().find('/INTERPOLATE')
        if interPlace != -1:
            interpolate = True
            details = details[0:interPlace]

        if details.upper().startswith('SPECTRUM'):
            reducer.set_monitor_spectrum(
                int(details.split('=')[1]), interpolate, override=False)
            self._incid_monitor_lckd = True

        elif details.upper().startswith('LENGTH'):
            details = details.split('=')[1]
            options = details.split()
            spectrum = int(options[1])
#            reducer.instrument.monitor_zs[spectrum] = options[0]

            #the settings here are overriden by MON/SPECTRUM
            if not self._incid_monitor_lckd:
                reducer.set_monitor_spectrum(
                    spectrum, interpolate, override=False)

        elif details.upper().startswith('TRANS'):
            parts = details.split('=')
            if len(parts) < 2 or parts[0].upper() != 'TRANS/SPECTRUM' :
                return 'Unable to parse MON/TRANS line, needs MON/TRANS/SPECTRUM=... not: '
            reducer.set_trans_spectrum(int(parts[1]), interpolate, override=False)

        elif 'DIRECT' in details.upper() or details.upper().startswith('FLAT'):
            parts = details.split("=")
            if len(parts) == 2:
                filepath = parts[1].rstrip()
                #for VMS compatibility ignore anything in "[]", those are normally VMS drive specifications
                if '[' in filepath:
                    idx = filepath.rfind(']')
                    filepath = filepath[idx + 1:]
                if not os.path.isabs(filepath):
                    filepath = os.path.join(reducer.user_file_path, filepath)

                # If a filepath has been provided, then it must exist to continue.
                if filepath and not os.path.isfile(filepath):
                    raise RuntimeError("The following MON/DIRECT datafile does not exist: %s" % filepath)

                type = parts[0]
                parts = type.split("/")
                if len(parts) == 1:
                    if parts[0].upper() == 'DIRECT':
                        reducer.instrument.cur_detector().correction_file \
                            = filepath
                        reducer.instrument.other_detector().correction_file \
                           = filepath
                    elif parts[0].upper() == 'HAB':
                        try:
                            reducer.instrument.getDetector('HAB').correction_file \
                                = filepath
                        except AttributeError:
                            raise AttributeError('Detector HAB does not exist for the current instrument, set the instrument to LOQ first')
                    elif parts[0].upper() == 'FLAT':
                        reducer.prep_normalize.setPixelCorrFile(filepath,'REAR')
                    else:
                        pass
                elif len(parts) == 2:
                    detname = parts[1]
                    if detname.upper() == 'REAR':
                        if parts[0].upper() == "FLAT":
                            reducer.prep_normalize.setPixelCorrFile(filepath,'REAR')
                        else:
                            reducer.instrument.getDetector('REAR').correction_file \
                                = filepath
                    elif detname.upper() == 'FRONT' or detname.upper() == 'HAB':
                        if parts[0].upper() == "FLAT":
                            reducer.prep_normalize.setPixelCorrFile(filepath,'FRONT')
                        else:
                            reducer.instrument.getDetector('FRONT').correction_file \
                                = filepath
                    else:
                        return 'Incorrect detector specified for efficiency file: '
                else:
                    return 'Unable to parse monitor line: '
            else:
                return 'Unable to parse monitor line: '
        else:
            return 'Unable to parse monitor line: '

    def _readDetectorCorrections(self, details, reducer):
        """
            Handle user commands of the type DET/CORR/FRONT/RADIUS x
            @param details: the contents of the line after DET/CORR
            @param reducer: the object that contains all the settings
        """
        if details[0]=='/':
            details = details.lstrip('/')
        values = details.split()
        if '/' in values[0]:
            # assume notation is e.g. FRONT/RADIUS x
            values2 = values[0].split('/')
            det_name = values2[0]
            det_axis = values2[1]
            shift = float(values[1])
        else:
            # assume notation is e.g. FRONT RADIUS x
            det_name = values[0]
            det_axis = values[1]
            shift = float(values[2])

        detector = reducer.instrument.getDetector(det_name)
        if det_axis == 'X':
            detector.x_corr = shift
        elif det_axis == 'Y':
            detector.y_corr = shift
        elif det_axis == 'Z':
            detector.z_corr = shift
        elif det_axis == 'ROT':
            detector.rot_corr = shift
        # 21/3/12 RKH added 2 variables
        elif det_axis == 'RADIUS':
            detector.radius_corr = shift
        elif det_axis == 'SIDE':
            detector.side_corr = shift
		# 10/03/15 RKH add 2 more variables
        elif det_axis == 'XTILT':
            detector.x_tilt = shift
        elif det_axis == 'YTILT':
            detector.y_tilt = shift
        else:
            raise NotImplemented('Detector correction on "'+det_axis+'" is not supported')

    def _readFrontRescaleShiftSetup(self, details, reducer):
        """
            Handle user commands of the type DET/RESCALE r and DET/RESCALE/FIT q1 q2
            which are used to scale+constant background shift front detector so that
            data from the front and rear detectors can be merged

            @param details: the contents of the line after DET/
            @param reducer: the object that contains all the settings
        """
        values = details.split()
        rAnds = reducer.instrument.getDetector('FRONT').rescaleAndShift
        rAnds.qRangeUserSelected = False
        if details.startswith('RESCALE'):
            if 'FIT' in details:
                if len(values) == 1:
                    rAnds.fitScale = True
                elif len(values) == 3:
                    rAnds.fitScale = True
                    rAnds.qMin = float(values[1])
                    rAnds.qMax = float(values[2])
                    rAnds.qRangeUserSelected = True
                else:
                    _issueWarning("Command: \"DET/" + details + "\" not valid. Expected format is /DET/RESCALE/FIT [q1 q2]")
            else:
                if len(values) == 2:
                    rAnds.scale = float(values[1])
                else:
                    _issueWarning("Command: \"DET/" + details + "\" not valid. Expected format is /DET/RESCALE r")
        elif details.startswith('SHIFT'):
            if 'FIT' in details:
                if len(values) == 1:
                    rAnds.fitShift = True
                elif len(values) == 3:
                    rAnds.fitShift = True
                    rAnds.qMin = float(values[1])
                    rAnds.qMax = float(values[2])
                    rAnds.qRangeUserSelected = True
                else:
                    _issueWarning("Command: \"DET/" + details + "\" not valid. Expected format is /DET/SHIFT/FIT [q1 q2]")
            else:
                if len(values) == 2:
                    rAnds.shift = float(values[1])
                else:
                    _issueWarning("Command: \"DET/" + details + "\" not valid. Expected format is /DET/RESCALE r")

    def _read_back_line(self, arguments, reducer):
        """
            Parses a line from the settings file
            @param arguments: the contents of the line after the first keyword
            @param reducer: the object that contains all the settings
            @return any errors encountered or ''
        """
        #a list of the key words this function can read and the functions it calls in response
        keys = ['MON/TIMES', 'M']
        funcs = [self._read_default_back_region, self._read_back_region]
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
            parts = arguments.split('/OFF')
            if len(parts) == 2:
                # set specific monitor to OFF
                reducer.inst.set_TOFs(None, None, int(parts[0]))
                return ''

            # assume a line of the form BACK/M1/TIMES
            parts = arguments.split('/TIMES')
            if len(parts) == 2:
                times = parts[1].split()
            else:
                #try the other possibility, something like, BACK/M2
                parts =  arguments.split()
                times = [parts[1], parts[2]]

            monitor = int(parts[0])

            # parse the words after 'TIME' as first the start time and then the end
            reducer.inst.set_TOFs(int(times[0]), int(times[1]), monitor)
            return ''
        except Exception, reason:
            # return a description of any problems and then continue to read the next line
            return str(reason) + ' on line: '

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
            return 'Only monitor specific backgrounds will be applied, no default is set due to incorrectly formatted background line:'

    def _read_trans_line(self, arguments, reducer):
        #a list of the key words this function can read and the functions it calls in response
        keys = ['TRANSPEC', 'SAMPLEWS', 'CANWS']
        funcs = [self._read_transpec, self._read_trans_samplews, self._read_trans_canws]
        return self._process(keys, funcs, arguments, reducer)

    def _process(self, keys, funcs, params, reducer):
        #go through the list of recognised commands
        for i in range(0, len(keys)):
            if params.startswith(keys[i]):
                #remove the keyword as it has already been parsed
                params = params[len(keys[i]):]
                #call the handling function for that keyword returning any error
                return funcs[i](params, reducer)
        return 'Unrecognised line: '

    def _read_transpec(self, arguments, reducer):
        arguments = arguments.split('/')

        #check if there is an optional shift specification
        if len(arguments) == 2:
            #deal with the shift specification first
            shift = arguments[1]
            terms = shift.split('=')
            if len(terms) < 2:
                return 'Bad TRANS/TRANSPEC= / line: '
            reducer.instrument.monitor_4_offset= float(terms[1])

        #now remove any shift specification and parse the first argument
        arguments = arguments[0]
        arguments = arguments.split('=')
        if len(arguments) == 1:
            raise RuntimeError('An "=" is required after TRANSPEC')

        reducer.transmission_calculator.trans_spec = int(arguments[1])

    def _read_trans_samplews(self, arguments, reducer):
        if arguments.find('=') > -1:
            arguments = arguments.split('=')
        else:
            arguments = arguments.split()

        if len(arguments) != 2:
            return 'Unrecognised line: '

        reducer.transmission_calculator.calculated_samp = arguments[1]

    def _read_trans_canws(self, arguments, reducer):
        if arguments.find('=') > -1:
            arguments = arguments.split('=')
        else:
            arguments = arguments.split()

        if len(arguments) != 2:
            return 'Unrecognised line: '

        reducer.transmission_calculator.calculated_can = arguments[1]

    def _check_instrument(self, inst_name, reducer):
        if reducer.instrument is None:
            raise RuntimeError('Use SANS2D() or LOQ() to set the instrument before Maskfile()')
        if not inst_name == reducer.instrument.name():
            raise RuntimeError('User settings file not compatible with the selected instrument '+reducer.instrument.name())

    def _restore_defaults(self, reducer):
        reducer.mask.parse_instruction(reducer.instrument.name(), 'MASK/CLEAR')
        reducer.mask.parse_instruction(reducer.instrument.name(), 'MASK/CLEAR/TIME')

        reducer.CENT_FIND_RMIN = reducer.CENT_FIND_RMAX
        reducer.QXY = None
        reducer.DQY = None

        reducer.to_Q.binning = None

        # Scaling values
        reducer._corr_and_scale.rescale = 100.  # percent

        reducer.inst.reset_TOFs()

    def _read_calibfile_line(self, arguments, reducer):
        # remove the equals from the beggining and any space around.
        parts = re.split("\s?=\s?", arguments)
        if len(parts) != 2:
            return "Invalid input for TUBECALIBFILE" + str(arguments)+ ". Expected TUBECALIBFILE = file_path"
        path2file = parts[1]

        try:
            file_path, suggested_name = getFileAndName(path2file)
            __calibrationWs = Load(file_path, OutputWorkspace=suggested_name)
            reducer.instrument.setCalibrationWorkspace(__calibrationWs)
        except:
            # If we throw a runtime here, then we cannot execute 'Load Data'.
            raise RuntimeError("Invalid input for tube calibration file (" + path2file + " ).\n" \
            "Please do not run a reduction as it will not successfully complete.\n")

    def _read_maskfile_line(self, line, reducer):
        try:
            _, value = re.split("\s?=\s?", line)
        except ValueError:
            return "Invalid input: \"%s\".  Expected \"MASKFILE = path to file\"." % line

        reducer.settings["MaskFiles"] = value

class GetOutputName(ReductionStep):
    def __init__(self):
        """
            Reads a SANS mask file
        """
        super(GetOutputName, self).__init__()
        self.name_holder = ['problem_setting_name']

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
        ReplaceSpecialValues(InputWorkspace = workspace,OutputWorkspace = workspace, NaNValue="0", InfinityValue="0")

def _padRunNumber(run_no, field_width):
    nchars = len(run_no)
    digit_end = 0
    for i in range(0, nchars):
        if run_no[i].isdigit():
            digit_end += 1
        else:
            break

    if digit_end == nchars:
        filebase = run_no.rjust(field_width, '0')
        return filebase, run_no
    else:
        filebase = run_no[:digit_end].rjust(field_width, '0')
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

    def execute(self, reducer, workspace):
        """
            Trips leading and trailing Nan values from workspace
            @param reducer: unused
            @param workspace: the workspace to convert
        """
        result_ws = mtd[workspace]
        if result_ws.getNumberHistograms() != 1:
            #Strip zeros is only possible on 1D workspaces
            return

        y_vals = result_ws.readY(0)
        length = len(y_vals)
        # Find the first non-zero value
        start = 0
        for i in range(0, length):
            if not self._isNan(y_vals[i]):
                start = i
                break
        # Now find the last non-zero value
        stop = 0
        length -= 1
        for j in range(length, 0,-1):
            if not self._isNan(y_vals[j]):
                stop = j
                break
        # Find the appropriate X values and call CropWorkspace
        x_vals = result_ws.readX(0)
        startX = x_vals[start]
        # Make sure we're inside the bin that we want to crop
        endX = 1.001*x_vals[stop + 1]
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
    _shape_ids = {1 : 'cylinder-axis-up',
                  2 : 'cuboid',
                  3 : 'cylinder-axis-along'}
    _default_shape = 'cylinder-axis-along'

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
        if attrib == 'shape':
            return self._default_shape
        elif attrib == 'width' or attrib == 'thickness' or attrib == 'height':
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

        #check that the dimensions that we have make sense for our new shape
        if self._width:
            self.width = self._width
        if self._thickness:
            self.thickness = self._thickness

    def get_shape(self):
        if self._shape is None:
            return self._get_default('shape')
        else:
            return self._shape

    def set_width(self, width):
        self._width = float(width)
        self._use_wksp_width = False
        # For a disk the height=width
        if self._shape and self._shape.startswith('cylinder'):
            self._height = self._width
            self._use_wksp_height = False

    def get_width(self):
        self.raise_if_zero(self._width, "width")
        if self._width is None:
            return self._get_default('width')
        else:
            return self._width

    def set_height(self, height):
        self._height = float(height)
        self._use_wksp_height = False

        # For a cylinder and sphere the height=width=radius
        if (not self._shape is None) and (self._shape.startswith('cylinder')):
            self._width = self._height
        self._use_wksp_width = False

    def get_height(self):
        self.raise_if_zero(self._height, "height")
        if self._height is None:
            return self._get_default('height')
        else:
            return self._height

    def set_thickness(self, thickness):
        """
            Simply sets the variable _thickness to the value passed
        """
        #as only cuboids use the thickness the warning below may be informative
        #if (not self._shape is None) and (not self._shape == 'cuboid'):
        #    mantid.sendLogMessage('::SANS::Warning: Can\'t set thickness for shape "'+self._shape+'"')
        self._thickness = float(thickness)
        self._use_wksp_thickness = False

    def get_thickness(self):
        self.raise_if_zero(self._thickness, "thickness")
        if self._thickness is None:
            return self._get_default('thickness')
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
        return '-- Sample Geometry --\n' + \
               '    Shape: ' + self.shape+'\n'+\
               '    Width: ' + str(self.width)+'\n'+\
               '    Height: ' + str(self.height)+'\n'+\
               '    Thickness: ' + str(self.thickness)+'\n'

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
        assert  issubclass(geo.__class__, GetSampleGeom)

        try:
            if geo.shape == 'cylinder-axis-up':
                # Volume = circle area * height
                # Factor of four comes from radius = width/2
                volume = geo.height*math.pi
                volume *= math.pow(geo.width,2)/4.0
            elif geo.shape == 'cuboid':
                # Flat plate sample
                volume = geo.width
                volume *= geo.height*geo.thickness
            elif geo.shape == 'cylinder-axis-along':
                # Factor of four comes from radius = width/2
                # Disc - where height is not used
                volume = geo.thickness*math.pi
                volume *= math.pow(geo.width, 2)/4.0
            else:
                raise NotImplemented('Shape "'+geo.shape+'" is not in the list of supported shapes')
        except TypeError:
            raise TypeError('Error calculating sample volume with width='+str(geo.width) + ' height='+str(geo.height) + 'and thickness='+str(geo.thickness))

        return volume

    def execute(self, reducer, workspace):
        """
            Divide the counts by the volume of the sample
        """
        if not reducer.is_can():
            # it calculates the volume for the sample and may or not apply to the can as well.
            self.volume = self.calculate_volume(reducer)

        ws = mtd[str(workspace)]
        ws /= self.volume
