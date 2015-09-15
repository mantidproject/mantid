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
            OutputFilename = "TestRecord001.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "MinPCharge", "MeanPCharge"],
            SampleLogOperation = [None, None, "sum", "min", "average"],
            FileMode = "new")

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value
        try:
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
        self.assertEquals(len(terms), 5)

        # Get property
        pchargelog = ws.getRun().getProperty("proton_charge").value
        sumpcharge = numpy.sum(pchargelog)
        minpcharge = numpy.min(pchargelog)
        avgpcharge = numpy.average(pchargelog)

        v2 = float(terms[2])
        self.assertAlmostEqual(sumpcharge, v2)
        v3 = float(terms[3])
        self.assertAlmostEqual(minpcharge, v3)
        v4 = float(terms[4])
        self.assertAlmostEqual(avgpcharge, v4)

        # Remove generated files
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


    def test_sortRecordFile(self):
        """ Test to append logs and sort the log record file
        """
        # Record 0
        ws1 = self.createTestWorkspace(run=10000)
        AnalysisDataService.addOrReplace("TestMatrixWS1", ws1)

        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS1",
            OutputFilename = "TestRecord9.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge"],
            SampleLogOperation = [None, None, "min"],
            FileMode = "new",
            FileFormat = "comma (csv)",
            OrderByTitle = 'RUN')


        # Record 1
        ws2 = self.createTestWorkspace(run=11000)
        AnalysisDataService.addOrReplace("TestMatrixWS2", ws2)

        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS2",
            OutputFilename = "TestRecord9.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge"],
            SampleLogOperation = [None, None, "min"],
            FileMode = "fastappend",
            FileFormat = "comma (csv)",
            OrderByTitle = 'RUN')

        # Record 2
        ws3 = self.createTestWorkspace(run=10023)
        AnalysisDataService.addOrReplace("TestMatrixWS3", ws3)

        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS3",
            OutputFilename = "TestRecord9.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge"],
            SampleLogOperation = [None, None, "min"],
            FileMode = "fastappend",
            FileFormat = "comma (csv)",
            OrderByTitle = 'RUN')

        # Verify
        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value.split(".txt")[0] + ".csv"
        try:
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
        self.assertEquals(len(lines), 4)

        # Check value
        for i in xrange(1, 3):
            currline = lines[i]
            curr_run = int(currline.split(",")[0])
            curr_min = float(currline.split(",")[2])
            nextline = lines[i+1]
            next_run = int(nextline.split(',')[0])
            next_min = float(nextline.split(',')[2])
            self.assertTrue(curr_run < next_run)
            self.assertTrue(curr_min < next_min)


        # Remove generated files
        os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS1")
        AnalysisDataService.remove("TestMatrixWS2")
        AnalysisDataService.remove("TestMatrixWS3")

        return

    def test_removeDupRecord(self):
        """ Test to append logs and sort the log record file
        """
        # Record 0
        ws1 = self.createTestWorkspace(run=10000)
        AnalysisDataService.addOrReplace("TestMatrixWS1", ws1)

        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS1",
            OutputFilename = "TestRecord11.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "ProtonCharge-Avg"],
            SampleLogOperation = [None, None, "min", "average"],
            FileMode = "new",
            FileFormat = "tab",
            OverrideLogValue = ["Duration", "12345", "ProtonCharge-Avg", "32.921"],
            RemoveDuplicateRecord = True, 
            OrderByTitle = 'RUN')

        # Record 0B
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS1",
            OutputFilename = "TestRecord11.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "ProtonCharge-Avg"],
            SampleLogOperation = [None, None, "min", "average"],
            FileMode = "fastappend",
            FileFormat = "tab",
            OverrideLogValue = ["Duration", "12345", "ProtonCharge-Avg", "32.921"],
            RemoveDuplicateRecord = True, 
            OrderByTitle = 'RUN')

        # Record 1
        ws2 = self.createTestWorkspace(run=11000)
        AnalysisDataService.addOrReplace("TestMatrixWS2", ws2)

        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS2",
            OutputFilename = "TestRecord11.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "ProtonCharge-Avg"],
            SampleLogOperation = [None, None, "min", "average"],
            FileMode = "fastappend",
            FileFormat = "tab",
            OverrideLogValue = ["Duration", "23456", "ProtonCharge-Avg", "22.921"],
            RemoveDuplicateRecord = True, 
            OrderByTitle = 'RUN')

        # Record 2
        ws3 = self.createTestWorkspace(run=10023)
        AnalysisDataService.addOrReplace("TestMatrixWS3", ws3)

        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS3",
            OutputFilename = "TestRecord11.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "ProtonCharge-Avg"],
            SampleLogOperation = [None, None, "min", "average"],
            FileMode = "fastappend",
            FileFormat = "tab",
            OverrideLogValue = ["Duration", "34567", "ProtonCharge-Avg", "12.921"],
            RemoveDuplicateRecord = True, 
            OrderByTitle = 'RUN')

        # Record 2B
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS3",
            OutputFilename = "TestRecord11.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "ProtonCharge-Avg"],
            SampleLogOperation = [None, None, "min", "average"],
            FileMode = "fastappend",
            FileFormat = "tab",
            OverrideLogValue = ["Duration", "34567", "ProtonCharge-Avg", "12.921"],
            RemoveDuplicateRecord = True, 
            OrderByTitle = 'RUN')

        # Verify
        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value
        try:
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
        self.assertEquals(len(lines), 4)

        # Check value
        for i in xrange(1, 3):
            currline = lines[i]
            curr_run = int(currline.split("\t")[0])
            curr_min = float(currline.split("\t")[2])
            nextline = lines[i+1]
            next_run = int(nextline.split('\t')[0])
            next_min = float(nextline.split('\t')[2])
            self.assertTrue(curr_run < next_run)
            self.assertTrue(curr_min < next_min)

        line2 = lines[2]
        terms = line2.split("\t")
        duration = int(terms[1])
        self.assertEquals(duration, 34567)
        pchargeavg = float(terms[3])
        self.assertAlmostEqual(pchargeavg, 12.921)


        # Remove generated files
        os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS1")
        AnalysisDataService.remove("TestMatrixWS2")
        AnalysisDataService.remove("TestMatrixWS3")

        return


    def test_sortRecordFileOverride(self):
        """ Test to append logs and sort the log record file
        """
        # Record 0
        ws1 = self.createTestWorkspace(run=10000)
        AnalysisDataService.addOrReplace("TestMatrixWS1", ws1)

        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS1",
            OutputFilename = "TestRecord10.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "ProtonCharge-Avg"],
            SampleLogOperation = [None, None, "min", "average"],
            FileMode = "new",
            FileFormat = "tab",
            OverrideLogValue = ["Duration", "12345", "ProtonCharge-Avg", "32.921"],
            OrderByTitle = 'RUN')


        # Record 1
        ws2 = self.createTestWorkspace(run=11000)
        AnalysisDataService.addOrReplace("TestMatrixWS2", ws2)

        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS2",
            OutputFilename = "TestRecord10.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "ProtonCharge-Avg"],
            SampleLogOperation = [None, None, "min", "average"],
            FileMode = "fastappend",
            FileFormat = "tab",
            OverrideLogValue = ["Duration", "23456", "ProtonCharge-Avg", "22.921"],
            OrderByTitle = 'RUN')

        # Record 2
        ws3 = self.createTestWorkspace(run=10023)
        AnalysisDataService.addOrReplace("TestMatrixWS3", ws3)

        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS3",
            OutputFilename = "TestRecord10.txt",
            SampleLogNames = ["run_number", "duration", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "ProtonCharge", "ProtonCharge-Avg"],
            SampleLogOperation = [None, None, "min", "average"],
            FileMode = "fastappend",
            FileFormat = "tab",
            OverrideLogValue = ["Duration", "34567", "ProtonCharge-Avg", "12.921"],
            OrderByTitle = 'RUN')

        # Verify
        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value
        try:
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
        self.assertEquals(len(lines), 4)

        # Check value
        for i in xrange(1, 3):
            currline = lines[i]
            curr_run = int(currline.split("\t")[0])
            curr_min = float(currline.split("\t")[2])
            nextline = lines[i+1]
            next_run = int(nextline.split('\t')[0])
            next_min = float(nextline.split('\t')[2])
            self.assertTrue(curr_run < next_run)
            self.assertTrue(curr_min < next_min)

        line2 = lines[2]
        terms = line2.split("\t")
        duration = int(terms[1])
        self.assertEquals(duration, 34567)
        pchargeavg = float(terms[3])
        self.assertAlmostEqual(pchargeavg, 12.921)


        # Remove generated files
        os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS1")
        AnalysisDataService.remove("TestMatrixWS2")
        AnalysisDataService.remove("TestMatrixWS3")

        return

    def test_exportFileUTC(self):
        """ Test to export logs without header file
        """
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        alg_test = run_algorithm("ExportExperimentLog",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "TestRecord001utc.txt",
            SampleLogNames = ["run_number", "duration", "run_start", "proton_charge", "proton_charge", "proton_charge"],
            SampleLogTitles = ["RUN", "Duration", "StartTime", "ProtonCharge", "MinPCharge", "MeanPCharge"],
            SampleLogOperation = [None, None, "time", "sum", "min", "average"],
            TimeZone = 'UTC',
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
        self.assertEquals(len(terms), 6)

        # Get property
        runstarttime = ws.run().getProperty("run_start").value
        pchargelog = ws.getRun().getProperty("proton_charge").value
        sumpcharge = numpy.sum(pchargelog)
        minpcharge = numpy.min(pchargelog)
        avgpcharge = numpy.average(pchargelog)

        # run start time
        v2 = str(terms[2])
        self.assertEqual(runstarttime, v2.split("UTC")[0].strip())

        v3 = float(terms[3])
        self.assertAlmostEqual(sumpcharge, v3)

        v4 = float(terms[4])
        self.assertAlmostEqual(minpcharge, v4)

        v5 = float(terms[5])
        self.assertAlmostEqual(avgpcharge, v5)

        # Remove generated files
        # os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS")

        return


    def createTestWorkspace(self, run=23456):
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
            if run == 23456: 
                shift = 0
            else:
                shift = int(run)
            tsp_a.addValue(tmptime, 1.0*i*i + shift)
            tsp_b.addValue(tmptime, 1.234*(i+1))

        wksp.mutableRun()['run_number']=str(run)
        wksp.mutableRun()['duration']=342.3
        wksp.mutableRun()['SensorA'] = tsp_b
        wksp.mutableRun()['proton_charge']=tsp_a

        return wksp


if __name__ == '__main__':
    unittest.main()

