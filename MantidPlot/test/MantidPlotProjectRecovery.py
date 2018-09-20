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


class MantidPlotProjectRecovery(unittest.TestCase):

    def test_exec(self):
        CreateWorkspace(OutputWorkspace="ws", DataX=[
                        1, 2, 3], DataY=[1, 2, 3], NSpec=1)
        threadsafe_call(_qti.app.saveRecoveryCheckpoint)
        AnalysisDataService.clear()

    def test_checkpoint_creation(self):
        CreateWorkspace(OutputWorkspace="ws", DataX=[
                        1, 2, 3], DataY=[1, 2, 3], NSpec=1)
        path = os.path.join(
            ConfigService.getUserPropertiesDir(), 'recovery', platform.node())
        listOfCheckpointsBefore = glob(os.path.join(path, "*", ""))
        for ii in listOfCheckpointsBefore:
            shutil.rmtree(ii)
        threadsafe_call(_qti.app.saveRecoveryCheckpoint)
        AnalysisDataService.clear()
        listOfCheckpointsAfter = glob(os.path.join(path, "*", ""))
        self.assertTrue(len(listOfCheckpointsAfter) == 1)


# Run the unit tests
mantidplottests.runTests(MantidPlotProjectRecovery)
