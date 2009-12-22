from mantidsimple import *

# uses the extension to decide whether use LoadNexus or LoadRaw
def LoadNexRaw(filename, workspace):
  # this removes everything after the last . It returns '' if there is no dot or the filename is an empty string (partition always returns three strings)
  extension = filename.rpartition('.')[2]
  if (extension == 'nxs') | (extension == 'NXS') :
    LoadNexus(filename, workspace)
  elif (extension == 'raw') | (extension == 'RAW') :
    LoadRaw(filename, workspace)
  else : raise Exception("Could not find a load function for file "+filename+", *.raw and *.nxs accepted")
  
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
