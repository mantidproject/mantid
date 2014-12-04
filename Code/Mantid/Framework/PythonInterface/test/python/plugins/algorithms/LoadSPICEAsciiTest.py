import unittest
import numpy
from numpy import *
from mantid.kernel import *
from mantid.api import *
import mantid.kernel as kernel
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import os

class LoadSPICEAsciiTest(unittest.TestCase):

    def test_LoadHB2AData(self):
        """ Test to Load HB2A SPICE data
        """
        # Test algorithm
        alg_test = run_algorithm("LoadSPICEAscii",
            Filename = "HB2A_exp0231_scan0001.dat", 
	    OutputWorkspace = "HB2A_0231_0001_Data", 
	    RunInfoWorkspace = "HB2A_0231_Info",
            IgnoreUnlistedLogs = False)

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Output workspace
	datatbws = AnalysisDataService.retrieve("HB2A_0231_0001_Data")
	self.assertTrue(datatbws is not None)

	numcols = datatbws.columnCount()
	self.assertEquals(numcols, 70)
	
	colnames = datatbws.getColumnNames()
	self.assertEquals(colnames[0], "Pt.")

	numrows = datatbws.rowCount()
	self.assertEquals(numrows, 61)

	runinfows = AnalysisDataService.retrieve("HB2A_0231_Info")
	self.assertTrue(runinfows is not None)

	infodict = {}
	numrows = runinfows.rowCount()
	for irow in xrange(numrows):
	    title = runinfows.cell(irow, 0)
	    value = runinfows.cell(irow, 1)
	    infodict[title] = value

	self.assertEquals(infodict['proposal'], 'IPTS-6174')
	self.assertEquals(infodict['runend'], '12:33:21 PM  8/13/2012')


	# Clean
        AnalysisDataService.remove("HB2A_0231_0001_Data")
	AnalysisDataService.remove("HB2A_0231_Info")

        return


    def test_LoadHB3AData(self):
        """ Test to Load HB2A SPICE data
        """
        # Test algorithm
        alg_test = run_algorithm("LoadSPICEAscii",
            Filename = "HB2A_exp0231_scan0001.dat", 
            StringSampleLogNames = "a,b",
            IntSampleLogNames = "",
            FloatSampleLogNames = "", 
	    OutputWorkspace = "HB2A_0231_0001_Data", 
	    RunInfoWorkspace = "HB2A_0231_Info",
            IgnoreUnlistedLogs = True)

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Output workspace
	datatbws = AnalysisDataService.retrieve("HB2A_0231_0001_Data")
	self.assertTrue(datatbws is not None)

	numcols = datatbws.columnCount()
	self.assertEquals(numcols, 70)
	
	colnames = datatbws.getColumnNames()
	self.assertEquals(colnames[0], "Pt.")

	numrows = datatbws.rowCount()
	self.assertEquals(numrows, 61)

	runinfows = AnalysisDataService.retrieve("HB2A_0231_Info")
	self.assertTrue(runinfows is not None)

	infodict = {}
	numrows = runinfows.rowCount()
	for irow in xrange(numrows):
	    title = runinfows.cell(irow, 0)
	    value = runinfows.cell(irow, 1)
	    infodict[title] = value

	self.assertEquals(infodict['proposal'], 'IPTS-6174')
	self.assertEquals(infodict['runend'], '12:33:21 PM  8/13/2012')


	# Clean
        AnalysisDataService.remove("HB2A_0231_0001_Data")
	AnalysisDataService.remove("HB2A_0231_Info")

        return




if __name__ == '__main__':
    unittest.main()

