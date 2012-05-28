from MantidFramework import *
from mantid.simpleapi import *
from LoadEVSRaw import Intervals, sumWsList
from mantid import logger, config

def chunks(l, n):
    for i in xrange(0, len(l), n):
        yield l[i:i+n]

def createTableFromGroup(filenameGroup, propNames):
	for filename in filenameGroup:
		# If we are dealing with raw files then turn off loading log files, since they are often missing
		# and do not contain the log information we are looking for anyway.
		if filename.endswith(".raw"):
			LoadRaw(Filename=filename,LoadLogFiles="0",OutputWorkspace=filename)
		else:
			Load(Filename=filename, OutputWorkspace=filename)

	wsGroup = ', '.join(filenameGroup)
	tableWsName = "INFO_" + wsGroup
	CreateLogPropertyTable(InputWorkspaces=wsGroup, LogPropertyNames=propNames, GroupPolicy="First", OutputWorkspace=tableWsName)
	
	for filename in filenameGroup:
		DeleteWorkspace(Workspace=filename)
		
	return mtd[tableWsName]

def sumTableList(tableList, name):
	if len(tableList) == 0:
		raise ValueError("Unable to sum zero TableWorkspaces.")

	if len(tableList) == 1:
		# If we only have one table to sum, then all we have to do is rename it.
		RenameWorkspace(InputWorkspace=tableList[0].getName(),OutputWorkspace=name)
	else:
		# Else we have multiple tables, and want the first one to contain all the information from the others.
		firstTable = tableList[0]
		otherTables = tableList[1:]
		for table in otherTables:
			for i in range(0, table.rowCount()):
				row = table.row(i)
				firstTable.addRow(row)
		# Rename the first workspace to our result, then get rid of the rest.
		RenameWorkspace(InputWorkspace=firstTable.getName(),OutputWorkspace=name)
		for otherTable in otherTables:
			DeleteWorkspace(Workspace=otherTable)

class RetrieveRunInfo(PythonAlgorithm):
	def category(self):
		return 'Utility;PythonAlgorithms'

	def PyInit(self):
		# Declare algorithm properties.
		self.declareProperty('Runs', '' , MandatoryValidator(), Description='Runs, e.g. \"2012-2020\"')
		self.declareWorkspaceProperty('OutputWorkspace', '' , Direction.Output, Type=ITableWorkspace, Description='Name of workspace into which the result will be put.')

	def PyExec(self):
		CHUNK_SIZE = 10
		PROP_NAMES = "inst_abrv, run_number, user_name, run_title, hd_dur"
		
		# Check we're in ISIS so that PROP_NAMES are present.
		if config["default.facility"] != "ISIS":
			raise RuntimeError("Only ISIS instrument runs are currently supported.")
		
		runString = self.getPropertyValue("Runs")
		outputWsName = self.getPropertyValue("OutputWorkspace")
		
		runs = Intervals.fromString(runString)
		filenames = FileFinder.findRuns(runString)

		# Make sure we have something to work with, and warn user if some files are not present.
		if len(filenames) == 0:
			sys.exit("No files were found.  Quitting.")
		if len(filenames) < len(runs.getValues()):
			logger.error( str(len(runs.getValues()) - len(filenames)) + " files could not be found.  Carrying on with those that were." )

		# Split the file names into groups of given size.  Dealing with the runs in 
		filenameGroups = list(chunks(filenames, CHUNK_SIZE))

		tables = [createTableFromGroup(filenameGroup, PROP_NAMES) for filenameGroup in filenameGroups]

		sumTableList(tables, outputWsName)
		
		# Set as the output workspace.
		self.setPropertyValue('OutputWorkspace', outputWsName)

# Register algorthm with Mantid.
mantid.registerPyAlgorithm(RetrieveRunInfo())