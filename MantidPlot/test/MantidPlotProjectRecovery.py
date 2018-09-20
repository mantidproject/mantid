import mantidplottests
import os
import platform
import unittest
import mantidplot
from mantidplottests import runTests, threadsafe_call
from glob import glob
#from mantid.kernel import *
from mantid import AnalysisDataService, ConfigService
from mantid.simpleapi import CreateWorkspace
from PyQt4 import QtGui
import shutil
import _qti

path = os.path.join(ConfigService.getAppDataDirectory(), 'recovery', platform.node())

def cleanUp():
    # Clean Up
    for cc in glob(os.path.join(path, "*", "")):
        shutil.rmtree(cc)
    AnalysisDataService.clear()

class MantidPlotProjectRecovery(unittest.TestCase):



    def test_exec(self):
        CreateWorkspace(OutputWorkspace="ws", DataX=[
                        1, 2, 3], DataY=[1, 2, 3], NSpec=1)
        threadsafe_call(_qti.app.saveRecoveryCheckpoint)
        
        cleanUp()
        #Test that it cleaned up after itself
        self.assertEqual(len(glob(os.path.join(path, "*", ""))), 0)

    def test_checkpoint_creation(self):
        CreateWorkspace(OutputWorkspace="ws", DataX=[
                        1, 2, 3], DataY=[1, 2, 3], NSpec=1)

        listOfCheckpointsBefore = glob(os.path.join(path, "*", ""))
        for ii in listOfCheckpointsBefore:
            shutil.rmtree(ii)

        threadsafe_call(_qti.app.saveRecoveryCheckpoint)
        listOfCheckpointsAfter = glob(os.path.join(path, "*", "")) 

        self.assertEqual(len(listOfCheckpointsAfter), 1)

        cleanUp()
        #Test that it cleaned up after itself
        self.assertEqual(len(glob(os.path.join(path, "*", ""))), 0)


# Run the unit tests
mantidplottests.runTests(MantidPlotProjectRecovery)
