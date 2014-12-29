import unittest
import numpy
from numpy import *
from mantid.kernel import *
from mantid.api import *
import mantid.kernel as kernel
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import os

class ExportVulcanSampleLogTest(unittest.TestCase):

    def test_exportFileOnly(self):
        """ Test to export logs without header file
        """
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        alg_test = run_algorithm("ExportSampleLogsToCSVFile",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "furnace20333.txt",
            SampleLogNames = ["SensorA", "SensorB", "SensorC"],
            WriteHeaderFile = False)

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

        # Count lines in the file
        goodlines = 0
        for line in lines:
            line = line.strip()
            if len(line) > 0:
                goodlines += 1
            # ENDIF
        # ENDFOR
        self.assertEquals(goodlines, 25)

        # Remove generated files
        os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS")

        return


    def test_exportFile2(self):
        """ Get a partial of real load frame log values, and set them to
        different logs
        """
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace2()
        AnalysisDataService.addOrReplace("TestMatrixWS2", ws)

        # Test algorithm
        alg_test = run_algorithm("ExportSampleLogsToCSVFile",
            InputWorkspace = "TestMatrixWS2",
            OutputFilename = "furnace20334.txt",
            SampleLogNames = ["SensorA", "SensorB", "SensorC", "SensorD"],
            WriteHeaderFile = False,
            TimeTolerance = 1.0)

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

        # Count lines in the file
        goodlines = 0
        for line in lines:
            line = line.strip()
            if len(line) > 0:
                goodlines += 1
        self.assertEquals(goodlines, 64)

        # Remove generated files
        os.remove(outfilename)
        AnalysisDataService.remove("TestMatrixWS2")

        return


    def test_exportFileAndHeader(self):
        """ Test to export logs without header file
        """
        import os
        import os.path
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        alg_test = run_algorithm("ExportSampleLogsToCSVFile",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "furnace20339.txt",
            SampleLogNames = ["SensorA", "SensorB", "SensorC"],
            WriteHeaderFile = True,
            Header = "SensorA[K]\t SensorB[K]\t SensorC[K]")

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value
        filepath = os.path.dirname(outfilename)
        basename = os.path.basename(outfilename)
        baseheadername = basename.split(".")[0] + "_header.txt"
        headerfilename = os.path.join(filepath, baseheadername)
        try:
            ifile = open(headerfilename)
            lines = ifile.readlines()
            ifile.close()
        except IOError as err:
            errmsg = "Unable to open header file %s. " % (headerfilename)
            self.assertEquals(errmsg, "")
            return

        # Count lines in the file
        goodlines = 0
        for line in lines:
            line = line.strip()
            if len(line) > 0:
                goodlines += 1
        self.assertEquals(goodlines, 3)

        # Clean
        os.remove(outfilename)
        os.remove(headerfilename)
        AnalysisDataService.remove("TestMatrixWS")

        return


    def test_exportUTC(self):
        """ Test to export logs without header file
        """
        import os
        import os.path
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        alg_test = run_algorithm("ExportSampleLogsToCSVFile",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "furnace20339utc.txt",
            SampleLogNames = ["SensorA", "SensorB", "SensorC"],
            WriteHeaderFile = True,
            TimeZone = 'UTC', 
            Header = "SensorA[K]\t SensorB[K]\t SensorC[K]")

        # Validate
        self.assertTrue(alg_test.isExecuted())

        # Locate file
        outfilename = alg_test.getProperty("OutputFilename").value
        filepath = os.path.dirname(outfilename)
        basename = os.path.basename(outfilename)
        baseheadername = basename.split(".")[0] + "_header.txt"
        headerfilename = os.path.join(filepath, baseheadername)
        try:
            ifile = open(headerfilename)
            lines = ifile.readlines()
            ifile.close()
        except IOError as err:
            errmsg = "Unable to open header file %s. " % (headerfilename)
            self.assertEquals(errmsg, "")
            return

        # Count lines in the file
        goodlines = 0
        for line in lines:
            line = line.strip()
            if len(line) > 0:
                goodlines += 1
        self.assertEquals(goodlines, 3)

        # Clean
        os.remove(outfilename)
        os.remove(headerfilename)
        AnalysisDataService.remove("TestMatrixWS")

        return


    def test_exportFileMissingLog(self):
        """ Test to export logs without header file
        """
        # Generate the matrix workspace with some logs
        ws = self.createTestWorkspace()
        AnalysisDataService.addOrReplace("TestMatrixWS", ws)

        # Test algorithm
        alg_test = run_algorithm("ExportSampleLogsToCSVFile",
            InputWorkspace = "TestMatrixWS",
            OutputFilename = "furnace20335.txt",
            SampleLogNames = ["SensorA", "SensorB", "SensorX", "SensorC"],
            WriteHeaderFile = False)

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

        # Clean
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


    def createTestWorkspace2(self):
        """ Create a workspace for testing against with more situation
        """
        from mantid.simpleapi import CreateWorkspace
        from mantid.simpleapi import AddSampleLog
        from time import gmtime, strftime,mktime
        from datetime import datetime, timedelta
        import numpy as np

        # Create a matrix workspace
        x = np.array([1.,2.,3.,4.])
        y = np.array([1.,2.,3.])
        e = np.sqrt(np.array([1.,2.,3.]))
        wksp = CreateWorkspace(DataX=x, DataY=y,DataE=e,NSpec=1,UnitX='TOF')

        # Add run_start
        year = 2014
        month = 2
        day = 15
        hour = 13
        minute = 34
        second = 3
        dtimesec = 0.0010

        timefluc = 0.0001

        #tmptime = strftime("%Y-%m-%d %H:%M:%S", gmtime(mktime(gmtime())))
        runstart = datetime(year, month, day, hour, minute, second)
        AddSampleLog(Workspace=wksp,LogName='run_start',LogText=str(runstart))

        tsp_a = kernel.FloatTimeSeriesProperty("SensorA")
        tsp_b = kernel.FloatTimeSeriesProperty("SensorB")
        tsp_c = kernel.FloatTimeSeriesProperty("SensorC")
        tsp_d = kernel.FloatTimeSeriesProperty("SensorD")
        logs = [tsp_a, tsp_b, tsp_c, tsp_d]

        dbbuf = ""

        random.seed(0)
        for i in arange(25):
            # Randomly pick up log without records
            # first iteration must have all the record
            skiploglist = []
            if i > 0:
                numnorecord = random.randint(-1, 4)
                if numnorecord > 0:
                    for j in xrange(numnorecord):
                        logindex = random.randint(0, 6)
                        skiploglist.append(logindex)
                    # ENDFOR (j)
                # ENDIF (numnorecord)
            # ENDIF (i)

            dbbuf += "----------- %d -------------\n" % (i)

            # Record
            for j in xrange(4):
                # Skip if selected
                if j in skiploglist:
                    continue

                # get random time shifts
                timeshift = (random.random()-0.5)*timefluc

                if i == 0:
                    # first record should have the 'exactly' same time stamps
                    timeshift *= 0.0001

                deltatime = timedelta(i*dtimesec + timeshift)
                tmptime = str(runstart + deltatime)
                tmpvalue = float(i*i*6)+j
                logs[j].addValue(tmptime, tmpvalue)

                dbbuf += "%s: %s = %d\n" % (logs[j].name, tmptime, tmpvalue)

            # ENDFOR (j)
        # ENDFOR (i)

        # print dbbuf

        wksp.mutableRun()['SensorA']=tsp_a
        wksp.mutableRun()['SensorB']=tsp_b
        wksp.mutableRun()['SensorC']=tsp_c
        wksp.mutableRun()['SensorD']=tsp_d

        return wksp

    def Untest_exportVulcanFile(self):
        """ Test to export logs without header file
        File 2: VULCAN_41739_event
        """
        from mantid.simpleapi import Load

        # Generate the matrix workspace with some logs
        Load(Filename="/home/wzz/Projects/MantidProjects/Mantid2/Code/debug/VULCAN_41703_event.nxs",
                OutputWorkspace="VULCAN_41703",
                MetaDataOnly = True,
                LoadLog = True)

        # Test algorithm
        alg_test = run_algorithm("ExportSampleLogsToCSVFile",
            InputWorkspace = "VULCAN_41703",
            OutputFilename = "/tmp/furnace41703.txt",
            SampleLogNames = ["furnace.temp1", "furnace.temp2", "furnace.power"],
            WriteHeaderFile = False)

        # Validate
        self.assertTrue(alg_test.isExecuted())

        return

if __name__ == '__main__':
    unittest.main()

