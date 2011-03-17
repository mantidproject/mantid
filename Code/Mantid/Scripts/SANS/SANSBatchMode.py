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

# Add a CSV line to the input data store
def addRunToStore(parts, run_store):
    # Check logical structure of line
    allowednparts = set([20,14,8,6,4])
    nparts = len(parts) 
    if nparts not in allowednparts:
        return 1

    # Build a dictionary of input data  keys
    inputdata = {}
    inputdata['sample_sans'] = ''
    inputdata['sample_trans'] = ''
    inputdata['sample_direct_beam'] = ''
    inputdata['can_sans'] = ''
    inputdata['can_trans'] = ''
    inputdata['can_direct_beam'] = ''
    # Backgrounds not yet implemented
#    inputdata['background_sans'] = 0
#    inputdata['background_trans'] = 0
#    inputdata['background_direct_beam'] = 0
    inputdata['output_as'] = ''
    nparts -= 1
    for i in range(0,nparts):
        type = parts[i]
        if type in inputdata.keys():
            inputdata[parts[i]] = parts[i+1]
        if 'background' in type:
            issueWarning('Background runs are not yet implemented in SANSReduction.py! Will process Sample & Can only')
        
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

    # Now loop over run information and process
    for run in runinfo:
        try:
            # Sample run
            run_file = run['sample_sans']
            if len(run_file) == 0:
                issueWarning('No sample run given, skipping entry.')
                continue
            run_file += format
            sample_ws = AssignSample(run_file)[0]
            if len(sample_ws) == 0:
                issueWarning('Cannot load sample run "' + run_file + '", skipping reduction')
                continue
            
            #Sample trans
            run_file = run['sample_trans']
            run_file2 = run['sample_direct_beam']
            ws1, ws2 = TransmissionSample(run_file + format, run_file2 + format)
            if len(run_file) > 0 and len(ws1) == 0:
                issueWarning('Cannot load trans sample run "' + run_file + '", skipping reduction')
                continue
            if len(run_file2) > 0 and len(ws2) == 0: 
                issueWarning('Cannot load trans direct run "' + run_file2 + '", skipping reduction')
                continue
            
            # Sans Can 
            run_file = run['can_sans']
            can_ws = AssignCan(run_file + format)[0]
            if run_file != '' and len(can_ws) == 0:
                issueWarning('Cannot load can run "' + run_file + '", skipping reduction')
                continue
    
            #Can trans
            run_file = run['can_trans']
            run_file2 = run['can_direct_beam']
            ws1, ws2 = TransmissionCan(run_file + format, run_file2 + format)
            if len(run_file) > 0 and len(ws1) == 0:
                issueWarning('Cannot load trans can run "' + run_file + '", skipping reduction')
                continue
            if len(run_file2) > 0 and len(ws2) == 0: 
                issueWarning('Cannot load trans can direct run "' + run_file2 + '", skipping reduction')
                continue
    
    
            if centreit == 1:
                if verbose == 1:
                    FindBeamCentre(50.,170.,12)

        finally:
            # WavRangeReduction runs the reduction for the specfied wavelength range where the final argument can either be DefaultTrans or CalcTrans:
            # Parameter DefaultTrans specifies the transmission should be calculated for the whole range specified by L/WAV and then cropped it to the current wavelength range
            # Parameter CalcTrans specifies the transmission should be calculated for the specifed wavelength range only
            reduced = WavRangeReduction(full_trans_wav=full_trans_wav)

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


#ReductionSingleton.clean(isis_reducer.ISISReducer)
