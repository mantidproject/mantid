#------------------------------------------------------------------------------------------------
# Example: Retrieving log information
#------------------------------------------------------------------------------------------------
root = "../../../../Test/Data/"
rawDataTitle="RawFile"
# Load the crisp data set which we have log information for
LoadRaw(Filename=root+"Full point detector CRISP dataset/csp79590.raw",OutputWorkspace=rawDataTitle)

rawWS = mantid.getMatrixWorkspace(rawDataTitle)
logs = rawWS.getSampleDetails().getLogData()

# Print all of the log information
for i in range(0, len(logs)):
	print "----- " + logs[i].name() + " -------\n" +  logs[i].value()
	
logs
# Just get a single log by name
status = rawWS.getSampleDetails().getLogData("status")
print status.value()