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
