#
# Example: Retrieving log information from a data set
#

datadir = "../../../repo/Test/Data/"
rawDataTitle="outputSpace"
# Load the crisp data set which we have log information for
alg = LoadRaw(Filename = datadir+"Full point detector CRISP dataset/csp79590.raw",OutputWorkspace=rawDataTitle)

# This is a grouped file so get the first workspace
rawWS = alg.workspace()[0]
logs = rawWS.getRun().getLogData()

# Print all of the log information
for i in range(0, len(logs)):
	print "----- " + logs[i].name + " -------\n" +  str(logs[i].value)

# Just get a single log by name
status = rawWS.getSampleDetails().getLogData("status")
print status.value
