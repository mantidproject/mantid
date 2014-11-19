from mantid.api import *
from mantid.kernel import *
from mantid.api import AnalysisDataService

class LoadSPICEAscii(PythonAlgorithm):
    """ Create the input TableWorkspaces for LeBail Fitting
    """
    def category(self):
        """
        """
        return "Utility;Text"


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

        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                "Name of TableWorkspace containing experimental data.")

        self.declareProperty(ITableWorkspaceProperty("RunInfoWorkspace", "", Direction.Output),
                "Name of TableWorkspace containing experimental information.")

        return

    def PyExec(self):
        """ Main Execution Body
        """
        # Input
        filename = self.getPropertyValue("Filename")

	# Parse 
	datalist, titles, runinfodict = self.parseSPICEAscii(filename)

	# Build output workspaces
	outws = self.createDataWS(datalist, titles)

	# Build run information workspace
	runinfows = self.createRunInfoWS(runinfodict)

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


    def createRunInfoWS(self, runinfodict):
	""" Create run information workspace
	"""
        # Create an empty workspace
        tablews = WorkspaceFactory.createTable()

	tablews.addColumn("str", "title")
	tablews.addColumn("str", "value")

	# Add for
	for title in runinfodict.keys():
	    tablews.addRow([title, runinfodict[title]])
	# ENDFOR

	return tablews

# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadSPICEAscii)
