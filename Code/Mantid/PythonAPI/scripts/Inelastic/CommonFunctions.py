from mantidsimple import *
import os

def find_file(run_number):
    """Use Mantid to search for the given run.
    """    
    file_hint = str(run_number)
    found = FileFinder.findRuns(file_hint)
    if len(found) > 0:
        return found[0]
    else:
        message = 'Cannot find file matching hint "%s" on current search paths\n'
        'for instrument "%s"'
        raise ValueError( message % (file_hint, mtd.settings['default.instrument']))

def create_resultname(run_number, prefix='', suffix=''):
    """Create a string based on the run number and optional prefix and 
    suffix.    
    """
    if type(run_number) == list:
        name = create_resultname(run_number[0], prefix,suffix)
    elif type(run_number) == int:
        name = prefix + str(run_number) + '.spe' + suffix
    else:
        name = os.path.basename(run_number)
        if (suffix is None):
            name = os.path.splitext(name)[0] + '.spe'
        else:
            name = os.path.splitext(name)[0] + '.spe' + suffix
    return name

# Keeps track of loaded files for each piece of the diagnosis/reduction
# so that the raw data files don't stack up and can be deleted when necessary
_loaded_files = \
    {
    'mono-sample': '',
    'white-beam': '',
    'mono-van': '',
    }
# This is temporary so that the correct file can be picked up for LoadDetectorInfo
# When the algorithm is updated it can be moved to where LoadRaw takes place
_last_mono = None

def loaded_file(file_type):
    """Returns the full file path of the loaded
    file. If the file has not been loaded or the file_type
    is unrecognized then it returns an empty string.
    """
    global _last_mono, _loaded_files
    try:
        if file_type.startswith('mono'):
            if _last_mono is None:
                raise ValueError("A mono run cannot be retrieved, none has been loaded yet")
            file_type = _last_mono
        prev_file = _loaded_files[file_type]
    except KeyError:
        mtd.sendLogMessage('Unknown file_type "%s" passed to load_run' % (file_type))
        prev_file = ''
    return prev_file

def load_run(run_number, file_type='mono-sample',force=False):
    """Loads run/runs into the given workspace. The file_type is
    used to keep track the loaded data for various parts of the reduction.
    This ensures that multiple requests to load the same file into the same
    type will result in only a single load.

    If force is true then the file is loaded regardless of whether
    its workspace exists already.
    """
    global _last_mono, _loaded_files
    try:
        prev_file = _loaded_files[file_type]
    except KeyError:
        mtd.sendLogMessage("Unknown file type, forcing load of file")
        force = True
        prev_file = ''
    
    if type(run_number) == int: 
        filename = find_file(run_number)
    else:
        # Check if it exists, else tell Mantid to try and 
        # find it
        if os.path.exists(run_number):
            filename = run_number
        else:
            filename = find_file(run_number)

    # The output name 
    output_name = os.path.basename(filename)
    if force == False and filename == prev_file and \
       mtd.workspaceExists(output_name):
        mtd.sendLogMessage("%s already loaded" % filename)
        return mtd[output_name]

    # Don't have the file already so load it but first the delete the old one
    if mtd.workspaceExists(os.path.basename(prev_file)):
        mtd.deleteWorkspace(os.path.basename(prev_file))

    ext = os.path.splitext(filename)[1]
    if filename.endswith("_event.nxs"):
        LoadSNSEventNexus(Filename=filename, OutputWorkspace=output_name) 
    elif ext.startswith(".n"):
        LoadNexus(filename, output_name)
    elif filename.endswith("_event.dat"):
        #load the events
        LoadEventPreNeXus(EventFilename=filename, OutputWorkspace=output_name, PadEmptyPixels=True)       
    else:
        LoadRaw(filename, output_name)
        #LoadDetectorInfo(output_name, filename)

    mtd.sendLogMessage("Loaded %s" % filename)
    _loaded_files[file_type] = filename
    if file_type.startswith('mono'): 
        _last_mono = file_type

    return mtd[output_name]

def sum_files(accumulator, files, prefix):
    """
    Sum a current workspace and a list of files, acculating the results in the
    given workspace
    """
    if type(files) == list:
        tmp_suffix = '_plus_tmp'
        for filename in files:
            temp = load_run(filename, force=True)
            Plus(accumulator, temp, accumulator)
            mantid.deleteWorkspace(temp.getName())
    else:
        pass

# -- TODO: Remove this stuff in favour of the mask workspace concept --

# returns a string with is comma separated list of the elements in the tuple, array or comma separated string!
def listToString(list):
  stringIt = str(list).strip()
  
  if stringIt == '' : return ''
  if stringIt[0] == '[' or stringIt[0] == '(' :
    stringIt = stringIt[1:]
    lastInd = len(stringIt) - 1
    if stringIt[lastInd] == ']' or stringIt[lastInd] == ')':
      stringIt = stringIt[0:lastInd]
  return stringIt

def stringToList(commaSeparated):
  if commaSeparated == '' : return []
  #remove any leading or trailing ','
  if commaSeparated[0] == ',':
    commaSeparated = commaSeparated[1:]
  if commaSeparated[len(commaSeparated) - 1] == ',':
    commaSeparated = commaSeparated[0:len(commaSeparated) - 1]

  theList = []
  numbers = commaSeparated.split(',')
  for quoted in numbers :
    try :
      num = int(quoted)
      theList.append(num)
    except : #we're not demanding that the entries are all integers
      theList.append(quoted)                       
  return theList
    
def load_mask(hard_mask):
    """
    Load the hard mask file
    """
    inFile = open(hard_mask)
    spectraList = ""
    for line in inFile:
        numbers = line.split()
        if len(numbers) == 0:
            continue
        # Any non-numeric character at the start of the line marks a comment, 
        # check the first character of the first word
        if not numbers[0][0].isdigit():
            continue
        for specNumber in numbers :
            # If there is a hyphen we don't need commas 
            if specNumber == '-' :
                spectraList[len(spectraList) - 1] = spectraList + '-'   
            else: 
                spectraList = spectraList + "," + specNumber

    if len(spectraList) < 1:
        mantid.sendLogMessage('Only comment lines found in mask file ' + hard_mask)
        return ''
    # Return everything after the very first comma we added in the line above
    return spectraList[1:]
