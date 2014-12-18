import unittest
import numpy
from numpy import *
from mantid.kernel import *
from mantid.api import *
import mantid.kernel as kernel
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import os

class ExportExperimentLogTest(unittest.TestCase):

    def test_exportFileNew(self):
        """ Test to export logs without header file
        """
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "TestRecord.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge"],
            SampleLogOperation = [None, None, "sum"],
            FileMode = "new")

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value
        try:
            print "Output file is %s. " % (outfilename)
            ifile = open(outfilename)
            lines = ifile.readlines()
            ifile.close()
        except IOError as err:
            print "Unable to open file %s. " % (outfilename)
            self.assertTrue(False)
            return

        # Last line cannot be empty, i.e., before EOF '\n' is not allowed
        lastline = lines[-1]
        self.assertTrue(len(lastline.strip()) > 0)

        # Number of lines
        self.assertEquals(len(lines), 2)

        # Check line
        firstdataline = lines[1]
        terms = firstdataline.strip().split("\t")
        self.assertEquals(len(terms), 3)

        #
        # # Remove generated files
        os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS")

        return


    def test_exportFileFastAppend(self):
        """ Test to export logs without header file
        """
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        # create new file
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "TestRecord.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge"],
            SampleLogOperation = [None, None, "sum"],
            FileMode = "new")

        # Fast append
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "TestRecord.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogOperation = [None, None, "sum"],
            FileMode = "fastappend")

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value
        try:
            print "Output file is %s. " % (outfilename)
            ifile = open(outfilename)
            lines = ifile.readlines()
            ifile.close()
        except IOError as err:
            print "Unable to open file %s. " % (outfilename)
            self.assertTrue(False)
            return

        # Last line cannot be empty, i.e., before EOF '\n' is not allowed
        lastline = lines[-1]
        self.assertTrue(len(lastline.strip()) > 0)

        # Number of lines
        self.assertEquals(len(lines), 3)

        # Check line
        firstdataline = lines[1]
        terms = firstdataline.strip().split("\t")
        self.assertEquals(len(terms), 3)

        # Remove generated files
        os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS")

        return


    def test_exportFileAppend(self):
        """ Test to export logs without header file
        """
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        # create new file
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "TestRecord.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge"],
            SampleLogOperation = [None, None, "sum"],
            FileMode = "new")

        # append
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "TestRecord.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge"],
            SampleLogOperation = [None, None, "sum"],
            FileMode = "fastappend")

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value
        try:
            print "Output file is %s. " % (outfilename)
            ifile = open(outfilename)
            lines = ifile.readlines()
            ifile.close()
        except IOError as err:
            print "Unable to open file %s. " % (outfilename)
            self.assertTrue(False)
            return

        # Last line cannot be empty, i.e., before EOF '\n' is not allowed
        lastline = lines[-1]
        self.assertTrue(len(lastline.strip()) > 0)

        # Number of lines
        self.assertEquals(len(lines), 3)

        # Check line
        firstdataline = lines[1]
        terms = firstdataline.strip().split("\t")
        self.assertEquals(len(terms), 3)

        #
        # # Remove generated files
        os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS")

        return


    def test_exportFileAppend2(self):
        """ Test to export file in appending mode
        In this case, the original file will be renamed and a new file will
        be creatd
        """
        import datetime
        import time

        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        # create new file
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "TestRecord.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge"],
            SampleLogOperation = [None, None, "sum"],
            FileMode = "new")

        # append
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "TestRecord.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "SensorA"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "SensorA"],
            SampleLogOperation = [None, None, "sum", "0"],
            FileMode = "append")

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value
        try:
            print "Output file is %s. " % (outfilename)
            ifile = open(outfilename)
            lines = ifile.readlines()
            ifile.close()
        except IOError as err:
            print "Unable to open file %s. " % (outfilename)
            self.assertTrue(False)
            return

        # Last line cannot be empty, i.e., before EOF '\n' is not allowed
        lastline = lines[-1]
        self.assertTrue(len(lastline.strip()) > 0)

        # Number of lines
        self.assertEquals(len(lines), 2)

        # Check line
        firstdataline = lines[1]
        terms = firstdataline.strip().split("\t")
        self.assertEquals(len(terms), 4)

        # Locate the previos file


        # Rename old file and reset the file mode

        # Rename the old one: split path from file, new name, and rename
        fileName, fileExtension = os.path.splitext(outfilename)
        now = datetime.datetime.now()
        nowstr = time.strftime("%Y_%B_%d_%H_%M")
        oldfilename = fileName + "_" + nowstr + fileExtension
        print "Saved old file is %s. " % (oldfilename)
        self.assertTrue(os.path.exists(oldfilename))

        # Remove generated files
        os.remove(outfilename)
        os.remove(oldfilename)
        AnalysisDataService.remove("TestMatrixWS")

        return

    def test_exportFileNewCSV(self):
        """ Test to export logs without header file in csv format
        and with a name not endind with .csv
        """
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "TestRecord.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge"],
            SampleLogOperation = [None, None, "sum"],
            FileMode = "new",
            FileFormat = "comma (csv)")

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value.split(".txt")[0] + ".csv"
        try:
            print "Output file is %s. " % (outfilename)
            ifile = open(outfilename)
            lines = ifile.readlines()
            ifile.close()
        except IOError as err:
            print "Unable to open file %s. " % (outfilename)
            self.assertTrue(False)
            return

        # Last line cannot be empty, i.e., before EOF '\n' is not allowed
        lastline = lines[-1]
        self.assertTrue(len(lastline.strip()) > 0)

        # Number of lines
        self.assertEquals(len(lines), 2)

        # Check line
        firstdataline = lines[1]
        terms = firstdataline.strip().split(",")
        self.assertEquals(len(terms), 3)

        #
        # # Remove generated files
        os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS")

        return

    def createTestWorkspace(self):
        """ Create a workspace for testing against with ideal log values
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

        tsp_a=kernel.FloatTimeSeriesProperty("proton_charge")
        tsp_b=kernel.FloatTimeSeriesProperty("SensorA")
        for i in arange(25):
            tmptime = strftime("%Y-%m-%d %H:%M:%S", gmtime(mktime(gmtime())+i))
            tsp_a.addValue(tmptime, 1.0*i*i)
            tsp_b.addValue(tmptime, 1.234*(i+1))

        wksp.mutableRun()['run_number']="23456"
        wksp.mutableRun()['duration']=342.3
        wksp.mutableRun()['SensorA'] = tsp_b
        wksp.mutableRun()['proton_charge']=tsp_a

        return wksp


if __name__ == '__main__':
    unittest.main()

