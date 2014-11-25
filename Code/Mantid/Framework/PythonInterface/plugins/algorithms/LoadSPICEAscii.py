import mantid
import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from mantid.api import AnalysisDataService

class LoadSPICEAscii(PythonAlgorithm):
    """ Create the input TableWorkspaces for LeBail Fitting
    """
    def category(self):
        """
        """
        return 'DataHandling'

    def name(self):
        """
        """
        return "LoadSPICEAscii"

    def summary(self):
        return "Load data file generate from SPICE in ASCII format to table workspaces."

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(FileProperty("Filename", "", FileAction.Load, ['.dat']),
                "Name of SPICE data file.")

        strspckeyprop = StringArrayProperty("StringSampleLogNames", values=[], direction=Direction.Input)
        self.declareProperty(strspckeyprop, "List of log names that will be imported as string property.")

        intspckeyprop = IntArrayProperty("IntSampleLogNames", values=[], direction=Direction.Input)
        self.declareProperty(intspckeyprop, "List of log names that will be imported as integer property.")

        floatspckeyprop = FloatArrayProperty("FloatSampleLogNames", values=[], direction=Direction.Input)
        self.declareProperty(floatspckeyprop, "List of log names that will be imported as float property.")

        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                "Name of TableWorkspace containing experimental data.")

        self.declareProperty(MatrixWorkspaceProperty("RunInfoWorkspace", "", Direction.Output),
                "Name of TableWorkspace containing experimental information.")

        self.declareProperty("IgnoreUnlistedLogs", False, 
                "If it is true, all log names are not listed in any of above 3 input lists will be ignored.  \
                        Otherwise, any log name is not listed will be treated as string property.")

        return

    def PyExec(self):
        """ Main Execution Body
        """
        # Input
        filename = self.getPropertyValue("Filename")

        strlognames = self.getPropertyValue("StringSampleLogNames")
        intlognames = self.getPropertyValue("IntSampleLogNames")
        floatlognames = self.getPropertyValue("FloatSampleLogNames")

        valid = self.validateLogNamesType(floatlognames, intlognames, strlognames)
        if valid is False:
            raise RuntimeError("At one log name appears in multiple log type lists")

	# Parse 
	datalist, titles, runinfodict = self.parseSPICEAscii(filename)

	# Build output workspaces
	outws = self.createDataWS(datalist, titles)

	# Build run information workspace
	runinfows = self.createRunInfoWS(runinfodict, floatlognames, intloagnames, strlognames)

	# Set properties
        self.setProperty("OutputWorkspace", outws)
	self.setProperty('RunInfoWorkspace', runinfows)

        return


    def parseSPICEAscii(self, filename):
        """ Parse SPICE Ascii file to dictionary 
        """
	# Import file
	try:
	    spicefile = open(filename, "r")
	    lines = spicefile.readlines()
	    spicefile.close()
	except IOError, err:
	    raise RuntimeError("Unable to read SPICE file %s." % (filename))

	# Parse
	datalist = []
	infodict = {}

	for line in lines:
	    cline = line.strip()
	    if len(cline) == 0:
		continue

	    if cline.startswith("#"):
		# Remove comment flag #
		cline = cline.split('#')[-1].strip()

		if cline.count('=') >= 1:
		    # run information line
		    terms = cline.split('=', 1)
		    infoname = terms[0].strip()
		    if len(terms) == 2: 
		        infovalue = terms[1].strip()
		    else:
		        infovlaue = None
		    infodict[infoname] = infovalue

		elif cline.count("Pt.") == 1:
		    # Title line
		    titles = cline.split()

		elif cline.endswith('scan completed.'):
		    # Run end
		    infodict['runend'] = cline.split('scan')[0].strip()

		else:
		    # Not supported
		    self.log().warning("Line '%s' cannot be parsed. It is ignored then." % (line))
		    
	    else:
		# data line
		ptdataset = cline.split()
		datalist.append(ptdataset)

	# ENDFOR

	return datalist, titles, infodict

    
    def createDataWS(self, datalist, titles):
	""" Create the table workspace containing experimental data
	Each row is a data point measured in experiment
	"""
        # Create an empty workspace
        tablews = WorkspaceFactory.createTable()

	for title in titles:
	    if title == "Pt.":
		tablews.addColumn("int", title)
	    else:
		tablews.addColumn("double", title)
	# ENDFOR

	# Add rows
	for irow in xrange(len(datalist)):
	    ptdataset = datalist[irow]
	    dataset = [int(ptdataset[0])]
	    for ipt in xrange(1, len(ptdataset)):
		dataset.append(float(ptdataset[ipt]))
	    # add new row
	    tablews.addRow(dataset)
	# ENDFOR

        return tablews


    def createRunInfoWS(self, runinfodict, floatlognamelist, intlognamelist, strlognamelist, ignoreunlisted):
	""" Create run information workspace
	"""
        # Create an empty workspace
        matrixws = api.CreateWorkspace(DataX=[1,2], DataY=[1], DataE=[1], NSpce=1, VerticalAxisUnit="SpectraNumber")

        run = matrixws.getRun()
        
        for title in runinfodict.keys():
            tmpvalue = runinfodict[title]
            if title in floatlognamelist:
                tmpproperty = FloatTimeSeriesProperty(title)
                tmpproperty.addValue(runnend, float(tmpvalue))
            elif title in intlognamelist: 
                tmpproperty = IntTimeSeriesProperty(title)
                tmpproperty.addValue(runnend, int(tmpvalue))
            elif title in strlognamelist or ignoreunlisted is False:
                tmpproperty = StringProperty(title)
                tmpproperty.addValue(runnend, tmpvalue)

            run.addProperty(title, tmpproperty, False)
        # ENDIF

	return matrixws


    def validateLogNamesType(self, floatlognames, intlognames, strlognames):
        """ Check whether 3 sets of values have intersection
        """
        logsets = []
        logsets.append(set(floatlognames))
        logsets.append(set(intlognames))
        logsets.append(set(strlognames))

        for (i, j) in [(0, 1), (0, 2), (1, 2)]:
            if len(logsets[i] - logsets[j]) > 0 or len(logsets[j] - logsets[i]) > 0:
                return False

        return True



# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadSPICEAscii)
