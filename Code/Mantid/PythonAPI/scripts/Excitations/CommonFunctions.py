def loadMask(MaskFilename):
  inFile = open(MaskFilename)
  spectraList = ""
  for line in inFile:
    if line[0].isdigit():
      numbers = line.split()
      for specNumber in numbers:
        spectraList = spectraList + ", " + specNumber
  #get rid of the first coma and space
  return spectraList[2:]