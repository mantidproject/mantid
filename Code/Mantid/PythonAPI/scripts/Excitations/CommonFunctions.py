from mantidsimple import *

# returns a string with is comma separated list of the elements in the tuple, array or comma separated string!
def listToString(list):
  stringIt = str(list)
  if stringIt[0] == '[' or stringIt[0] == '(' :
    # we get here if we were passed an array or tuple
    lastInd = len(stringIt)-1
    if stringIt[lastInd] == ']' or stringIt[lastInd] == ')':
      # only need to knock the brackets off
      stringIt = stringIt[1:(lastInd-1)]
  return stringIt

def stringToList(commaSeparated):
  if commaSeparated == '' : return []
  theList = []
  numbers = commaSeparated.split(',')
  for quoted in numbers :
    theList.append(int(quoted))
  return theList
    
#sum all the workspaces, when the workspaces are not summed single input files are specified in this file and the final Python script is made of many copies of this file
def sumWorkspaces(total, runNumbers):
    if len(runNumbers) > 1:
      for toAdd in runNumbers[ 1 : ] :
        #workspaces are overwritten to save memory
        instru.loadRunNumber(toAdd, conv.tempWS)
        Plus(total, conv.tempWS, total)
      mantid.deleteWorkspace(conv.tempWS)
  
#-- Functions to do with input files
# uses the extension to decide whether use LoadNexus or LoadRaw
def LoadNexRaw(filename, workspace):
  # this removes everything after the last, partition always returns three strings
  extension = filename.rpartition('.')[2]
  if (extension == 'nxs') | (extension == 'NXS') :
    #return the first property from the algorithm, which for LoadNexus is the output workspace
    return LoadNexus(filename, workspace)[0]
  if (extension == 'raw') | (extension == 'RAW') :
    return LoadRaw(filename, workspace, LoadLogFiles=0)[0]
  #we shouldn't get to here, the function should have returned by now
  raise Exception("Could not find a load function for file "+filename+", *.raw and *.nxs accepted")

# guess the filename from run number using the instrument code
def getFileName(instrumentPref, runNumber):
  runNumber = str(runNumber)
  try :
    number = int(runNumber, 10)
  except TypeError :
    # means we weren't passed a number assume it is a valid file name and return it unprocessed
    return runNumber
  #only raw files are supported at the moment
  return instrumentPref + runNumber + '.raw'

def loadRunNumber(instru, runNum, workspace):
  return LoadNexRaw(getFileName(instru, runNum), workspace)
  
def loadMask(MaskFilename):
  inFile = open(MaskFilename)
  spectraList = ""
  for line in inFile:
    # for each line separate all the numbers (or words) into array
    numbers = line.split()
    # if the line didn't start with a number reject that whole line
    if len(numbers) > 0 :
      if numbers[0].isdigit() :
        for specNumber in numbers :
          spectraList = spectraList + ", " + specNumber
  #return everything after the first comma and space we added in the line above
  return spectraList[2:]
	
def getRunName(path):
  # get the string after the last /
  filename = path.split('/')
  filename = filename[len(filename)-1]
  # and the last \
  filename = filename.split('\\')
  filename = filename[len(filename)-1]
  # remove the last '.' and everything after it i.e. the extension. If there is not extension this just returns the whole thing
  return filename.rpartition('.')[0]
  
#-- Holds data about the defaults used for diferent instruments (MARI, MAPS ...)
class defaults:
  # set the defaults for a default machine. They won't work and so they must be overriden by the correct machine
  def __init__(self, backgroundRange=(-1, -1), normalization='not set', instrument_pref=''):
    self.backgroundRange = backgroundRange
    self.normalization = normalization
    self.instrument_pref = instrument_pref
      
  # guess the filename from run number assuming global_getFileName_instrument_pref is setup
  def getFileName(self, runNumber):
    try :
      number = int(str(runNumber), 10)
    except TypeError :
	# means we weren't passed a number assume it is a valid file name and return it unprocessed
	    return runNumber
    return getFileName(self.instrument_pref, number)
    
  def loadRunNumber(self, runNum, workspace):
    return LoadNexRaw(self.getFileName(runNum), workspace)