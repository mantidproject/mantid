from mantidsimple import *
import os
import string

def find_file(run_number):
    """Use Mantid to search for the given run.
    """    
    file_hint = str(run_number)
    found = FileFinder.findRuns(file_hint)
    if len(found) > 0:
        return found[0]
    else:
        message = 'Cannot find file matching hint "%s" on current search paths ' + \
                  'for instrument "%s"'
        raise ValueError( message % (file_hint, mtd.settings['default.instrument']))

def create_resultname(run_number, prefix='', suffix=''):
    """Create a string based on the run number and optional prefix and 
    suffix.    
    """
    if type(run_number) == list:
        name = create_resultname(run_number[0], prefix, suffix)
    elif type(run_number) == int:
        name = prefix + str(run_number) + '.spe' + suffix
    else:
        name = os.path.basename(run_number)
        # Hack any instrument name off the front so the output is the same as if you give it a run number
        name = name.lstrip(string.ascii_letters)
        if (suffix is None):
            name = os.path.splitext(name)[0] + '.spe'
        else:
            name = os.path.splitext(name)[0] + '.spe' + suffix
    return name
    
def create_dataname(input):
    """This assumes some kind of filename input and creates a workspace
    from the basename of the full file path
    """
    return os.path.basename(input)

# Keeps track of loaded data files so that they can be clean up easily
_loaded_data = []

#--- Temporary ----
# This is temporary so that the correct file can be picked up for LoadDetectorInfo & LoadEventNexusMonitors
# When the scripts do the splitting of montors correctlyt this can go
_last_mono_file = None

def last_mono_file():
    return _last_mono_file
#----------------------

def clear_loaded_data():
    """Clears any previously loaded data workspaces
    """
    global _last_mono_file, _loaded_data
    _last_mono_file = None
    for data_ws in _loaded_data:
        mtd.deleteWorkspace(data_ws)
    _loaded_data = []
   
def is_loaded(filename):
    """Returns True if the file is already loaded, false otherwise
    """
    global _loaded_files
    data_name =  create_dataname(filename)
    if data_name in _loaded_files:
        return True
    else:
        return False
    
def mark_as_loaded(filename, is_mono_file=False):
    """Mark a file as loaded.
    """
    global _last_mono_file, _loaded_data
    data_name =  create_dataname(filename)
    if data_name not in _loaded_data:
        mtd.sendLogMessage("Marking %s as loaded." % filename)
        _loaded_data.append(data_name)
    if is_mono_file:
        _last_mono_file = filename
        mtd.sendLogMessage("Marking %s as last mono file used." % filename)

def load_runs(runs, file_type, sum=True):
    """
    Loads a list of files, summing if the required.
    
    file_type is used to see if we need to keep track of a loaded mono run
    This is a hack for the moment while the LoadDetectorInfo algorithm and 
    splitting monitors is sorted out.
    """
    if type(runs) == list:
        if len(runs) == 1:
            sum = False
        if sum == True:
            result_ws = load_run(runs[0], file_type)
            summed = 'summed-run-files'
            if len(file_type) > 0: 
                summed += '-' + file_type
            CloneWorkspace(result_ws, summed)
            if len(runs) > 1:
                sum_files(summed, runs[1:], file_type)
                result_ws = mtd[summed]
            mark_as_loaded(summed, False)
            return result_ws
        else:
            loaded = []
            for r in runs:
                loaded.append(load_run(r, file_type))
            if len(loaded) == 1:
                return loaded[0]
            else:
                return loaded
    else:
        # Try a single run
        return load_run(runs, file_type)

def load_run(run_number, file_type='mono-sample',force=False):
    """Loads run into the given workspace. 
    
    The file_type is used to track whether this is a mono run.
    This is a hack for the moment while the LoadDetectorInfo algorithm and 
    splitting monitors is sorted out.
    If force is true then the file is loaded regardless of whether
    its workspace exists already.
    """
    if type(run_number) == int: 
        filename = find_file(run_number)
    elif type(run_number) == list:
        raise TypeError('load_run() cannot handle run lists')
    else:
        # Check if it exists, else tell Mantid to try and 
        # find it
        if os.path.exists(run_number):
            filename = run_number
        else:
            filename = find_file(run_number)
       
    # The output name 
    output_name = os.path.basename(filename)
    if force == False and mtd.workspaceExists(output_name):
        mtd.sendLogMessage("%s already loaded" % filename)
        return mtd[output_name]

    ext = os.path.splitext(filename)[1]
    if filename.endswith("_event.nxs"):
        LoadEventNexus(Filename=filename, OutputWorkspace=output_name) 
    elif ext.startswith(".n"):
        LoadNexus(filename, output_name)
    elif filename.endswith("_event.dat"):
        #load the events
        LoadEventPreNeXus(EventFilename=filename, OutputWorkspace=output_name, PadEmptyPixels=True)       
    else:
        LoadRaw(filename, output_name)
        #LoadDetectorInfo(output_name, filename)

    mtd.sendLogMessage("Loaded %s" % filename)
    is_mono_file = file_type.startswith('mono')
    mark_as_loaded(filename, is_mono_file)
    return mtd[output_name]

def sum_files(accumulator, files, file_type):
    """
    Sum a current workspace and a list of files, accumulating the results in the
    given workspace
    """
    if type(files) == list:
        for filename in files:
            temp = load_run(filename, file_type)
            Plus(accumulator, temp, accumulator)
    else:
        pass

def load_mask(hard_mask):
    """
    Load a hard mask file and return the
    list of spectra numbers it contains as a string

    Each line of the file specifies spectra to be masked by either specifying a range
    using a hypen or a single spectra number using a space as a delimiter between fields i.e.

    48897 - 49152
    50000
    60100-60105
    """
    mask_file = open(hard_mask)
    spectra_list = ""
    for line in mask_file:
        numbers = line.split()
        if len(numbers) == 0:
            continue
        # Any non-numeric character at the start of the line marks a comment, 
        # check the first character of the first word
        if not numbers[0][0].isdigit():
            continue
        num_cols = len(numbers)
        remainder = num_cols % 3
        group_end = num_cols - remainder
        # Jump in steps of 3 where there are whole blocks
        for index in range(0, group_end, 3):
            # Can either have a range specified with a "-" or single numbers
            n_i = numbers[index]
            n_ip1 = numbers[index+1]
            n_ip2 = numbers[index+2]
            # If there is a dash it will have to be the middle value
            if n_ip1 == '-':
                spectra_list += n_i + '-' + n_ip2
            else:
                spectra_list += n_i + "," + n_ip1 + ',' + n_ip2
            spectra_list += ","
        # Now deal with the remainder
        for index in range(group_end,num_cols):
            spectra_list += numbers[index] + ","
            
    if len(spectra_list) < 1:
        mantid.sendLogMessage('Only comment lines found in mask file ' + hard_mask)
        return ''
    # Return everything after the very first comma we added in the line above
    return spectra_list.rstrip(',')
