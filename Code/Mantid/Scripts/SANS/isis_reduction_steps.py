"""
    This file defines what happens in each step in the data reduction, it's
    the guts of the reduction. See ISISReducer for order the steps are run
    in and the names they are given to identify them
    
    Most of this code is a copy-paste from SANSReduction.py, organized to be used with
    ReductionStep objects. The guts needs refactoring.
"""
from reduction import ReductionStep
import isis_reducer
import reduction.instruments.sans.sans_reduction_steps as sans_reduction_steps
from mantidsimple import *
import SANSUtility
import isis_instrument
import os
import math
import copy

#TODO: remove when center finding is working 
DEL__FINDING_CENTRE_ = False

def _issueWarning(msg):
    """
        Prints a message to the log marked as warning
        @param msg: message to be issued
    """
    print msg
    mantid.sendLogMessage('::SANS::Warning: ' + msg)

def _issueInfo(msg):
    """
        Prints a message to the log
        @param msg: message to be issued
    """
    print msg
    mantid.sendLogMessage(msg)


class LoadRun(ReductionStep):
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
        if int(entry) > 0:
            self._period = int(entry)
        else:
            self._period = self.UNSET_PERIOD

        self._spec_min = None
        self._spec_max = None
        self.ext = ''
        self.shortrun_no = -1
        
    def _load(self, inst = None, is_can=False):
        """
            Load a workspace and read the logs into the passed instrument reference
            @param inst: a reference to the current instrument
            @param iscan: set this to True for can runs 
            @return: name of the workspace, log values, number of periods in the workspace
        """
        workspace = self._get_workspace_name()

        
        if os.path.splitext(self._data_file)[1].lower().startswith('.r'):
            # MG - 2011-02-24: Temporary fix to load .sav or .s* files. Lets the file property
            # work it out
            file_hint = os.path.splitext(self._data_file)[0]
            #raw files have some different options
            alg = LoadRaw(file_hint, workspace, SpectrumMin=self._spec_min, SpectrumMax=self._spec_max)
            self._data_file = alg.getPropertyValue("Filename")
            LoadSampleDetailsFromRaw(workspace, self._data_file)
            #if the user didn't specify a period use the first period
            if self._period != self.UNSET_PERIOD:
                workspace = self._leaveSinglePeriod(workspace)

            log_file = alg.getPropertyValue("Filename")

            base_name = os.path.splitext(log_file)[0]
            if base_name.endswith('-add'):
                #remove the add files specifier, if it's there
                base_name = base_name.rpartition('-add')[0]
            SANS2D_log_file = base_name+'.log'
            
        else:
            #a particular entry was selected just load that one
            if not self._period == self.UNSET_PERIOD:
                alg = LoadNexus(self._data_file, workspace,
                  SpectrumMin=self._spec_min, SpectrumMax=self._spec_max, EntryNumber=self._period)
            else:
                #no specific period was requested
                alg = LoadNexus(self._data_file, workspace,
                  SpectrumMin=self._spec_min, SpectrumMax=self._spec_max)

            SANS2D_log_file = mtd[workspace]

            #get rid of these two lines when files store their logs properly
            log_file = alg.getPropertyValue("Filename")
            base_name = os.path.splitext(log_file)[0]
            if base_name.endswith('-add'):
                #remove the add files specifier, if it's there
                base_name = base_name.rpartition('-add')[0]
            SANS2D_log_file = base_name+'.log'

       
        numPeriods  = self._find_workspace_num_periods(workspace)
        #deal with the difficult situation of not reporting the period of single period files
        if numPeriods > 1 and self._period == 1:
            #the string "workspace" contains the period number only if it is greater than 1, this always contains the period number
            period_definitely_inc = self._get_workspace_name(False)
            RenameWorkspace(workspace, period_definitely_inc)
            workspace = period_definitely_inc 
        
        log = None
        if (not inst is None) and inst.name() == 'SANS2D':
            #this instrument has logs to be loaded 
            try:
                if numPeriods > 1:
                    log = inst.get_detector_log(SANS2D_log_file, self._period)
                else:
                    log = inst.get_detector_log(SANS2D_log_file)
            except:
                #transmission files, don't have logs 
                if not self._is_trans:
                    raise

        return workspace, numPeriods, log        

    def _get_workspace_name(self, optional_entry_no=True):
        """
            Creates a name for the workspace that will contain the raw
            data. If the entry number == 1 it is omitted, unless
            optional_entry_no = False
            @param optional_entry_no: include the entry number even if it's 1, default: false
        """  
        run = str(self.shortrun_no)
        if self._period > self.UNSET_PERIOD:
            if (int(self._period) != 1) or (not optional_entry_no):
                run += 'p'+str(self._period)
        
        if self._is_trans:
            return run + '_trans_' + self.ext.lower()
        else:
            return run + '_sans_' + self.ext.lower()

    # Helper function
    def _assignHelper(self, reducer):
        if self._data_file == '' or self._data_file.startswith('.'):
            return '', '', -1
        
        try:
            data_file = self._extract_run_details(
                self._data_file, self._is_trans, prefix=reducer.instrument.name(), 
                run_number_width=reducer.instrument.run_number_width, period=self._period)
        except AttributeError:
            raise AttributeError('No instrument has been assign, run SANS2D or LOQ first')

        workspace = self._get_workspace_name()
        #this always contains the period number
        period_definitely_inc = self._get_workspace_name(False)
        if self._reload == False and mantid.workspaceExists(workspace):
            return workspace, '', -1
        if self._reload == False and mantid.workspaceExists(period_definitely_inc):
            return period_definitely_inc, '', -1

        self._data_file = os.path.join(reducer._data_path, data_file)
        # Workaround so that the FileProperty does the correct searching of data paths if this file doesn't exist
        if not os.path.exists(self._data_file):
            self._data_file = data_file

        if self._is_trans:
            try:
                if reducer.instrument.name() == 'SANS2D' and int(self.shortrun_no) < 568:
                    dimension = SANSUtility.GetInstrumentDetails(reducer.instrument)[0]
                    self._spec_min = dimension*dimension*2
                    self._spec_max = self._spec_min + 4
                else:
                    self._spec_min = None
                    self._spec_max = 8
                wkspname, nPeriods, logs = self._load(reducer.instrument)
            except RuntimeError, err:
                mantid.sendLogMessage("::SANS::Warning: "+str(err))
                return '', '', -1
        else:
            try:
                wkspname, nPeriods, logs = self._load(reducer.instrument)
            except RuntimeError, details:
                mantid.sendLogMessage("::SANS::Warning: "+str(details))
                return '', '', -1
        
        return wkspname, nPeriods, logs

    def _leaveSinglePeriod(self, workspace):
        groupW = mantid[workspace]
        if groupW.isGroup():
            num_periods = groupW.getNames()
        else:
            num_periods = 1

        if (self._period > num_periods) or (self._period < 1):
            raise ValueError('_loadRawData: Period number ' + str(self._period) + ' doesn\'t exist in workspace ' + groupW.getName())
        
        if num_periods == 1:
            return workspace
        #get the name of the individual workspace in the group
        oldName = groupW.getName()+'_'+str(self._period)
        #move this workspace out of the group (this doesn't delete it)
        groupW.remove(oldName)
    
        discriptors = groupW.getName().split('_')       #information about the run (run number, if it's 1D or 2D, etc) is listed in the workspace name between '_'s
        for i in range(0, len(discriptors) ):           #insert the period name after the run number
            if i == 0 :                                 #the run number is the first part of the name
                newName = discriptors[0]+'p'+str(self._period)#so add the period number here
            else :
                newName += '_'+discriptors[i]
    
        if oldName != newName:
            RenameWorkspace(oldName, newName)
    
        #remove the rest of the group
        mantid.deleteWorkspace(groupW.getName())
        return newName
    
    def _clearPrevious(self, inWS, others = []):
        if inWS != None:
            if mantid.workspaceExists(inWS) and (not inWS in others):
                mantid.deleteWorkspace(inWS)
                

    def _extract_run_details(self, run_string, is_trans=False, prefix='', run_number_width=8, period=-1):
        """
            Takes a run number and file type and generates the filename, workspace name and log name
            @param run_string either the name of a run file or a run number followed by a dot and then the file type, i.e. file extension
            @param is_trans true for transmission files, false for sample files (default is false)
        """
        pieces = run_string.split('.')
        if len(pieces) != 2 :
             raise RuntimeError, "Invalid run specified: " + run_string + ". Please use RUNNUMBER.EXT format"
        
        #get a consistent format for path names, the Linux/Mac version
        if run_string.find('\\') > -1 and run_string.find('/') == -1:
            #means we have windows style their paths contain \ but can't contain /   
            run_string = run_string.replace('\\', '/')

        if run_string.find('/') > -1:
            #assume we have a complete filename
            filename = run_string
            #remove the path name
            run_name = run_string.rpartition('/')[2]
            #remove the extension
            file_parts = run_name.rpartition('.')
            run_name = file_parts[0]
            self.ext = file_parts[2]
            if run_name.endswith('-add'):
                #remove the add files specifier, if it's there
                run_name = run_name.rpartition('-add')[0]
            self.shortrun_no = str(run_name.partition(prefix)[2])
        else:
            #this is a run number dot extension
            run_no = pieces[0]
            self.ext = pieces[1]
            fullrun_no, self.shortrun_no = _padRunNumber(run_no, run_number_width)
            filename = prefix+fullrun_no+'.'+self.ext
            
        self.shortrun_no = int(self.shortrun_no)

        return filename 
    
    def _find_workspace_num_periods(self, workspace): 
        """
            Deal with selection and reporting of periods in multi-period files,
            this is complicated because different file formats have different ways
            of report numbers of periods
            @param workspace: the name of the workspace
        """
        numPeriods = -1
        pWorksp = mantid[workspace]
        if pWorksp.isGroup() :
            #get the number of periods in a group using the fact that each period has a different name
            numPeriods = len(pWorksp.getNames())
        else :
            if self._period == self.UNSET_PERIOD:
                #they didn't specify a period but there is only one period, the original file must contain no more than one period 
                numPeriods = 1
        #the logs have the definitive information on the number of periods, if it is in the logs
        try:
            samp = pWorksp.getSampleDetails()
            numPeriods = samp.getLogData('nperiods').value
        except:
            #it's OK for there not to be any logs
            pass
        
        return numPeriods


class LoadTransmissions(sans_reduction_steps.BaseTransmission):
    """
        Loads the file used to apply the transmission correction to the
        sample or can 
    """

    def __init__(self, is_can=False, reload=True):
        """
            Two settings can be set at initialization, if this is for
            can and if the workspaces should be reloaded if they already
            exist
            @param is_can: if this is to correct the can (default false i.e. it's for the sample)
            @param reload: setting this to false will mean the workspaces aren't reloaded if they already exist (default True i.e. reload)
        """
        super(LoadTransmissions, self).__init__()
        self.trans_name = None
        self.direct_name = None
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
            load = LoadRun(self._trans_name, trans=True, reload=self._reload, entry=self._period_t)
            self.trans_name, self.TRANS_SAMPLE_N_PERIODS, dummy = \
                                        load._assignHelper(reducer)
        
        if self._direct_name not in [None, '']:
            load = LoadRun(self._direct_name, trans=True, reload=self._reload, entry=self._period_d)
            self.direct_name, self.DIRECT_SAMPLE_N_PERIODS, dummy = \
                                        load._assignHelper(reducer)

        return self.trans_name, self.direct_name

class CanSubtraction(LoadRun):
    """
        Subtract the can after correcting it.
        Note that in the original SANSReduction.py, the can run was loaded immediately after
        the AssignCan() command was called. Since the loading needs information from the instrument, 
        we load only before doing the subtraction.
    """
    def __init__(self, can_run, reload = True, period = -1):
        """
            @param lambda_min: MinWavelength parameter for CalculateTransmission
            @param lambda_max: MaxWavelength parameter for CalculateTransmission
            @param fit_method: FitMethod parameter for CalculateTransmission (Linear or Log)
        """
        super(CanSubtraction, self).__init__(can_run, reload=reload, entry=period)
        self._scatter_can = None

    def assign_can(self, reducer, reload = True):
        #TODO: get rid of any reference to the instrument object as much as possible
        # Definitely get rid of the if-statements checking the instrument name.
        if not issubclass(reducer.instrument.__class__, isis_instrument.ISISInstrument):
            raise RuntimeError, "CanSubtraction.assign_can expects an argument of class ISISInstrument"
        
        if( self._data_file.startswith('.') or self._data_file == '' or self._data_file == None):
            self._data_file = None
            return '', '()'

            return '', '()'
    
        self._scatter_can, self._CAN_N_PERIODS, logs = self._assignHelper(
                                                                 reducer)
        if self._scatter_can == '':
            mantid.sendLogMessage('::SANS::Warning: Unable to load SANS can run, cannot continue.')
            return '','()'
    
        if reducer.instrument.name() == 'LOQ':
            return self._scatter_can, ""

        if reducer.instrument.name() == 'SANS2D':
            if logs is None:
                _issueWarning("Can logs could not be loaded, using sample values.")
                return self._scatter_can, "()"
            reducer.instrument.check_can_logs(logs)           
        
        return self._scatter_can, logs

    def execute(self, reducer, workspace):
        """
            Apply same corrections as for data then subtract from data
        """
        if not self._data_file:
            return

        beamcoords = reducer._beam_finder.get_beam_center()

        # Put the components in the correct positions
        reducer.instrument.set_component_positions(
                self._scatter_can, beamcoords[0], beamcoords[1])
        mantid.sendLogMessage('::SANS:: Initialized can workspace to [' + str(beamcoords[0]) + ',' + str(beamcoords[1]) + ']' )

        # Create a run details object
        TRANS_CAN = ''
        DIRECT_CAN = ''
        if reducer._transmission_calculator is not None:
            TRANS_CAN = reducer._transmission_calculator.TRANS_CAN
        if reducer._transmission_calculator is not None:
            DIRECT_CAN = reducer._transmission_calculator.DIRECT_CAN
            
        finding_centre = False
        
        
        tmp_smp = workspace+"_sam_tmp"
        RenameWorkspace(workspace, tmp_smp)

        tmp_can = workspace+"_can_tmp"

        # Can correction
        #replaces Correct(can_setup, wav_start, wav_end, use_def_trans, finding_centre)
        reduce_can = copy.deepcopy(reducer)

        #the workspace is again branched with a new name
        reduce_can.flood_file.out_container[0] = tmp_can
        #the line below is required if the step above is optional
        reduce_can.crop_detector.out_container[0] = tmp_can
        if not reducer.transmission_calculator is None:
            reduce_can.transmission_calculator.set_loader(reduce_can.can_trans_load)

        norm_step_ind = reduce_can.step_num(reduce_can.norm_mon)
        reduce_can.norm_mon = NormalizeToMonitor(
                                        raw_ws = self._scatter_can)
        reduce_can._reduction_steps[norm_step_ind] = reduce_can.norm_mon
           
        #set the workspace that we've been setting up as the one to be processed 
        reduce_can.set_process_single_workspace(self._scatter_can)

        #this will be the first command that is run in the new chain
        start = reduce_can.step_num(reduce_can.flood_file)
        #stop before this current step
        end = reducer.step_num(self)-1
        #the reducer is completely setup, run it
        reduce_can.run_steps(start_ind=start, stop_ind=end)
        

        #we now have the can workspace, use it
        Minus(tmp_smp, tmp_can, workspace)
    
#        if DEL__FINDING_CENTRE_:
#            mantid.deleteWorkspace(tmp_smp)
#            mantid.deleteWorkspace(tmp_can)
#            mantid.deleteWorkspace(workspace)        
#        else:
        #clean up the workspaces ready users to see them if required
        # Due to rounding errors, small shifts in detector encoders and poor stats in highest Q bins need "minus" the
        # workspaces before removing nan & trailing zeros thus, beware,  _sc,  _sam_tmp and _can_tmp may NOT have same Q bins
        ReplaceSpecialValues(InputWorkspace = tmp_smp,OutputWorkspace = tmp_smp, NaNValue="0", InfinityValue="0")
        ReplaceSpecialValues(InputWorkspace = tmp_can,OutputWorkspace = tmp_can, NaNValue="0", InfinityValue="0")
        if reducer.to_Q.output_type == '1D':
             rem_zeros = sans_reduction_steps.StripEndZeros()
             rem_zeros.execute(reducer, tmp_smp)
             rem_zeros.execute(reducer, tmp_can)
    
class Mask_ISIS(sans_reduction_steps.Mask):
    """
        Provides ISIS specific mask functionality (e.g. parsing
        MASK commands from user files), inherits from Mask
    """
    def __init__(self, timemask='', timemask_r='', timemask_f='', 
                 specmask='', specmask_r='', specmask_f=''):
        sans_reduction_steps.Mask.__init__(self)
        self.time_mask=timemask 
        self.time_mask_r=timemask_r
        self.time_mask_f=timemask_f
        self.spec_mask_r=specmask_r
        self.spec_mask_f=specmask_f

        self._lim_phi_xml = ''
        self.phi_min = -90.0
        self.phi_max = 90.0
        self.phi_mirror = True
        self._readonly_phi = False

        ########################## Masking  ################################################
        # Mask the corners and beam stop if radius parameters are given

        self.min_radius = None
        self.max_radius = None

    def set_radi(self, min, max):
        self.min_radius = float(min)/1000.
        self.max_radius = float(max)/1000.

    def parse_instruction(self, details):
        """
            Parse an instruction line from an ISIS mask file
        """
        details = details.lstrip()
        details = details.upper()
        if not details.startswith('MASK'):
            _issueWarning('Ignoring malformed mask line ' + details)
            return
        
        parts = details.split('/')
        # A spectrum mask or mask range applied to both detectors
        if len(parts) == 1:
            #by default only the rear detector is masked
            self.add_mask_string(details[4:].lstrip(), detect='rear')
        elif len(parts) == 2:
            type = parts[1]
            detname = type.split()
            if type == 'CLEAR':
                self.spec_mask_r = ''
                self.spec_mask_f = ''
            elif type.startswith('T'):
                if type.startswith('TIME'):
                    bin_range = type[4:].lstrip()
                else:
                    bin_range = type[1:].lstrip()
                self.time_mask += ';' + bin_range
            elif len(detname) == 2:
                self.add_mask_string(mask_string=detname[1],detect=detname[0])
            else:
                _issueWarning('Unrecognized masking option "' + details + '"')
        elif len(parts) == 3:
            type = parts[1]
            if type == 'CLEAR':
                self.time_mask = ''
                self.time_mask_r = ''
                self.time_mask_f = ''
            elif (type == 'TIME' or type == 'T'):
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
        if detect.upper() == 'FRONT':
            self.spec_mask_f += ',' + mask_string
        elif detect.upper() == 'REAR':
            self.spec_mask_r += ',' + mask_string
        else:
            _issueWarning('Detector \'' + detect + '\' not found in currently selected instrument ' + self.instrument.name() + '. Skipping line.')

    def _ConvertToSpecList(self, maskstring, detector):
        '''
            Convert a mask string to a spectra list
            6/8/9 RKH attempt to add a box mask e.g.  h12+v34 (= one pixel at intersection), h10>h12+v101>v123 (=block 3 wide, 23 tall)
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
                    speclist += detector.spectrum_block(low2, low,nstrips, 'all')+ ','
                else:
                    print "error in mask, ignored:  " + x
            elif '>' in x:
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
            elif 's' in x:
                speclist += x.lstrip('s') + ','
            elif x == '':
                #empty entries are allowed
                pass
            elif len(x.split()) == 4:
                _issueWarning('Box mask entry "%s" ignored. Box masking is not supported by Mantid'%('mask '+x))
            else:
                raise SyntaxError('Problem reading a mask entry: "%s"' % x)
        
        return speclist.rpartition(',')[0]

    def _mask_phi(self, id, centre, phimin, phimax, use_mirror=True):
        '''
            Mask the detector bank such that only the region specified in the
            phi range is left unmasked
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
        self._lim_phi_xml = (
            self._infinite_plane(id+'_plane1',centre, [math.cos(-phimin + math.pi/2.0),math.sin(-phimin + math.pi/2.0),0])
            + self._infinite_plane(id+'_plane2',centre, [-math.cos(-phimax + math.pi/2.0),-math.sin(-phimax + math.pi/2.0),0])
            + self._infinite_plane(id+'_plane3',centre, [math.cos(-phimax + math.pi/2.0),math.sin(-phimax + math.pi/2.0),0])
            + self._infinite_plane(id+'_plane4',centre, [-math.cos(-phimin + math.pi/2.0),-math.sin(-phimin + math.pi/2.0),0]))
        
        if use_mirror : 
            self._lim_phi_xml += '<algebra val="#(('+id+'_plane1 '+id+'_plane2):('+id+'_plane3 '+id+'_plane4))" />'
        else:
            #the formula is different for acute verses obstruse angles
            if phimax-phimin > math.pi :
              # to get an obtruse angle, a wedge that's more than half the area, we need to add the semi-inifinite volumes
                self._lim_phi_xml += '<algebra val="#('+id+'_plane1:'+id+'_plane2)" />'
            else :
              # an acute angle, wedge is more less half the area, we need to use the intesection of those semi-inifinite volumes
                self._lim_phi_xml += '<algebra val="#('+id+'_plane1 '+id+'_plane2)" />'

    def normalizePhi(self, phi):
        if phi > 90.0:
            phi -= 180.0
        elif phi < -90.0:
            phi += 180.0
        else:
            pass
        return phi

    def set_phi_limit(self, phimin, phimax, phimirror, override=True):
        if phimirror :
            if phimin > phimax:
                phimin, phimax = phimax, phimin
            if abs(phimin) > 180.0:
                phimin = -90.0
            if abs(phimax) > 180.0:
                phimax = 90.0
        
            if phimax - phimin == 180.0:
                self.phi_min = -90.0
                self.phi_max = 90.0
            else:
                self.phi_min = self.normalizePhi(phimin)
                self.phi_max = self.normalizePhi(phimax)
        else:
            self.phi_min = phimin
            self.phi_max = phimax

        self.phi_mirror = phimirror

        if override:
            self._readonly_phi = True
            
        if (not self._readonly_phi) or override:
            self._mask_phi(
                'unique phi', [0,0,0], self.phi_min,self.phi_max,self.phi_mirror)

    def execute(self, reducer, workspace, instrument=None, xcentre=None, ycentre=None):
        if instrument is None:
            instrument = reducer.instrument
        #set up the spectra lists and shape xml to mask
        detector = instrument.cur_detector()
        if detector.isAlias('rear'):
            self.spec_list = self._ConvertToSpecList(self.spec_mask_r, detector)
            #Time mask
            SANSUtility.MaskByBinRange(workspace,self.time_mask_r)
            SANSUtility.MaskByBinRange(workspace,self.time_mask)

        if detector.isAlias('front'):
            #front specific masking
            self.spec_list = self._ConvertToSpecList(self.spec_mask_f, detector)
            #Time mask
            SANSUtility.MaskByBinRange(workspace,self.time_mask_f)
            SANSUtility.MaskByBinRange(workspace,self.time_mask)

        #reset the xml, as execute can be run more than once
        self._xml = []
        if DEL__FINDING_CENTRE_ == True:
            if ( not self.min_radius is None) and (self.min_radius > 0.0):
                self.add_cylinder(self.min_radius, self._maskpt_rmin[0], self._maskpt_rmin[1], 'center_find_beam_cen')
            if ( not self.max_radius is None) and (self.max_radius > 0.0):
                self.add_outside_cylinder(self.max_radius, self._maskpt_rmin[0], self._maskpt_rmin[1], 'center_find_beam_cen')
        else:
            if xcentre is None:
                detectorCentreX = reducer.place_det_sam.maskpt_rmax[0]
                beamCentreX = reducer.place_det_sam.maskpt_rmin[0]
            else:
                detectorCentreX = xcentre
                beamCentreX = xcentre
            if ycentre is None:
                detectorCentreY = reducer.place_det_sam.maskpt_rmax[1]
                beamCentreY = reducer.place_det_sam.maskpt_rmin[1]
            else:
                detectorCentreY = ycentre
                beamCentreY = ycentre

            if ( not self.min_radius is None) and (self.min_radius > 0.0):
                self.add_cylinder(self.min_radius, beamCentreX, beamCentreY,
                                                             'beam_stop')
            if ( not self.max_radius is None) and (self.max_radius > 0.0):
                self.add_outside_cylinder(self.max_radius, detectorCentreX,
                                            detectorCentreY, 'beam_area')
        #now do the masking
        sans_reduction_steps.Mask.execute(self, reducer, workspace, instrument)

        if self._lim_phi_xml != '':
            MaskDetectorsInShape(workspace, self._lim_phi_xml)

    def view(self, instrum):
        """
            Display the masked detectors in the bank in a different color
            in instrument view
            @param instrum: a reference an instrument object to view
        """
        wksp_name = 'CurrentMask'
        instrum.load_empty(wksp_name)

        #apply masking to the current detector
        self.execute(None, wksp_name, instrum, 0, 0)
        
        #now the other detector
        other = instrum.other_detector().name()
        original = instrum.cur_detector().name()
        instrum.setDetector(other)
        self.execute(None, wksp_name, instrum, 0, 0)
        #reset the instrument to mask the currecnt detector
        instrum.setDetector(original)

        # Mark up "dead" detectors with error value 
        FindDeadDetectors(wksp_name, wksp_name, DeadValue=500)

        #opens an instrument showing the contents of the workspace (i.e. the instrument with masked detectors) 
        instrum.view(wksp_name)

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
        super(LoadSample, self).__init__(sample, reload=reload, entry=entry)
        self._scatter_sample = None
        self._SAMPLE_RUN = None
        self._SAMPLE_N_PERIODS = -1
        
        self.maskpt_rmin = None
        
        #This is set to the name of the workspace that was loaded, with some changes made to it 
        self.uncropped = None
    
    def execute(self, reducer, workspace):
        # If we don't have a data file, look up the workspace handle
        if self._data_file is None:
            self._data_file = reducer.get_sample()
        # Code from AssignSample
        self._clearPrevious(self._scatter_sample)
        self._SAMPLE_N_PERIODS = -1
        
        if self._data_file.startswith('.') or ( not self._data_file ):
            self._SAMPLE_RUN = ''
            self._scatter_sample = None
            raise RuntimeError('Sample needs to be assigned as run_number.file_type')

        self._scatter_sample, self._SAMPLE_N_PERIODS, logs =\
            self._assignHelper(reducer)

        if self._scatter_sample == '':
            raise RuntimeError('Unable to load SANS sample run, cannot continue.')

        self.uncropped  = self._scatter_sample
        p_run_ws = mantid[self.uncropped]
        
        if p_run_ws.isGroup():
            p_run_ws = p_run_ws[0]
    
        try:
            run_num = p_run_ws.getSampleDetails().getLogData('run_number').value
        except RuntimeError:
            # if the run number is not stored in the workspace, take it from the filename
            run_num = self._data_file.split('.')[0].split('-')[0]
        
        reducer.instrument.set_up_for_run(run_num)

        if reducer.instrument.name() == 'SANS2D':
            if logs == None:
                mantid.deleteWorkspace(self._scatter_sample)
                raise RuntimeError('Sample logs cannot be loaded, cannot continue')
            reducer.instrument.apply_detector_logs(logs)           

        reducer.wksp_name = self.uncropped
        
        return reducer.wksp_name, logs

class MoveComponents(ReductionStep):
    """
        Moves the components so that the centre of the detector bank is at the location
        that was passed to the BeamFinder
    """
    def __init__(self):
        super(MoveComponents, self).__init__()
        #save where we have moved the workspace to so that we don't move it again
        self.moved_to = None

    def execute(self, reducer, workspace):

        # Put the components in the correct positions
        beamcoords = reducer._beam_finder.get_beam_center()
        if self.moved_to and self.moved_to != beamcoords:
            #the components had already been moved, return them to their original location first
            reducer.instrument.set_component_positions(workspace, -self.moved_to[0], -self.moved_to[1])
        
        if self.moved_to != beamcoords:
            self.maskpt_rmin, self.maskpt_rmax = reducer.instrument.set_component_positions(workspace, beamcoords[0], beamcoords[1])
            self.moved_to = beamcoords

        mantid.sendLogMessage('::SANS:: Moved sample workspace to [' + str(self.maskpt_rmin)+','+str(self.maskpt_rmax) + ']' )

class CropDetBank(ReductionStep):
    """
        Takes the spectra range of the current detector from the instrument object
        and crops the input workspace to just those spectra. Supports optionally
        changing the name of the output workspace, which is more efficient than
        running clone workspace to do that
    """ 
    def __init__(self, name_container=[]):
        """
            If a name is passed to this function this reduction step
            will branch to a new output workspace. The name could be
            GetOutputName object or a string
            @param name_change: an object that contains the new name
        """
        super(CropDetBank, self).__init__()
        self.out_container = name_container

    def execute(self, reducer, workspace):
        if len(self.out_container) > 0:
            reducer.wksp_name = self.out_container[0]
         # Get the detector bank that is to be used in this analysis leave the complete workspace
        CropWorkspace(workspace, reducer.wksp_name,
            StartWorkspaceIndex = reducer.instrument.cur_detector().get_first_spec_num() - 1,
            EndWorkspaceIndex = reducer.instrument.cur_detector().last_spec_num - 1)

class ConvertToQ(ReductionStep):
    _OUTPUT_TYPES = {'1D' : 'Q1D', '2D': 'Qxy'}
    _DEFAULT_GRAV = False
    
    def __init__(self, type = '1D'):
        super(ConvertToQ, self).__init__()
        
        #this should be set to 1D or 2D
        self._output_type = None
        #the algorithm that corrosponds to the above choice
        self._Q_alg = None
        self.set_output_type(type)
        #if true gravity is taken into account in the Q1D calculation
        self._use_gravity = self._DEFAULT_GRAV
        self._grav_set = False
        
        self.error_est_1D = None
    
    def set_output_type(self, discript):
        self._Q_alg = self._OUTPUT_TYPES[discript]
        self._output_type = discript
        
    def get_output_type(self):
        return self._output_type

    output_type = property(get_output_type, set_output_type, None, None)

    def get_gravity(self):
        return self._use_gravity

    def set_gravity(self, flag, override=True):
        if override:
            self._grav_set = True
            
        if (not self._grav_set) or override:
                self._use_gravity = bool(flag)
        else:
            _issueWarning("User file can't override previous gravity setting, do gravity correction remains " + str(self._use_gravity)) 

    def execute(self, reducer, workspace):
        if self._Q_alg == 'Q1D':
            if self.error_est_1D is None:
                raise RuntimeError('Could not find the workspace containing error estimates')
            Q1D(workspace, self.error_est_1D, workspace, reducer.Q_REBIN, AccountForGravity=self._use_gravity)
            mtd.deleteWorkspace(self.error_est_1D)
            self.error_est_1D = None

        elif self._Q_alg == 'Qxy':
            Qxy(workspace, workspace, reducer.QXY2, reducer.DQXY, AccountForGravity=self._use_gravity)
            ReplaceSpecialValues(workspace, workspace, NaNValue="0", InfinityValue="0")
        else:
            raise NotImplementedError('The type of Q reduction hasn\'t been set, e.g. 1D or 2D')

class NormalizeToMonitor(sans_reduction_steps.Normalize):
    """
        Before normalisation the monitor spectrum's background is removed 
        and for LOQ runs also the prompt peak. The input workspace is copied
        and accessible later as prenomed 
    """
    def __init__(self, spectrum_number=None, raw_ws=None):
        if not spectrum_number is None:
            index_num = spectrum_number - 1
        else:
            index_num = None
        super(NormalizeToMonitor, self).__init__(index_num)
        self._raw_ws = raw_ws

    def execute(self, reducer, workspace):
        normalization_spectrum = self._normalization_spectrum 
        if normalization_spectrum is None:
            #the -1 converts from spectrum number to spectrum index
            normalization_spectrum = reducer.instrument.get_incident_mon()-1
        
        raw_ws = self._raw_ws
        if raw_ws is None:
            raw_ws = reducer.data_loader.uncropped

        mantid.sendLogMessage('::SANS::Normalizing to monitor ' + str(self._normalization_spectrum))
        # Get counting time or monitor
        norm_ws = workspace+"_normalization"
        norm_ws = 'Monitor'

        
        CropWorkspace(raw_ws, norm_ws,
                      StartWorkspaceIndex = normalization_spectrum, 
                      EndWorkspaceIndex   = normalization_spectrum)
    
        if reducer.instrument.name() == 'LOQ':
            RemoveBins(norm_ws, norm_ws, '19900', '20500',
                Interpolation="Linear")
        
        # Remove flat background
        if reducer.BACKMON_START != None and reducer.BACKMON_END != None:
            FlatBackground(norm_ws, norm_ws, StartX = reducer.BACKMON_START,
                EndX = reducer.BACKMON_END, WorkspaceIndexList = '0')
    
        #perform the same conversion on the monitor spectrum as was applied to the workspace but with a possibly different rebin
        sample_rebin = reducer.to_wavelen.rebin_alg 
        if reducer.instrument.is_interpolating_norm():
            reducer.to_wavelen.rebin_alg = 'InterpolatingRebin'
        else :
            reducer.to_wavelen.rebin_alg = 'Rebin'
        reducer.to_wavelen.execute(reducer, norm_ws)
        reducer.to_wavelen.rebin_alg = sample_rebin 

        out_workspace = workspace
        if reducer.to_Q.get_output_type() == '1D':
            # At this point need to fork off workspace name to keep a workspace containing raw counts
            reducer.to_Q.error_est_1D = 'to_delete_'+workspace+'_prenormed'
            RenameWorkspace(workspace, reducer.to_Q.error_est_1D)
            workspace = reducer.to_Q.error_est_1D

        Divide(workspace, norm_ws, out_workspace)

        mantid.deleteWorkspace(norm_ws)

# Setup the transmission workspace
##
class TransmissionCalc(sans_reduction_steps.BaseTransmission):
    # Map input values to Mantid options
    TRANS_FIT_OPTIONS = {
        'YLOG' : 'Log',
        'STRAIGHT' : 'Linear',
        'CLEAR' : 'Off',
        # Add Mantid ones as well
        'LOGARITHMIC' : 'Log',
        'LOG' : 'Log',
        'LINEAR' : 'Linear',
        'LIN' : 'Linear',
        'OFF' : 'Off'}

    #map to restrict the possible values of _trans_type
    CAN_SAMPLE_SUFFIXES = {
        False : 'sample',
        True : 'can'}
    
    DEFAULT_FIT = 'Log'

    def __init__(self, loader=None):
        super(TransmissionCalc, self).__init__()
        #set these variables to None, which means they haven't been set and defaults will be set further down
        self._lambda_min = None
        self._min_set = False
        self._lambda_max = None
        self._max_set = False
        self.fit_method = None
        self._method_set = False
        self._use_full_range = None
        self.loader = loader

    def get_lambdamin(self, instrum):
        """
            Gets lambdamin or the default if no value has been set
        """
        if self._lambda_min is None:
            return instrum.WAV_RANGE_MIN
        else:
            return self._lambda_min

    def get_lambdamax(self, instrum):
        """
            Gets lambdamax or the default if no value has been set
        """
        if self._lambda_max is None:
            return instrum.WAV_RANGE_MAX
        else:
            return self._lambda_max

    def set_trans_fit(self, min=None, max=None, fit_method=None, override=True):
        if min: min = float(min)
        if max: max = float(max)
        if not min is None:
            if (not self._min_set) or override:
                self._lambda_min = min
                self._min_set = override
        if not max is None:
            if (not self._max_set) or override:
                self._lambda_max = max
                self._max_set = override

        if not fit_method is None:
            if (not self._method_set) or override:
                fit_method = fit_method.upper()
                if fit_method in self.TRANS_FIT_OPTIONS.keys():
                    self.fit_method = self.TRANS_FIT_OPTIONS[fit_method]
                    override 
                else:
                    self.fit_method = self.DEFAULT_FIT
                    _issueWarning('ISISReductionStep.Transmission: Invalid fit mode passed to TransFit, using default method (%s)' % self.DEFAULT_FIT)
                self._method_set = override

    def set_full_wav(self, is_full):
        self._use_full_range = is_full

    def set_loader(self, loader):
        self.loader = loader

    def execute(self, reducer, workspace):
        if (self.loader is None) or (not self.loader.trans_name):
            return

        trans_raw = self.loader.trans_name
        direct_raw = self.loader.direct_name
        
        if not trans_raw:
            raise RuntimeError('Attempting transmission correction with no specified transmission %s file' % self.CAN_SAMPLE_SUFFIXES[self.loader.can])
        if not direct_raw:
            raise RuntimeError('Attempting transmission correction with no direct file')

        instrum = reducer.instrument

        if self.fit_method is None:
            self.fit_method = self.DEFAULT_FIT
            
        if self._use_full_range is None:
            use_full_range = reducer.full_trans_wav
        else:
            use_full_range = self._use_full_range
        if use_full_range:
            wavbin = str(instrum.WAV_RANGE_MIN) 
            wavbin +=','+str(reducer.to_wavelen.wav_step)
            wavbin +=','+str(instrum.WAV_RANGE_MAX)
            translambda_min = instrum.WAV_RANGE_MIN
            translambda_max = instrum.WAV_RANGE_MAX
        else:
            translambda_min = reducer.get_trans_lambdamin()
            translambda_max = reducer.get_trans_lambdamax()
            wavbin = str(reducer.to_wavelen.get_rebin())
    
        fittedtransws = trans_raw.split('_')[0] + '_trans_'
        fittedtransws += self.CAN_SAMPLE_SUFFIXES[self.loader.can]
        fittedtransws += '_'+str(translambda_min)+'_'+str(translambda_max)
        unfittedtransws = fittedtransws + "_unfitted"
        
        if (not use_full_range) or \
        (self.fit_method != 'Off' and mantid.workspaceExists(fittedtransws) == False) or \
        (self.fit_method == 'Off' and mantid.workspaceExists(unfittedtransws) == False):
            # If no fitting is required just use linear and get unfitted data from CalculateTransmission algorithm
            if self.fit_method == 'Off':
                fit_type = 'Linear'
            else:
                fit_type = self.fit_method

            if reducer.instrument.name() == 'LOQ':
                # Load in the instrument setup used for LOQ transmission runs
                instrum.load_transmission_inst(trans_raw)
                instrum.load_transmission_inst(direct_raw)
                
                trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw,
                    '1,2', reducer.BACKMON_START, reducer.BACKMON_END, wavbin, 
                    reducer.instrument.use_interpol_trans_calc, True)
                
                direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_raw,
                    '1,2', reducer.BACKMON_START, reducer.BACKMON_END, wavbin,
                    reducer.instrument.use_interpol_trans_calc, True)
                
                CalculateTransmission(trans_tmp_out,direct_tmp_out, fittedtransws, MinWavelength=translambda_min, MaxWavelength =  translambda_max, \
                                      FitMethod = fit_type, OutputUnfittedData=True)
            else:
                trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw,
                    '1,2', reducer.BACKMON_START, reducer.BACKMON_END, wavbin,
                    reducer.instrument.use_interpol_trans_calc, False)

                direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_raw,
                    '1,2', reducer.BACKMON_START, reducer.BACKMON_END, wavbin,
                    reducer.instrument.use_interpol_trans_calc, False)
                
                CalculateTransmission(trans_tmp_out,direct_tmp_out, fittedtransws,
                    reducer.instrument.incid_mon_4_trans_calc, reducer.instrument.trans_monitor,
                    MinWavelength = translambda_min, MaxWavelength = translambda_max,
                    FitMethod = fit_type, OutputUnfittedData=True)
            # Remove temporaries
            mantid.deleteWorkspace(trans_tmp_out)
            mantid.deleteWorkspace(direct_tmp_out)
            
        if self.fit_method == 'Off':
            result = unfittedtransws
            mantid.deleteWorkspace(fittedtransws)
        else:
            result = fittedtransws
    
        try:
            if use_full_range:
                tmp_ws = 'trans_' + self.CAN_SAMPLE_SUFFIXES[self.loader.can]
                tmp_ws += '_' + reducer.to_wavelen.get_range()
                Rebin(result, tmp_ws, Params = reducer.to_wavelen.get_rebin())
                trans_ws = tmp_ws
            else: 
                trans_ws = result
        except:
            raise RuntimeError("Failed to Rebin the workspaces %s and %s to be the same."%(workspace, trans_ws))
    
        try:
            Divide(workspace, trans_ws, workspace)
        except:
            raise RuntimeError("Failed to correct for transmission, are the bin boundaries for %s and %s the same?"%(workspace, trans_ws))

class ISISCorrections(sans_reduction_steps.CorrectToFileStep):
    DEFAULT_SCALING = 100.0
    def __init__(self, corr_type = '', operation = ''):
        super(ISISCorrections, self).__init__('', "Wavelength", "Divide")

        # Scaling values [%]
        self.rescale= self.DEFAULT_SCALING
    
    def set_filename(self, filename):
        raise AttributeError('The correction must be set in the instrument, or use the CorrectionToFileStep instead')

    def execute(self, reducer, workspace):
        #use the instrument's correction file
        self._filename = reducer.instrument.cur_detector().correction_file
        #do the correct to file
        super(ISISCorrections, self).execute(reducer, workspace)

        scalefactor = self.rescale
        # Data reduced with Mantid is a factor of ~pi higher than colette.
        # For LOQ only, divide by this until we understand why.
        if reducer.instrument.name() == 'LOQ':
            rescaleToColette = math.pi
            scalefactor /= rescaleToColette

        ws = mantid[workspace]
        ws *= scalefactor

class CorrectToFileISIS(sans_reduction_steps.CorrectToFileStep):
    """
        Adds the ability to change the name of the output workspace to
        it CorrectToFileStep, its base ReductionStep 
    """
    def __init__(self, file='', corr_type='', operation='', name_container=[]):
        super(CorrectToFileISIS, self).__init__(file, corr_type, operation)
        self.out_container = name_container

    def execute(self, reducer, workspace):
        if self._filename:
            if len(self.out_container) > 0:
                reducer.wksp_name = self.out_container[0]
                CorrectToFile(workspace, self._filename, reducer.wksp_name,
                              self._corr_type, self._operation)

class UserFile(ReductionStep):
    def __init__(self, file=None):
        """
            Reads a SANS mask file
        """
        super(UserFile, self).__init__()
        self.filename = file
        self._incid_monitor_lckd = False

    def execute(self, reducer, workspace=None):
        if self.filename is None:
            raise AttributeError('The user file must be set, use the function MaskFile')
        user_file = self.filename
        #Check that the file exists.
        if not os.path.isfile(user_file):
            user_file = os.path.join(reducer.user_file_path, self.filename)
            if not os.path.isfile(user_file):
                user_file = reducer._full_file_path(self.filename)
                if not os.path.isfile(user_file):
                    raise RuntimeError, "Cannot read mask. File path '%s' does not exist or is not in the user path." % self.filename
            
        # Re-initializes default values
        self._initialize_mask(reducer)
        reducer.flood_file.set_filename("")
    
        file_handle = open(user_file, 'r')
        for line in file_handle:
            self.read_line(line, reducer)

        # Close the handle
        file_handle.close()
        # Check if one of the efficency files hasn't been set and assume the other is to be used
        reducer.instrument.copy_correction_files()
        
        return True

    def read_line(self, line, reducer):
        # This is so that I can be sure all EOL characters have been removed
        line = line.lstrip().rstrip()
        upper_line = line.upper()
        if upper_line.startswith('L/'):
            self.readLimitValues(line, reducer)
        
        elif upper_line.startswith('MON/'):
            self._readMONValues(line, reducer)
        
        elif upper_line.startswith('MASK'):
            if len(upper_line[5:].strip().split()) == 4:
                _issueInfo('Box mask lines are not supported')
            else:
                reducer.mask.parse_instruction(upper_line)
        
        elif upper_line.startswith('SET CENTRE'):
            values = upper_line.split()
            reducer.set_beam_finder(sans_reduction_steps.BaseBeamFinder(float(values[2])/1000.0, float(values[3])/1000.0))
        
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
            else:
                # This checks whether the type is correct and issues warnings if it is not
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
        
        elif upper_line.startswith('BACK/MON/TIMES'):
            tokens = upper_line.split()
            if len(tokens) == 3:
                reducer.BACKMON_START = int(tokens[1])
                reducer.BACKMON_END = int(tokens[2])
            else:
                _issueWarning('Incorrectly formatted BACK/MON/TIMES line, not running FlatBackground.')
                reducer.BACKMON_START = None
                reducer.BACKMON_END = None
        
        elif upper_line.startswith("FIT/TRANS/"):
            params = upper_line[10:].split()
            nparams = len(params)
            if nparams == 3 or nparams == 1:
                if nparams == 1:
                    fit_type = params[0]
                    lambdamin = lambdamax = None
                else:
                    fit_type, lambdamin, lambdamax = params

                reducer.transmission_calculator.set_trans_fit(min=lambdamin, 
                    max=lambdamax, fit_method=fit_type, override=False)
            else:
                _issueWarning('Incorrectly formatted FIT/TRANS line, %s, line ignored' % upper_line)

        elif upper_line == 'SANS2D' or upper_line == 'LOQ':
            self._check_instrument(upper_line, reducer)  

        elif upper_line.startswith('PRINT '):
            _issueInfo(upper_line[6:])
        
        elif line.startswith('!') or not line:
            # this is a comment or empty line, these are allowed
            pass

        else:
            _issueWarning('Unrecognized line in user file the line %s, ignoring' % upper_line)
    
    def _initialize_mask(self, reducer):
        self._restore_defaults(reducer)

        reducer.CENT_FIND_RMIN = None
        reducer.CENT_FIND_RMAX = None
       
        reducer.Q_REBIN = None
        reducer.QXY = None
        reducer.DQY = None
         
        reducer.BACKMON_END = None
        reducer.BACKMON_START = None

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

        if not ',' in limit_line:
            # Split with no arguments defaults to any whitespace character and in particular
            # multiple spaces are include
            elements = limits.split()
            if len(elements) == 4:
                limit_type, minval, maxval, step = elements[0], elements[1], elements[2], elements[3]
                rebin_str = None
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
        else:
            limit_type = limits[0].lstrip().rstrip()
            rebin_str = limits[1:].lstrip().rstrip()
            minval = maxval = step_type = step_size = None
    
        if limit_type.upper() == 'WAV':
            reducer.to_wavelen.set_rebin(
                minval, step_type + step_size, maxval, override=False)
        elif limit_type.upper() == 'Q':
            if not rebin_str is None:
                reducer.Q_REBIN = rebin_str
            else:
                reducer.Q_REBIN = minval + "," + step_type + step_size + "," + maxval
        elif limit_type.upper() == 'QXY':
            reducer.QXY2 = float(maxval)
            reducer.DQXY = float(step_type + step_size)
        elif limit_type.upper() == 'R':
            reducer.mask.set_radi(minval, maxval)
            reducer.CENT_FIND_RMIN = float(minval)/1000.
            reducer.CENT_FIND_RMAX = float(maxval)/1000.
        elif limit_type.upper() == 'PHI':
            reducer.mask.set_phi_limit(
                float(minval), float(maxval), True, override=False) 
        elif limit_type.upper() == 'PHI/NOMIRROR':
            reducer.mask.set_phi_limit(
                float(minval), float(maxval), False, override=False)
        else:
            _issueWarning('Error in user file after L/, "%s" is not a valid limit line' % limit_type.upper())

    def _readMONValues(self, line, reducer):
        details = line[4:]
    
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
            #the settings here are overriden by MON/SPECTRUM
            if not self._incid_monitor_lckd:
                reducer.set_monitor_spectrum(
                    int(details.split()[1]), interpolate, override=False)
        
        elif details.upper().startswith('TRANS'):
            parts = details.split('=')
            if len(parts) < 2 or parts[0].upper() != 'TRANS/SPECTRUM' :
                _issueWarning('Unable to parse MON/TRANS line, needs MON/TRANS/SPECTRUM=')
            reducer.set_trans_spectrum(int(parts[1]), interpolate)        
    
        elif 'DIRECT' in details.upper() or details.upper().startswith('FLAT'):
            parts = details.split("=")
            if len(parts) == 2:
                filepath = parts[1].rstrip()
                #for VMS compatibility ignore anything in "[]", those are normally VMS drive specifications
                if '[' in filepath:
                    idx = filepath.rfind(']')
                    filepath = filepath[idx + 1:]
                if not os.path.isabs(filepath):
                    filepath = reducer.user_file_path+'/'+filepath
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
                        reducer.flood_file.set_filename(filepath)
                    else:
                        pass
                elif len(parts) == 2:
                    detname = parts[1]
                    if detname.upper() == 'REAR':
                        reducer.instrument.getDetector('REAR').correction_file \
                            = filepath
                    elif detname.upper() == 'FRONT' or detname.upper() == 'HAB':
                        reducer.instrument.getDetector('FRONT').correction_file \
                            = filepath
                    else:
                        _issueWarning('Incorrect detector specified for efficiency file "' + line + '"')
                else:
                    _issueWarning('Unable to parse monitor line "' + line + '"')
            else:
                _issueWarning('Unable to parse monitor line "' + line + '"')
        else:
            _issueWarning('Unable to parse monitor line "' + line + '"')

    def _readDetectorCorrections(self, details, reducer):
        values = details.split()
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
        else:
            raise NotImplemented('Detector correction on "'+det_axis+'" is not supported')

    def _check_instrument(self, inst_name, reducer):
        if reducer.instrument is None:
            raise RuntimeError('Use SANS2D() or LOQ() to set the instrument before Maskfile()')
        if not inst_name == reducer.instrument.name():
            raise RuntimeError('User settings file not compatible with the selected instrument '+reducer.instrument.name())

    def _restore_defaults(self, reducer):
        reducer.mask.parse_instruction('MASK/CLEAR')
        reducer.mask.parse_instruction('MASK/CLEAR/TIME')

        reducer.CENT_FIND_RMIN = reducer.CENT_FIND_RMAX
        reducer.Q_REBIN = reducer.QXY = reducer.DQY = None

        # Scaling values
        reducer._corr_and_scale.rescale = 100.  # percent
        
        reducer.BACKMON_START = reducer.BACKMON_END = None

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
        run = reducer._data_files.values()[0]
        name = run.split('_')[0]
        
        name += reducer.instrument.cur_detector().name('short')
        name += '_' + reducer.to_Q.output_type
        name += '_' + reducer.to_wavelen.get_range()
        self.name_holder[0] = name

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
