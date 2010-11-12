#
# Example: Retrieving log information from a data set
#

datadir = "../../../../Test/Data/"
rawDataTitle="outputSpace"
# Load the crisp data set which we have log information for
alg = LoadRaw(Filename = datadir+"Full point detector CRISP dataset/csp79590.raw",OutputWorkspace=rawDataTitle)

rawWS = alg.workspace()
logs = rawWS.getSampleDetails().getLogData()

# Print all of the log information
for i in range(0, len(logs)):
	print "----- " + logs[i].name() + " -------\n" +  logs[i].value

# Just get a single log by name
status = rawWS.getSampleDetails().getLogData("status")
print status.value
