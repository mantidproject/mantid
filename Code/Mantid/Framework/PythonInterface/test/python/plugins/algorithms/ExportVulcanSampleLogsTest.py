import unittest
import os
import numpy 
from numpy import * 
from mantid.kernel import *
from mantid.api import *
import mantid.kernel as kernel
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

class ExportVulcanSampleLogTest(unittest.TestCase):

    def test_exportFileOnly(self):
	""" Test to export logs without header file
	"""
	# Generate the matrix workspace with some logs
	ws = self.createTestWorkspace()
	AnalysisDataService.addOrReplace("TestMatrixWS", ws)

	# Test algorithm
	alg_test = run_algorithm("ExportVulcanSampleLogs", 
	    InputWorkspace = "TestMatrixWS",
	    OutputFilename = "furnace20333.txt",
	    SampleLogNames = ["SensorA", "SensorB", "SensorC"],
	    WriteHeaderFile = False)

	# Validate
	self.assertTrue(alg_test.isExecuted())
        if (alg_test.isExecuted() is False):
            return

        opfilename = alg_test.getProperty("OutputFilename").value

	# Locate file
	try:
	    ifile = open(opfilename)
	    lines = ifile.readlines()
	    ifile.close()
	except IOError as err:
	    self.assertTrue(False)
	    return

	# Count lines in the file
	goodlines = 0
	for line in lines:
	    line = line.strip()
	    if len(line) > 0:
		goodlines += 1
	self.assertEquals(goodlines, 25)

        # Remove output files 
        os.remove(opfilename)

	return


    def test_exportFileAndHeader(self):
	""" Test to export logs without header file
	"""
	# Generate the matrix workspace with some logs
	ws = self.createTestWorkspace()
	AnalysisDataService.addOrReplace("TestMatrixWS", ws)

	# Test algorithm
	alg_test = run_algorithm("ExportVulcanSampleLogs", 
	    InputWorkspace = "TestMatrixWS",
	    OutputFilename = "furnace20333.txt",
	    SampleLogNames = ["SensorA", "SensorB", "SensorC"],
	    WriteHeaderFile = True, 
	    Header = "SensorA[K]\t SensorB[K]\t SensorC[K]")

	# Validate
	self.assertTrue(alg_test.isExecuted())
        if (alg_test.isExecuted() is False):
            return

        opfilename = alg_test.getProperty("OutputFilename").value
        opheadername = opfilename.split(".")[0] + "_header.txt"

	# Locate file
	try:
	    ifile = open(opheadername)
	    lines = ifile.readlines()
	    ifile.close()
	except IOError as err:
	    self.assertTrue(False)
	    return

	# Count lines in the file
	goodlines = 0
	for line in lines:
	    line = line.strip()
	    if len(line) > 0:
		goodlines += 1
	self.assertEquals(goodlines, 3)

        # Remove output files 
        os.remove(opfilename)
        os.remove(opheadername)

	return


    def test_exportFileMissingLog(self):
	""" Test to export logs without header file
	"""
	# Generate the matrix workspace with some logs
	ws = self.createTestWorkspace()
	AnalysisDataService.addOrReplace("TestMatrixWS", ws)

	# Test algorithm
	alg_test = run_algorithm("ExportVulcanSampleLogs", 
	    InputWorkspace = "TestMatrixWS",
	    OutputFilename = "furnace20333.txt",
	    SampleLogNames = ["SensorA", "SensorB", "SensorX", "SensorC"],
	    WriteHeaderFile = False)

	# Validate
	self.assertTrue(alg_test.isExecuted())
        if (alg_test.isExecuted() is False):
            return

        opfilename = alg_test.getProperty("OutputFilename").value

	# Locate file
	try:
	    ifile = open(opfilename)
	    lines = ifile.readlines()
	    ifile.close()
	except IOError as err:
	    self.assertTrue(False)
	    return

	# Count lines in the file
	goodlines = 0
	for line in lines:
	    line = line.strip()
	    if len(line) > 0:
		goodlines += 1
	self.assertEquals(goodlines, 25)

	# Check values
	line0 = lines[0]
	terms = line0.split()
	self.assertEquals(len(terms), 6)
	value2 = float(terms[4])
	self.assertEquals(value2, 0.)

        # Remove generated files
        os.remove(opfilename)

	return

    def createTestWorkspace(self):
        """ Create a workspace for testing against
        """
	from mantid.simpleapi import CreateWorkspace
	from mantid.simpleapi import AddSampleLog
        from time import gmtime, strftime,mktime 
        import numpy as np
      
	# Create a matrix workspace
	x = np.array([1.,2.,3.,4.])
        y = np.array([1.,2.,3.])
        e = np.sqrt(np.array([1.,2.,3.]))
	
	wksp = CreateWorkspace(DataX=x, DataY=y,DataE=e,NSpec=1,UnitX='TOF')

	# Add run_start 
	tmptime = strftime("%Y-%m-%d %H:%M:%S", gmtime(mktime(gmtime())))
	AddSampleLog(Workspace=wksp,LogName='run_start',LogText=str(tmptime))

        tsp_a=kernel.FloatTimeSeriesProperty("SensorA") 
        tsp_b=kernel.FloatTimeSeriesProperty("SensorB") 
        tsp_c=kernel.FloatTimeSeriesProperty("SensorC") 
        for i in arange(25): 
	    tmptime = strftime("%Y-%m-%d %H:%M:%S", gmtime(mktime(gmtime())+i))
            tsp_a.addValue(tmptime, 1.0*i*i) 
            tsp_b.addValue(tmptime, 2.0*i*i) 
            tsp_c.addValue(tmptime, 3.0*i*i) 

        wksp.mutableRun()['SensorA']=tsp_a
        wksp.mutableRun()['SensorB']=tsp_b
        wksp.mutableRun()['SensorC']=tsp_c

        return wksp

if __name__ == '__main__':
    unittest.main()

