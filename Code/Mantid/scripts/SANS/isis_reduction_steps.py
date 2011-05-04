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
        #the name of the loaded workspace in Mantid
        self.wksp_name = ''
        
    def _load(self, inst = None, is_can=False):
        """
            Load a workspace and read the logs into the passed instrument reference
            @param inst: a reference to the current instrument
            @param iscan: set this to True for can runs 
            @return: log values, number of periods in the workspace
        """
        workspace = self._get_workspace_name()

        if os.path.splitext(self._data_file)[1].lower().startswith('.r'):
            try:
                alg = LoadRaw(self._data_file, workspace, SpectrumMin=self._spec_min, SpectrumMax=self._spec_max)
            except ValueError:
                # MG - 2011-02-24: Temporary fix to load .sav or .s* files. Lets the file property
                # work it out
                file_hint = os.path.splitext(self._data_file)[0]
                alg = LoadRaw(file_hint, workspace, SpectrumMin=self._spec_min, SpectrumMax=self._spec_max)
                self._data_file = alg.getPropertyValue("Filename")
    
            LoadSampleDetailsFromRaw(workspace, self._data_file)
            #if the user didn't specify a period use the first period
            if self._period != self.UNSET_PERIOD:
                workspace = self._leaveSinglePeriod(workspace)

#the following code needs to be removed because log information is now stored in raw files
#            log_file = alg.getPropertyValue("Filename")
#
#            base_name = os.path.splitext(log_file)[0]
#            if base_name.endswith('-add'):
#                #remove the add files specifier, if it's there
#                base_name = base_name.???????????????????rpartition('-add')[0]
#            SANS2D_log_file = base_name+'.log'
            
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
                log = inst.get_detector_log(SANS2D_log_file)
            except:
                #transmission workspaces, don't have logs 
                if not self._is_trans:
                    raise

        self.wksp_name = workspace 
        return numPeriods, log        

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
            return '', -1
        
        try:
            data_file = self._extract_run_details(
                self._data_file, self._is_trans, prefix=reducer.instrument.name(), 
                run_number_width=reducer.instrument.run_number_width, period=self._period)
        except AttributeError:
            raise AttributeError('No instrument has been assign, run SANS2D or LOQ first')

        workspace = self._get_workspace_name()
        if not self._reload:
            raise NotImplementedError('Raw workspaces must be reloaded, run with reload=True')
            nPeriods = self._find_workspace_num_periods(workspace)
            if mantid.workspaceExists(workspace):
                self.wksp_name = workspace
                return '', nPeriods
            #this always contains the period number
            period_definitely_inc = self._get_workspace_name(False)
            if mantid.workspaceExists(period_definitely_inc):
                self.wksp_name = period_definitely_inc
                return '', nPeriods

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
                nPeriods, logs = self._load(reducer.instrument)
            except RuntimeError, err:
                mantid.sendLogMessage("::SANS::Warning: "+str(err))
                return '', -1
        else:
            try:
                nPeriods, logs = self._load(reducer.instrument)
            except RuntimeError, details:
                mantid.sendLogMessage("::SANS::Warning: "+str(details))
                self.wksp_name = ''
                return '', -1
        
        return nPeriods, logs

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

        #interpret an entire file name
        if run_string.find('/') > -1 or (prefix and run_string.find(prefix) == 0 ):
            #assume we have a complete filename
            filename = run_string
            #remove the path name
            names = run_string.split('/')
            run_name = names[len(names)-1]
            #remove the extension
            file_parts = run_name.split('.')
            run_name = file_parts[0]
            self.ext = file_parts[len(file_parts)-1]
            run_name = run_name.upper()
            if run_name.endswith('-ADD'):
                #remove the add files specifier, if it's there
                end = len(run_name)-len('-ADD')
                run_name = run_name[0:end]
            names = run_name.split(prefix)
            self.shortrun_no = names[len(names)-1]

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
            self.TRANS_SAMPLE_N_PERIODS, dummy = \
                                        load._assignHelper(reducer)
            if not load.wksp_name:
                # do nothing if no workspace was specified
                return '', ''
            self.trans_name = load.wksp_name

        if self._direct_name not in [None, '']:
            load = LoadRun(self._direct_name, trans=True, reload=self._reload, entry=self._period_d)
            self.DIRECT_SAMPLE_N_PERIODS, dummy = \
                                        load._assignHelper(reducer)
            if not load.wksp_name:
                raise RuntimeError('Transmission run set without direct run error')
            self.direct_name = load.wksp_name
 
        #transmission workspaces sometimes have monitor locations, depending on the instrument, load these locations
        reducer.instrument.load_transmission_inst(self.trans_name)
        reducer.instrument.load_transmission_inst(self.direct_name)

        return self.trans_name, self.direct_name

class CanSubtraction(LoadRun):
    """
        Apply the same corrections to the can that were applied to the sample and
        then subtracts this can from the sample.
    """
    def __init__(self, can_run, reload = True, period = -1):
        """
            @param can_run: the run number followed by dot and the extension 
            @param reload: if set to true (default) the workspace is replaced if it already exists
            @param period: for multiple entry workspaces this is the period number
        """
        super(CanSubtraction, self).__init__(can_run, reload=reload, entry=period)
        #contains the workspace with the background (can) data
        self.workspace = LoadRun()


    def assign_can(self, reducer):
        """
            Loads the can workspace into Mantid and reads any log file
            @param reducer: the reduction chain
            @return: the logs object  
        """
        if not reducer.user_settings.executed:
            raise RuntimeError('User settings must be loaded before the can can be loaded, run UserFile() first')
        
        if( self._data_file.startswith('.') or self._data_file == '' or self._data_file == None):
            self.workspace.wksp_name = ''
            return '()'
    
        self._CAN_N_PERIODS, logs = self._assignHelper(reducer)
        #refactor this so that CanSubtraction doesn't inherit from LoadRun
        self.workspace.wksp_name = self.wksp_name

        if self.workspace.wksp_name == '':
            mantid.sendLogMessage('::SANS::Warning: Unable to load SANS can run, cannot continue.')
            return '()'
          
        if logs:
            reducer.instrument.check_can_logs(logs)
        else:
            logs = ""
            if reducer.instrument.name() == 'SANS2D':
                _issueWarning("Can logs could not be loaded, using sample values.")
                return "()"    
        
        if self._reload:
            reducer.place_det_sam.reset(self.workspace.wksp_name)
        reducer.place_det_sam.execute(reducer, self.workspace.wksp_name)

        return logs

    def execute(self, reducer, workspace):
        """
            Apply same corrections as for sample workspace then subtract from data
        """
        if not self.workspace.wksp_name:
            # no can data was loaded skip the correction
            return        
        
        #remain the sample workspace, its name will be restored to the original once the subtraction has been done 
        tmp_smp = workspace+"_sam_tmp"
        RenameWorkspace(workspace, tmp_smp)

        tmp_can = workspace+"_can_tmp"

        #do same corrections as were done to the sample
        reducer.reduce_can(self.workspace.wksp_name, tmp_can)

        #we now have the can workspace, use it
        Minus(tmp_smp, tmp_can, workspace)
    
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

        self.mask_phi = True
        self.phi_mirror = True
        self._lim_phi_xml = ''
        self.phi_min = -90.0
        self.phi_max = 90.0
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
        
        #remove any trailing comma
        if speclist.endswith(','):
            speclist = speclist[0:len(speclist)-1]
        
        return speclist

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

    def execute(self, reducer, workspace, instrument=None):
        if not instrument:
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

        if ( not self.min_radius is None ) and ( self.min_radius > 0.0 ):
            self.add_cylinder(self.min_radius, 0, 0, 'beam_stop')
        if ( not self.max_radius is None ) and ( self.max_radius > 0.0 ):
            self.add_outside_cylinder(self.max_radius, 0, 0, 'beam_area')
        #now do the masking
        sans_reduction_steps.Mask.execute(self, reducer, workspace, instrument)

        if self._lim_phi_xml != '' and self.mask_phi:
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
        self.execute(None, wksp_name, instrum)
        
        #now the other detector
        other = instrum.other_detector().name()
        original = instrum.cur_detector().name()
        instrum.setDetector(other)
        self.execute(None, wksp_name, instrum)
        #reset the instrument to mask the currecnt detector
        instrum.setDetector(original)

        # Mark up "dead" detectors with error value 
        FindDeadDetectors(wksp_name, wksp_name, DeadValue=500)

        #opens an instrument showing the contents of the workspace (i.e. the instrument with masked detectors) 
        instrum.view(wksp_name)

    def display(self, wksp, instrum, counts=None):
        """
            Mask detectors in a workspace and display its show instrument
            @param wksp: this named workspace will be masked and displayed
            @param instrum: the instrument that the workspace is from
            @param counts: optional workspace containing neutron counts data that the mask will be supperimposed on to   
        """
        #apply masking to the current detector
        self.execute(None, wksp, instrum)
        
        #now the other detector
        other = instrum.other_detector().name()
        original = instrum.cur_detector().name()
        instrum.setDetector(other)
        self.execute(None, wksp, instrum)
        #reset the instrument to mask the current detector
        instrum.setDetector(original)

        # Mark up "dead" detectors with error value 
        FindDeadDetectors(wksp, wksp, LiveValue = 0, DeadValue=1)

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
                
                if (vals.readY(i)[0] > maxval):
                    #don't include masked or monitors
                    if (flags.readY(i)[0] == 0) and (vals.readY(i)[0] < 5000):
                        maxval = vals.readY(i)[0]
    
            #now normalise to the max/5
            maxval /= 5.0
            for i in range(0, len(Ys)):
                if Ys[i] != 0:
                    Ys[i] = maxval*Ys[i] + vals.readY(i)[0]

            CreateWorkspace(wksp, Xs, Ys, Es, len(Ys), UnitX='TOF', VerticalAxisValues=Ys)
            #change the units on the workspace so it is compatible with the workspace containing counts data
            Power(counts, 'ones', 0)
            Multiply('ones', wksp, 'units')
            #do the super-position and clean up
            Minus(counts, 'units', wksp)
            mantid.deleteWorkspace('ones')
            mantid.deleteWorkspace('units')

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
        super(LoadSample, self).__init__(sample, reload=reload, entry=entry)
        self._scatter_sample = None
        self._SAMPLE_RUN = None
        self._SAMPLE_N_PERIODS = -1
        
        self.maskpt_rmin = None
        
        #This is set to the name of the workspace that was loaded, with some changes made to it 
        self.uncropped = None
    
    def execute(self, reducer, workspace):
        if not reducer.user_settings.executed:
            raise RuntimeError('User settings must be loaded before the sample can be assigned, run UserFile() first')

        # Code from AssignSample
        self._clearPrevious(self._scatter_sample)
        self._SAMPLE_N_PERIODS = -1
        
        if self._data_file.startswith('.') or ( not self._data_file ):
            self._SAMPLE_RUN = ''
            self._scatter_sample = None
            raise RuntimeError('Sample needs to be assigned as run_number.file_type')

        self._SAMPLE_N_PERIODS, logs =\
            self._assignHelper(reducer)

        if self.wksp_name == '':
            raise RuntimeError('Unable to load SANS sample run, cannot continue.')

        self.uncropped  = self.wksp_name
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
                mantid.deleteWorkspace(self.wksp_name)
                raise RuntimeError('Sample logs cannot be loaded, cannot continue')
            reducer.instrument.apply_detector_logs(logs)           

        if self._reload:
            reducer.place_det_sam.reset(self.wksp_name)
        reducer.place_det_sam.execute(reducer, self.wksp_name)

        reducer.sample_wksp = self.wksp_name
        return logs

class MoveComponents(ReductionStep):
    """
        Moves the components so that the centre of the detector bank is at the location
        that was passed to the BeamFinder
    """
    def __init__(self):
        super(MoveComponents, self).__init__()
        #dictionary contains where given workspaces were moved to to avoid moving them again
        self._moved_to = {}

    def execute(self, reducer, workspace):

        # Put the components in the correct positions
        beamcoords = reducer._beam_finder.get_beam_center()
        if self._moved_to.has_key(workspace):
            if self._moved_to[workspace] != beamcoords:
                #the components had already been moved, return them to their original location first
                reducer.instrument.set_component_positions(workspace,
                    -self._moved_to[workspace][0], -self._moved_to[workspace][1])
        
        if ( not self._moved_to.has_key(workspace) ) or \
                (self._moved_to[workspace] != beamcoords):
            self.maskpt_rmin, self.maskpt_rmax = reducer.instrument.set_component_positions(workspace, beamcoords[0], beamcoords[1])
            self._moved_to[workspace] = beamcoords

        mantid.sendLogMessage('::SANS:: Moved sample workspace to [' + str(self.maskpt_rmin)+','+str(self.maskpt_rmax) + ']' )

    def reset(self, workspace):
        self._moved_to.pop(workspace, None)

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
            reducer.output_wksp = self.out_container[0]
        # Get the detector bank that is to be used in this analysis leave the complete workspace
        CropWorkspace(workspace, reducer.output_wksp,
            StartWorkspaceIndex = reducer.instrument.cur_detector().get_first_spec_num() - 1,
            EndWorkspaceIndex = reducer.instrument.cur_detector().last_spec_num - 1)

class ConvertToQ(ReductionStep):
    _OUTPUT_TYPES = {'1D' : 'Q1D',
                     '2D': 'Qxy'}
    _DEFAULT_GRAV = False
    
    def __init__(self, container=None):
        super(ConvertToQ, self).__init__()
	#allows running the reducers keep_un_normalised(), if required
	self._error_holder = container
        
        #this should be set to 1D or 2D
        self._output_type = '1D'
        #the algorithm that corrosponds to the above choice
        self._Q_alg = self._OUTPUT_TYPES[self._output_type]
        #if true gravity is taken into account in the Q1D calculation
        self._use_gravity = self._DEFAULT_GRAV
        self._grav_set = False
    
    def set_output_type(self, descript):
        """
	    Requests the given output from the Q conversion, either 1D or 2D. For
	    the 1D calculation it asks the reducer to keep a workspace for error
	    estimates
	    @param descript: 1D or 2D
	"""
        self._Q_alg = self._OUTPUT_TYPES[descript]
        self._output_type = descript
	
        if self._output_type == '1D':
            if not self._error_holder['Q1D errors']:
                raise RuntimeError('Could not find the un-normalised sample workspace needed for error estimates')
	        	
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
	    #use a counts workspace for errors only if it exists, otherwise use the data workspace as a dummy
            if self._error_holder['Q1D errors']:
                errors = self._error_holder['Q1D errors']
	    else:
	        errors = workspace

            Q1D(workspace, errors, workspace, reducer.Q_REBIN, AccountForGravity=self._use_gravity)

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
    NORMALISATION_SPEC_NUMBER = 1
    NORMALISATION_SPEC_INDEX = 0
    def __init__(self, spectrum_number=None, raw_ws=None):
        if not spectrum_number is None:
            index_num = spectrum_number
        else:
            index_num = None
        super(NormalizeToMonitor, self).__init__(index_num)
        self._raw_ws = raw_ws
	#it is possible to keep the un-normalised workspace, to do this set this to the name you want this workspace to have
	self.save_original = None

    def execute(self, reducer, workspace):
        normalization_spectrum = self._normalization_spectrum 
        if normalization_spectrum is None:
            #the -1 converts from spectrum number to spectrum index
            normalization_spectrum = reducer.instrument.get_incident_mon()
        
        raw_ws = self._raw_ws
        if raw_ws is None:
            raw_ws = reducer.data_loader.uncropped

        mantid.sendLogMessage('::SANS::Normalizing to monitor ' + str(normalization_spectrum))
        # Get counting time or monitor
        norm_ws = workspace+"_normalization"
        norm_ws = 'Monitor'

        
        CropWorkspace(raw_ws, norm_ws,
                      StartWorkspaceIndex = normalization_spectrum-1, 
                      EndWorkspaceIndex   = normalization_spectrum-1)
    
        if reducer.instrument.name() == 'LOQ':
            RemoveBins(norm_ws, norm_ws, '19900', '20500',
                Interpolation="Linear")
        
        # Remove flat background
        TOF_start, TOF_end = reducer.inst.get_TOFs(
                                    self.NORMALISATION_SPEC_NUMBER)
        if TOF_start and TOF_end:
            FlatBackground(norm_ws, norm_ws, StartX=TOF_start, EndX=TOF_end,
                WorkspaceIndexList=self.NORMALISATION_SPEC_INDEX, Mode='Mean')

        #perform the same conversion on the monitor spectrum as was applied to the workspace but with a possibly different rebin
        if reducer.instrument.is_interpolating_norm():
            r_alg = 'InterpolatingRebin'
        else :
            r_alg = 'Rebin'
        reducer.to_wavelen.execute(reducer, norm_ws, bin_alg=r_alg)

        out_workspace = workspace
        if self.save_original:
            # At this point need to fork off workspace name to keep a workspace containing raw counts
            RenameWorkspace(workspace, self.save_original)
            workspace = self.save_original

        Divide(workspace, norm_ws, out_workspace)

        DeleteWorkspace(norm_ws)

class TransmissionCalc(sans_reduction_steps.BaseTransmission):
    """
        Calculatates the proportion of neutrons that are transmitted through the sample
        as a function of wavelenth. The resulsts are stored aas a workspace
    """
    
    # The different ways of doing a fit, convert the possible ways of specifying this into the stand way it's shown on the GUI 
    TRANS_FIT_OPTIONS = {
        'YLOG' : 'Logarithmic',
        'STRAIGHT' : 'Linear',
        'CLEAR' : 'Off',
        # Add Mantid ones as well
        'LOGARITHMIC' : 'Logarithmic',
        'LOG' : 'Logarithmic',
        'LINEAR' : 'Linear',
        'LIN' : 'Linear',
        'OFF' : 'Off'}
    
    
    # Relate the different GUI names for doing a fit to the arguments that can be sent to CalculateTransmission 
    CALC_TRANS_FIT_PARAMS = {
        'Logarithmic' : 'Log',
        'Linear' : 'Linear',
        'Off' : 'Linear'
    }

    #map to restrict the possible values of _trans_type
    CAN_SAMPLE_SUFFIXES = {
        False : 'sample',
        True : 'can'}
    
    DEFAULT_FIT = 'Logarithmic'

    def __init__(self, loader=None):
        super(TransmissionCalc, self).__init__()
        #set these variables to None, which means they haven't been set and defaults will be set further down
        self.lambda_min = None
        self._min_set = False
        self.lambda_max = None
        self._max_set = False
        self.fit_method = None
        self._method_set = False
        self._use_full_range = None
        # A LoadTransmissions object that contains the names of the transmission and direct workspaces
        self.loader = loader
        # this contains the spectrum number of the monitor that comes after the sample from which the transmission calculation is done 
        self._trans_spec = None
        # use InterpolatingRebin 
        self.interpolate = None
        # a custom transmission workspace, if we have this there is much less to do 
        self.calculated_samp = ''
        self.calculated_can = None

    def set_trans_fit(self, min=None, max=None, fit_method=None, override=True):
        if min: min = float(min)
        if max: max = float(max)
        if not min is None:
            if (not self._min_set) or override:
                self.lambda_min = min
                self._min_set = override
        if not max is None:
            if (not self._max_set) or override:
                self.lambda_max = max
                self._max_set = override

        if fit_method:
            if (not self._method_set) or override:
                fit_method = fit_method.upper()
                if fit_method in self.TRANS_FIT_OPTIONS.keys():
                    self.fit_method = self.TRANS_FIT_OPTIONS[fit_method]
                else:
                    self.fit_method = self.DEFAULT_FIT
                    _issueWarning('ISISReductionStep.Transmission: Invalid fit mode passed to TransFit, using default method (%s)' % self.DEFAULT_FIT)
                self._method_set = override

    def set_loader(self, loader):
        self.loader = loader
        
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
        
        #exclude unused spectra because the empty spectra can cause errors
        spectrum1 = min(pre_monitor, post_monitor)
        #index numbers are one less than spectrum number for raw data files
        index1 = spectrum1-1
        index2 = max(pre_monitor, post_monitor)-1
        CropWorkspace(inputWS, tmpWS,
            StartWorkspaceIndex=index1, EndWorkspaceIndex=index2)
    

        if inst.name() == 'LOQ':
            RemoveBins(tmpWS, tmpWS, 19900, 20500, Interpolation='Linear')

        for spectra_number in [pre_monitor, post_monitor]:
            back_start, back_end = inst.get_TOFs(spectra_number)
            if back_start and back_end:
                index = spectra_number - spectrum1
                FlatBackground(tmpWS, tmpWS, StartX=back_start, EndX=back_end,
                               WorkspaceIndexList=index, Mode='Mean')

        ConvertUnits(tmpWS, tmpWS,"Wavelength")
        
        if self.interpolate:
            InterpolatingRebin(tmpWS, tmpWS, wavbining)
        else :
            Rebin(tmpWS, tmpWS, wavbining)
    
        return tmpWS

    def execute(self, reducer, workspace):
        """
            Reads in the different settings, without affecting self. Calculates
            or estimates the proportion of neutrons that are transmitted
            through the sample
        """
        #look for run files that contain transmission data
        test1, test2 = self._get_run_wksps()
        if test1 or test2:
            if self.calculated_samp:
                raise RuntimeError('Cannot use TransWorkspace() and TransmissionSample() together')
            
            trans_ws = self.calculate(reducer)
        else:
            if self.calculated_samp:
                temp_ws = self.calculated_samp+'_rebinned'
                RebinToWorkspace(self.calculated_samp, workspace, temp_ws)
                trans_ws = temp_ws
            else:
                #if no transmission files were specified this isn't an error, we just do nothing
                return None

        try:
            Divide(workspace, trans_ws, workspace)
        except:
#when we are all up to Python 2.5 replace the duplicated code below with one finally:
            if self.calculated_samp:
                if mtd.workspaceExists(temp_ws):
                    DeleteWorkspace(temp_ws)

            raise RuntimeError("Failed to correct for transmission, are the bin boundaries for %s and %s the same?"%(workspace, trans_ws))


        if self.calculated_samp:
            if mtd.workspaceExists(temp_ws):
                DeleteWorkspace(temp_ws)

    def _get_run_wksps(self):
        """
            Retrieves the names runs that contain the user specified for calculation
            of the transmission
            @return: post_sample pre_sample workspace names
        """  
        if (not self.loader) or (not self.loader.trans_name):
            return '', ''
        else:
            return self.loader.trans_name, self.loader.direct_name

    def calculate(self, reducer):       
        #get the settings required to do the calculation
        trans_raw, direct_raw = self._get_run_wksps()
        
        if not trans_raw:
            raise RuntimeError('Attempting transmission correction with no specified transmission %s file' % self.CAN_SAMPLE_SUFFIXES[self.loader.can])
        if not direct_raw:
            raise RuntimeError('Attempting transmission correction with no direct file')

        if self.fit_method:
            fit_meth = self.fit_method
        else:
            fit_meth = self.DEFAULT_FIT

        if self._trans_spec:
            post_sample = self._trans_spec
        else:
            post_sample = reducer.instrument.default_trans_spec

        pre_sample = reducer.instrument.incid_mon_4_trans_calc

        if self._use_full_range is None:
            use_full_range = reducer.full_trans_wav
        else:
            use_full_range = self._use_full_range
        if use_full_range:
            translambda_min = reducer.instrument.WAV_RANGE_MIN
            translambda_max = reducer.instrument.WAV_RANGE_MAX
        else:
            if self.lambda_min:
                translambda_min = self.lambda_min
            else:
                translambda_min = reducer.to_wavelen.wav_low
            if self.lambda_max:
                translambda_max = self.lambda_max
            else:
                translambda_max = reducer.to_wavelen.wav_high
        wavbin = str(translambda_min) 
        wavbin +=','+str(reducer.to_wavelen.wav_step)
        wavbin +=','+str(translambda_max)

        #set up the input workspaces
        trans_tmp_out = self.setup_wksp(trans_raw, reducer.instrument,
            wavbin, pre_sample, post_sample)
        direct_tmp_out = self.setup_wksp(direct_raw, reducer.instrument,
            wavbin, pre_sample, post_sample)

        fittedtransws, unfittedtransws = self.get_wksp_names(
                            trans_raw, translambda_min, translambda_max)
        
        # If no fitting is required just use linear and get unfitted data from CalculateTransmission algorithm
        fit_type = self.CALC_TRANS_FIT_PARAMS[fit_meth]
        CalculateTransmission(trans_tmp_out,direct_tmp_out, fittedtransws,
            pre_sample, post_sample, MinWavelength=translambda_min,
            MaxWavelength=translambda_max, FitMethod=fit_type, OutputUnfittedData=True)

        # Remove temporaries
        DeleteWorkspace(trans_tmp_out)
        if direct_tmp_out != trans_tmp_out:
            DeleteWorkspace(direct_tmp_out)
            
        if fit_meth == 'Off':
            result = unfittedtransws
            mantid.deleteWorkspace(fittedtransws)
        else:
            result = fittedtransws
    
        try:
            Rebin(result, result, Params = reducer.to_wavelen.get_rebin())
        except:
            raise RuntimeError("Failed to Rebin the the transmission workspace %s to match the sample" %result)

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

    def get_wksp_names(self, raw_name, lambda_min, lambda_max):
        fitted_name = raw_name.split('_')[0] + '_trans_'
        fitted_name += self.CAN_SAMPLE_SUFFIXES[self.loader.can]
        fitted_name += '_'+str(lambda_min)+'_'+str(lambda_max)
        
        unfitted = fitted_name + "_unfitted"
        
        return fitted_name, unfitted


class ISISCorrections(ReductionStep):
    DEFAULT_SCALING = 100.0
    def __init__(self):
        # Scaling values [%]
        self.rescale= self.DEFAULT_SCALING
    
    def set_filename(self, filename):
        raise AttributeError('The correction must be set in the instrument, or use the CorrectionToFileStep instead')

    def execute(self, reducer, workspace):
        #use the instrument's correction file
        corr_file = reducer.instrument.cur_detector().correction_file
        corr = sans_reduction_steps.CorrectToFileStep(corr_file, "Wavelength", "Divide")
        corr.execute(reducer, workspace)

        scalefactor = self.rescale
        # Data reduced with Mantid is a factor of ~pi higher than Colette.
        # For LOQ only, divide by this until we understand why.
        if reducer.instrument.name() == 'LOQ':
            rescaleToColette = math.pi
            scalefactor /= rescaleToColette

        ws = mantid[workspace]
        ws *= scalefactor

class CorrectToFileISIS(sans_reduction_steps.CorrectToFileStep):
    """
        Adds the ability to change the name of the output workspace to
        its CorrectToFileStep (the base ReductionStep) 
    """
    def __init__(self, file='', corr_type='', operation='', name_container=[]):
        super(CorrectToFileISIS, self).__init__(file, corr_type, operation)
        self.out_container = name_container

    def execute(self, reducer, workspace):
        if self._filename:
            if len(self.out_container) > 0:
                reducer.output_wksp = self.out_container[0]
                CorrectToFile(workspace, self._filename, reducer.output_wksp,
                              self._corr_type, self._operation)

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
        ConvertUnits(workspace, workspace, self._units)
        
        low_wav = self.wav_low
        high_wav = self.wav_high
        
        if low_wav is None and high_wav is None:
            low_wav = min(mtd[workspace].readX(0))
            high_wav = max(mtd[workspace].readX(0))

         
        if not bin_alg:
            bin_alg = self.rebin_alg
 
        rebin_com = bin_alg+'(workspace, workspace, "'+\
            self._get_rebin(low_wav, self.wav_step, high_wav)+'")'
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
            'TRANS/': self._read_trans_line}

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
                params = upper_line[len(keyword):]
                #call the handling function for that keyword
                error = self.key_functions[keyword](params, reducer)
                
                if error:
                    _issueWarning(error+line)
                
                return

        if upper_line.startswith('L/'):
            self.readLimitValues(line, reducer)
        
        elif upper_line.startswith('MON/'):
            self._readMONValues(line, reducer)
        
        elif upper_line.startswith('MASK'):
            if len(upper_line[5:].strip().split()) == 4:
                _issueInfo('Box masks can only be defined using the V and H syntax, not "mask x1 y1 x2 y2"')
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
        
        elif upper_line.startswith('FIT/TRANS/'):
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
                reducer.to_wavelen.set_rebin(
                        minval, step_type + step_size, maxval, override=False)
        elif limit_type.upper() == 'Q':
            if rebin_str:
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
        elif (limit_type.upper() == 'PHI') or (limit_type.upper() == 'PHI/NOMIRROR'):
            mirror = limit_type.upper() != 'PHI/NOMIRROR'
            if maxval.endswith('/NOMIRROR'):
                maxval = maxval.split('/NOMIRROR')[0]
                mirror = False
            reducer.mask.set_phi_limit(
                float(minval), float(maxval), mirror, override=False)
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
            window for the background region for a specific monitor
            @param arguments: the contents of the line after the first keyword
            @param reducer: the object that contains all the settings
            @return any errors encountered or ''
        """
        try:
            # assume a line of the form BACK/M1/TIME 
            parts = arguments.split('/TIME')
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
        self._process(keys, funcs, arguments, reducer)

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
        arguments = arguments.split('=')
        if len(arguments) < 2:
            raise RuntimeError('An "=" is required after TRANSPEC')
        reducer.transmission_calculator.trans_spec = arguments[1]
        return ''
        
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
        reducer.mask.parse_instruction('MASK/CLEAR')
        reducer.mask.parse_instruction('MASK/CLEAR/TIME')

        reducer.CENT_FIND_RMIN = reducer.CENT_FIND_RMAX
        reducer.Q_REBIN = reducer.QXY = reducer.DQY = None

        # Scaling values
        reducer._corr_and_scale.rescale = 100.  # percent
        
        reducer.inst.reset_TOFs()

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
        name = reducer.sample_wksp.split('_')[0]
        
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
