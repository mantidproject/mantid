from mantidsimple import *

# uses the extension to decide whether use LoadNexus or LoadRaw
def LoadNexRaw(filename, workspace):
  # this removes everything after the last . It returns '' if there is no dot or the filename is an empty string (partition always returns three strings)
  extension = filename.rpartition('.')[2]
  if (extension == 'nxs') | (extension == 'NXS') :
    LN = LoadNexus(filename, workspace)
    return LN.getPropertyValue('OutputWorkspace')
  if (extension == 'raw') | (extension == 'RAW') :
    LR = LoadRaw(filename, workspace, LoadLogFiles=0)
    return LR.getPropertyValue('OutputWorkspace')
  #we shouldn't get to here, the function should have returned by now
  raise Exception("Could not find a load function for file "+filename+", *.raw and *.nxs accepted")
  
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
  #return everything after the first coma and space we added in the line above
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

