#
# SANSBatchMode.py
#
# M. Gigg - Based on Steve King's example

#
# For batch-reduction of LOQ or SANS2D runs in Mantid using a SINGLE set of instrument parameters (Q, wavelength, radius, etc)
#
# Reads multi-line input from file of the form:
# sample_sans   54431   sample_trans    54435   direct_beam 54433   can_sans    54432   can_trans   54434   direct_beam 54433   background_sans     background_trans        direct_beam     output_as   script54435.txt
#
# Assumes the following have already been set by either the SANS GUI or directly from python using the AssignSample, LimitsR etc commands
# instrument
# data directory
# data format
# user file directory
# user file name
# reduction type
# detector bank
# whether to use default wavelength range for transmissions

# Allows over-riding of following from user (mask) file:
# radial integration limits
# wavelength limits, bin size and binning
# q limits, q bin size and binning
# qxy limits, qxy bin size and binning

# The save directory must currently be specified in the Mantid.user.properties file

#Make the reduction module available
from ISISCommandInterface import *
from mantidsimple import *
import copy
import sys

################################################################################
# Avoid a bug with deepcopy in python 2.6, details and workaround here:
# http://bugs.python.org/issue1515
if sys.version_info[0] == 2 and sys.version_info[1] == 6:
    import types
    def _deepcopy_method(x, memo):
        return type(x)(x.im_func, copy.deepcopy(x.im_self, memo), x.im_class)
    copy._deepcopy_dispatch[types.MethodType] = _deepcopy_method
################################################################################

ALLOWED_NUM_ENTRIES = set([20,14,8,6,4])

# Build a dictionary of possible input data  keys
IN_FORMAT = {}
IN_FORMAT['sample_sans'] = ''
IN_FORMAT['sample_trans'] = ''
IN_FORMAT['sample_direct_beam'] = ''
IN_FORMAT['can_sans'] = ''
IN_FORMAT['can_trans'] = ''
IN_FORMAT['can_direct_beam'] = ''
# Backgrounds not yet implemented
#    IN_FORMAT['background_sans'] = 0
#    IN_FORMAT['background_trans'] = 0
#    IN_FORMAT['background_direct_beam'] = 0
IN_FORMAT['output_as'] = ''
#maps the run types above to the Python interface command to use to load it
COMMAND = {}
COMMAND['sample_sans'] = 'AssignSample(' 
COMMAND['can_sans'] = 'AssignCan('
COMMAND['sample_trans'] = 'TransmissionSample(' 
COMMAND['can_trans'] = 'TransmissionCan('

class SkipEntry(RuntimeError):
    pass
class SkipReduction(RuntimeError):
    pass

# Add a CSV line to the input data store
def addRunToStore(parts, run_store):
    # Check logical structure of line
    nparts = len(parts) 
    if nparts not in ALLOWED_NUM_ENTRIES:
        return 1

    inputdata = copy.deepcopy(IN_FORMAT)
    nparts -= 1
    #move through the file like sample_sans,99630,output_as,99630, ...
    for i in range(0, nparts, 2):
        role = parts[i]
        if role in inputdata.keys():
            inputdata[parts[i]] = parts[i+1]
        if 'background' in role:
            issueWarning('Background runs are not yet implemented in Mantid! Will process Sample & Can only')
        
    run_store.append(inputdata)
    return 0

def BatchReduce(filename, format, full_trans_wav=True, plotresults=False, saveAlgs={'SaveRKH':'txt'},verbose=False, centreit=False, reducer=None):
    """
        @param filename: the CSV file with the list of runs to analyse
        @param format: type of file to load, nxs for Nexus, etc.
        @param full_trans_wav: when set to true (default) a wider range of wavelengths are used in the transmission correction  calculation
        @param plotresults: if true and this function is run from Mantidplot a graph will be created for the results of each reduction
        @param save: this named algorithm will be passed the name of the results workspace and filename (default = 'SaveRKH'). Pass a tuple of strings to save to multiple file formats
        @param verbose: set to true to write more information to the log (default=False)
        @param centreit: do centre finding (default=False)
        @param reducer: if to use the command line (default) or GUI reducer object
    """     
    if not format.startswith('.'):
        format = '.' + format
        
    file_handle = open(filename, 'r')
    runinfo = []
    for line in file_handle:
        # See how many pieces of information have been provided; brackets delineate the field seperator (nothing for space-delimited, ',' for comma-seperated)
        parts = line.rstrip().split(',')
        if addRunToStore(parts, runinfo) > 0:
            issueWarning('Incorrect structure detected in input file "' + filename + '" at line \n"' + line + '"\nEntry skipped\n')
    # End of file reading
    file_handle.close()

    if reducer:
        ReductionSingleton().replace(reducer)

    #first copy the user settings incase running the reductionsteps can change it
    settings = copy.deepcopy(ReductionSingleton().reference())

    # Now loop over all the lines and do a reduction (hopefully) for each
    for run in runinfo:
        raw_workspaces = []
        try:
            # Load in the runs specified in the csv file
            raw_workspaces.append(read_run(run, 'sample_sans', format))
            
            #Transmission runs to be applied to the sample
            raw_workspaces += read_trans_runs(run, 'sample', format)
            
            # Can run 
            raw_workspaces.append(read_run(run, 'can_sans', format))
    
            #Transmission runs for the can
            raw_workspaces += read_trans_runs(run, 'can', format)
    
            if centreit == 1:
                if verbose == 1:
                    FindBeamCentre(50.,170.,12)
                    
            
            # WavRangeReduction runs the reduction for the specified wavelength range where the final argument can either be DefaultTrans or CalcTrans:
            reduced = WavRangeReduction(full_trans_wav=full_trans_wav)

        except SkipEntry, reason:
            #this means that a load step failed, the warning and the fact that the results aren't there is enough for the user
            issueWarning(str(reason)+ ', skipping entry')
            continue
        except SkipReduction, reason:
            #this means that a load step failed, the warning and the fact that the results aren't there is enough for the user
            issueWarning(str(reason)+ ', skipping reduction')
            continue
        except ValueError, reason:
            issueWarning('Cannot load file :'+str(reason))
#when we are all up to Python 2.5 replace the duplicated code below with one finally:
            delete_workspaces(raw_workspaces)
            raise
        
        delete_workspaces(raw_workspaces)
            

        file = run['output_as']
        #saving if optional and doesn't happen if the result workspace is left blank. Is this feature used?
        if file:
            for algor in saveAlgs.keys():
                #add the file extension, important when saving different types of file so they don't over-write each other
                ext = saveAlgs[algor]
                if not ext.startswith('.'):
                    ext = '.' + ext
                exec(algor+'(reduced,file+ext)')

        if verbose:
            mantid.sendLogMessage('::SANS::' + createColetteScript(run, format, reduced, centreit, plotresults, filename))
        # Rename the final workspace
        final_name = run['output_as']
        if final_name == '':
            final_name = reduced
        RenameWorkspace(reduced,final_name)
        if plotresults == 1:
            PlotResult(final_name)

        #the call to WaveRang... killed the reducer so copy back over the settings
        ReductionSingleton().replace(copy.deepcopy(settings))

def parse_run(run_num, ext):
    """
        Extracts an (optional) period specification from the run_num
        and then adds the extension
        @param run_num: run number with optional period specified a after the letter 'p'
        @param ext: file extension
        @return: run specification (number.extension), period
    """
    if not run_num:
        return '', -1
    parts = run_num.upper().split('P')
    if len(parts) > 2:
        raise RuntimeError('Problem reading run number "'+run_num+'"')
    run_spec = parts[0]+ext

    if len(parts) == 2:
        period = parts[1]
    else:
        period = -1

    return run_spec, period 

def read_run(runs, run_role, format):
    """
        Load a run specified in the CSV file
        @param runs: a line from a CSV file
        @param run_role: type of run, e.g. sample
        @param format: extension to add to the end of the run number specification
        @return: name of the workspace that was loaded
        @throw SkipReduction: if there is a problem with the line that means a reduction is impossible
        @throw SkipEntry: if the sample is entry is empty
    """
    run_file = runs[run_role]
    if len(run_file) == 0:
        if run_role == 'sample_sans':
            raise SkipEntry('Empty ' + run_role + ' run given')
        else:
            #only the sample is a required run
            return

    run_file, period = parse_run(run_file, format)
    run_ws = eval(COMMAND[run_role] + 'run_file, period=period)[0]')
    if not run_ws:
        raise SkipReduction('Cannot load ' + run_role + ' run "' + run_file + '"')
    return run_ws

def read_trans_runs(runs, sample_or_can, format):
    """
        Loads the transmission runs to either be applied to the sample or the can.
        There must be two runs
        @param runs: a line from a CSV file
        @param sample_or_can: a string with the name of the set of transmission runs to use, e.g. can 
        @param format: extension to add to the end of the run number specifications
        @return: names of the two workspaces that were loaded
        @throw SkipReduction: if there is a problem with the line that means a reduction is impossible
    """
    role1 = sample_or_can+'_trans'
    role2 = sample_or_can+'_direct_beam'

    run_file1, p1 = parse_run(runs[role1], format)
    run_file2, p2 = parse_run(runs[role2], format)
    if (not run_file1) and (not run_file2):
        #it is OK for transmission files not to be present
        return []

    ws1, ws2 = eval(COMMAND[role1] + 'run_file1, run_file2, period_t=p1, period_d=p2)')
    if len(run_file1) > 0 and len(ws1) == 0:
        raise SkipReduction('Cannot load ' + role1 + ' run "' + run_file1 + '"')
    if len(run_file2) > 0 and len(ws2) == 0: 
        raise SkipReduction('Cannot load ' + role2 + ' run "' + run_file2 + '"')
    return [ws1, ws2]

def delete_workspaces(workspaces):
    """
        Delete the list of workspaces if possible but fail siliently if there is
        a problem
        @param workspaces: the list to delete
    """
    if type(workspaces) != type(list()):
        if type(workspaces) != type(tuple()):
            workspaces = [workspaces]

    for wksp in workspaces:
        if wksp and mtd.workspaceExists(wksp):
            try:
                DeleteWorkspace(wksp)
            except:
                #we're only deleting to save memory, if the workspace really won't delete leave it
                pass
