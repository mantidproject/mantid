#
# Example: Retrieving log information from a data set
#

datadir = "../../../repo/Test/Data/"
rawDataTitle="outputSpace"
# Load the crisp data set which we have log information for
ws = LoadRaw(Filename = datadir+"Full point detector CRISP dataset/csp79590.raw",OutputWorkspace=rawDataTitle)

if isinstance(ws,tuple):
    ws = ws[0]

# This is a grouped file so get the first workspace
rawWS = mtd[ws.getNames()[0]]
logs = rawWS.getRun().getLogData()

# Print all of the log information
for i in range(0, len(logs)):
    print "----- " + logs[i].name + " -------\n" +  str(logs[i].value)

# Just get a single log by name
status = rawWS.getRun().getLogData("status")
print status.value
