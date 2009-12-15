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
from SANSReduction import _printMessage,AssignSample, AssignCan,TransmissionSample,TransmissionCan,FindBeamCentre,plotSpectrum,WavRangeReduction,NewTrans,DefaultTrans
from mantidsimple import *

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
            _printMessage('WARNING! Background runs are not yet implemented in SANSReduction.py! Will process Sample & Can only')
        
    run_store.append(inputdata)
    return 0

def BatchReduce(filename, format, deftrans = DefaultTrans, plotresults = False, verbose = False, centreit = False):
    if not format.startswith('.'):
        format = '.' + format
        
    file_handle = open(filename, 'r')
    if verbose == 1:
        _printMessage('[COLETTE]  @',filename)
    runinfo = []
    for line in file_handle:
        # See how many pieces of information have been provided; brackets delineate the field seperator (nothing for space-delimited, ',' for comma-seperated)
        parts = line.rstrip().split(',')
        if verbose == 1:
            _printMessage('Number of parts to the input line is:' + str(len(parts)))
        
        if addRunToStore(parts, runinfo) > 0:
            _printMessage('WARNING! Incorrect structure detected in input file "' + filename + '" at line \n"' + line + '"\nEntry skipped\n')
    # End of file reading
    file_handle.close()

    # Now loop over run information and process
    for run in runinfo:
        # Sample run
        run_file = run['sample_sans']
        if len(run_file) == 0:
            _printMessage('Warning! No sample run given, skipping entry.')
            continue
        run_file += format
        AssignSample(run_file)
        
        #Sample trans
        run_file = run['sample_trans']
        run_file2 = run['sample_direct_beam']
        TransmissionSample(run_file + format, run_file2 + format)
        
        # Sans Can 
        AssignCan(run['can_sans'] + format)

        #Can trans
        run_file = run['can_trans']
        run_file2 = run['can_direct_beam']
        TransmissionCan(run_file + format, run_file2+ format)

        if centreit == 1:
            if verbose == 1:
                _printMessage('[COLETTE]  FIT/MIDDLE')
                FindBeamCentre(50.,170.,12)
        # WavRangeReduction runs the reduction for the specfied wavelength range where the final argument can either be DefaultTrans or CalcTrans:
        # Parameter DefaultTrans specifies the transmission should be calculated for the whole range specified by L/WAV and then cropped it to the current wavelength range
        # Parameter CalcTrans specifies the transmission should be calculated for the specifed wavelength range only
        if verbose == 1:
            _printMessage('[COLETTE]  CORRECT/SC')
        reduced = WavRangeReduction(use_def_trans=deftrans)
        if plotresults == 1:
            if verbose == 1:
                _printMessage('[COLETTE]  DISPLAY/HISTOGRAM ' + reduced)
            plotSpectrum(reduced,0)
        file_1 = run['output_as'] + '.txt'
        if file_1 != '.txt':
            SaveRKH(reduced,file_1,'0')
        if verbose == 1:
            _printMessage('[COLETTE]  WRITE/LOQ ' + reduced + ' ' + file_1)
        # Rename the final workspace
        RenameWorkspace(reduced,run['output_as'])
