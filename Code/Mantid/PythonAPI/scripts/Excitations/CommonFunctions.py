from mantidsimple import *

# returns a string with is comma separated list of the elements in the tuple, array or comma separated string!
def listToString(list):
  stringIt = str(list).strip()                                                   #remove any white space from the front and end
  
  if stringIt == '' : return ''
  if stringIt[0] == '[' or stringIt[0] == '(' :
    stringIt = stringIt[1:]
    lastInd = len(stringIt)-1
    if stringIt[lastInd] == ']' or stringIt[lastInd] == ')':
      stringIt = stringIt[0:lastInd]
  return stringIt

def stringToList(commaSeparated):
  if commaSeparated == '' : return []
  #remove any leading or trailing ','
  if commaSeparated[0] == ',':
    commaSeparated = commaSeparated[1:]
  if commaSeparated[len(commaSeparated)-1] == ',':
    commaSeparated = commaSeparated[0:len(commaSeparated)-1]

  theList = []
  numbers = commaSeparated.split(',')
  for quoted in numbers :
    try :
      num = int(quoted)
      theList.append(num)
    except : #we're not demanding that the entries are all integers
      theList.append(quoted)                       
  return theList
    
#sum all the workspaces, when the workspaces are not summed single input files are specified in this file and the final Python script is made of many copies of this file
def sumWorkspaces(total, prefix, runNumbers):
  if len(runNumbers) > 1:
    tempWS = 'CommonFuncs_sumWorkspaces_tempory'
    for toAdd in runNumbers[1:]:
      loadRun(prefix, toAdd, tempWS)
      Plus(total, tempWS, total)
    mantid.deleteWorkspace(tempWS)
  
#-- Functions to do with input files
# uses the extension to decide whether use LoadNexus or LoadRaw
def LoadNexRaw(filename, workspace):
  filename = filename.strip()                                                   #remove any white space from the front or end of the name
  # this removes everything after the last '.'  (rpartition always returns three strings)
  partitions = filename.split('.')
  if len(partitions) != 2:
    raise RuntimeError('Incorrect filename format encountered "' + filename + '"')
  extension = filename.split('.')[1]
  if (extension.lower() == 'raw'):
    loader = LoadRaw(filename, workspace)
  elif (extension.lower() == 'nxs'):
    #return the first property from the algorithm, which for LoadNexus is the output workspace
    loader = LoadNexus(filename, workspace)
  else:
    raise Exception("Could not find a load function for file "+filename+", *.raw and *.nxs accepted")
  return loader.workspace(),loader.getPropertyValue("Filename")

# guess the filename from run number using the instrument code
def getFileName(instrumentPref, runNumber):
  runNumber = str(runNumber)
  try:
    number = int(runNumber)
  except ValueError:
    # means we weren't passed a number assume it is a valid file name and return it unprocessed
    return runNumber
  #only raw files are supported at the moment
  return instrumentPref + runNumber.zfill(5) + '.raw'

def loadRun(prefix, runNum, workspace):
  return LoadNexRaw(getFileName(prefix, runNum), workspace)
  
def loadMask(MaskFilename):
  inFile = open(MaskFilename)
  spectraList = ""
  for line in inFile:                                             # for each line separate all the numbers (or words) into array
    numbers = line.split()
    if len(numbers) > 0 :                                         # ignore empty lines
      if numbers[0][0].isdigit() :                                # any non-numeric character at the start of the line marks a comment, check the first character of the first word
        for specNumber in numbers :
          if specNumber == '-' :
            spectraList[len(spectraList)-1] = spectraList + '-'   #if there is a hyphen we don't need commas 
          else : spectraList = spectraList + "," + specNumber

  if len(spectraList) < 1 :
    mantid.sendLogMessage('Only comment lines found in mask file '+MaskFilename)
    return ''
  return spectraList[1:]                                          #return everything after the very first comma we added in the line above

def getRunName(path):
  # get the string after the last /
  filename = path.split('/')
  filename = filename[len(filename)-1]
  # and the last \
  filename = filename.split('\\')
  filename = filename[len(filename)-1]
  # remove the last '.' and everything after it i.e. the extension. If there is not extension this just returns the whole thing
  return filename.rpartition('.')[0]
  
def loadRun(prefix, runNum, workspace):
    return LoadNexRaw(getFileName(prefix,runNum), workspace)

#-- Holds data about the defaults used for diferent instruments (MARI, MAPS ...)
class defaults:
  # set the defaults for a default machine. These default values for defaults won't work and so they must be overriden by the correct values for the machine when this is run
  def __init__(self, background_range=(-1.0,-1.0), normalization='not set', instrument_pref='', white_beam_integr=(-1.0,-1.0), \
                scale_factor=1, monitor1_integr=(-1.0e5, -1.0e5), white_beam_scale = 1.0, getei_monitors = (-1,-1)):
    self.background_range=background_range
    self.normalization=normalization
    self.instrument_pref=instrument_pref
    self.white_beam_integr=white_beam_integr
    self.scale_factor=scale_factor
    self.monitor1_integr=monitor1_integr
    self.white_beam_scale = white_beam_scale
    self.getei_monitors = getei_monitors
      
  # guess the filename from run number assuming global_getFileName_instrument_pref is setup
  def getFileName(self, runNumber):
    try :
      number = int(str(runNumber), 10)
    except Exception :
      # means we weren't passed a number assume it is a valid file name and return it unprocessed
      return runNumber
    return getFileName(self.instrument_pref, number)
    
