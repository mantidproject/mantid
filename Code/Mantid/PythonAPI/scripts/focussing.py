import re
###########################################################################
#the ouput from the file reading will be a dictionary of lists
def LoadGrouping(filename):
	groupDictionary = {}
	#comile regular expression
	calPattern = re.compile(r'\s+(\d+)\s+(\d+)\s+((\d|\.)+)\s+(\d+)\s+(\d+)') 

	f=open(filename)
	for line in f:
		# now parse the line with an regular expression
		results=calPattern.search(line)
		if results:
			#we are interested in groups 1 (udet), 4(select) and 5 (group)
			udet = int(results.groups()[1])
			select = int(results.groups()[4])
			group = int(results.groups()[5])
			if select >0:
				if groupDictionary.has_key(group):
					groupDictionary[group].append(udet)
				else:
					#add as a new list
					groupDictionary[group]=[udet]	
	f.close()
	return groupDictionary

###########################################################################
def LoadRawSegment(filename,workspaceName, spectraList):
	print "loading " + workspaceName
	alg=mtd.createAlgorithm("LoadRaw",-1)
	alg.setPropertyValue("Filename",filename)
	alg.setPropertyValue("OutputWorkspace",workspaceName)
	spectraString = ",".join(["%s" % i for i in spectraList])
	alg.setPropertyValue("spectrum_list",spectraString)
	return alg.execute()
	
###########################################################################
def DeleteWorkspace(workspaceName):
	print('Removing workspace: ' + workspaceName)
	mtd.deleteWorkspace(workspaceName)
	return
	
###########################################################################
def Focus(calfilename,workspaceName, outputWorkspace):
	print "Focussing " + workspaceName
	alg=mtd.createAlgorithm("DiffractionFocussing",-1)
	alg.setPropertyValue("InputWorkspace",workspaceName)
	alg.setPropertyValue("OutputWorkspace",outputWorkspace)
	alg.setPropertyValue("GroupingFileName",calfilename)
	return alg.execute()
	
	
###########################################################################
def ConvertUdettoSpectrumNumber(groupDictionary,dataFilename):
	#load file with a minimum of data
	tempWorkspaceName = "_temp_ConvertUdettoSpectrumNumber"
	LoadRawSegment(dataFilename,tempWorkspaceName,range(100,200))
	
	#get a handle to the spectra-detector map
	tempworkspace=mtd.getMatrixWorkspace(tempWorkspaceName)
	sdMap = tempworkspace.getSpectraMap()
	
	for groupNumber in groupDictionary.keys():
		#for each udet array convert to spectra number
		detList = groupDictionary[groupNumber]
		detVec = MantidPythonAPI.IntVec()
		for det in detList:
			detVec.push_back(det)
		specList = sdMap.getSpectra(detVec)
		groupDictionary[groupNumber] = StripZeros(specList)

	#delete temp workspace
	DeleteWorkspace(tempWorkspaceName)
	return

###########################################################################
def StripZeros(list):
	outputList = []
	for item in list:
		if item != 0:
			outputList.append(item)
	return outputList;

###########################################################################
#Main program
###########################################################################
calFilename = "Data/offsets_2006_cycle064.cal"
dataFilename = "Data/GEM38370.raw"
outputWorkspaceList = []

groupDictionary = LoadGrouping(calFilename)
ConvertUdettoSpectrumNumber(groupDictionary,dataFilename)

for groupNumber in groupDictionary.keys():
	#load temporary workspace
	workspaceName = "_inputWork" + str(groupNumber)
	LoadRawSegment(dataFilename,workspaceName,groupDictionary[groupNumber])
	#focus the group
	outputWorkspace = "_outputwork" + str(groupNumber)
	Focus(calFilename,workspaceName,outputWorkspace)
	#delete temp workspace
	DeleteWorkspace(workspaceName)
	#record result workspace
	outputWorkspaceList.append(outputWorkspace)
	
print ("\n\nResults")
for workspaceName in outputWorkspaceList:
	print ("\t" + workspaceName)
	


	

	
