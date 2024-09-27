# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
"""
Check the loaders of ISIS SANS reduction. It is created as systemtest because it does
take considerable time because it involves loading data. Besides, it uses data that is
currently available inside the systemtests.
"""

import unittest
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import *
import isis_reduction_steps as steps
import ISISCommandInterface as ici
from sans_core.common.enums import SANSInstrument


class LoadRunTest(unittest.TestCase):
    def setUp(self):
        config["default.instrument"] = "SANS2D"
        ici.SANS2D()

    def loadAndAssign(self, run_spec, options=dict()):
        loadRun = steps.LoadRun(str(run_spec), **options)
        loadRun._assignHelper(ici.ReductionSingleton())
        return loadRun

    def passWsAndAssign(self, ws, options=dict()):
        loadRun = steps.LoadRun(ws, **options)
        loadRun._assignHelper(ici.ReductionSingleton())
        return loadRun

    def basicChecks(self, loadRun, file_path, runnum, periods_in_file, ws_name):
        self.assertEqual(loadRun.periods_in_file, periods_in_file)
        self.assertEqual(loadRun.wksp_name, ws_name)
        self.assertEqual(loadRun.shortrun_no, runnum)

        if periods_in_file == 1:
            self.assertEqual(loadRun._wksp_name, ws_name)
            self.assertTrue(not loadRun.move2ws(0))
            self.assertEqual(loadRun.wksp_name, ws_name)
        else:
            self.assertTrue(loadRun.move2ws(0))
            self.assertEqual(loadRun.wksp_name, ws_name)

    def test_single_period_nxs_file(self):
        runnum = 22048
        loadRun = self.loadAndAssign(runnum)
        self.basicChecks(loadRun, "SANS2D00022048.nxs", runnum, 1, "22048_sans_nxs")

        self.assertEqual(loadRun._period, -1)
        self.assertEqual(loadRun.ext, "nxs")

    def test_single_period_raw_file(self):
        runnum = 5547
        loadRun = self.loadAndAssign(runnum)
        self.basicChecks(loadRun, "SANS2D0000%d.raw" % (runnum), runnum, 1, "5547_sans_raw")
        self.assertEqual(loadRun._period, -1)
        self.assertEqual(loadRun.ext, "raw")

    def test_single_period_from_workspace_reload_true(self):
        runnum = 22048
        ws22048 = Load(str(runnum))
        loadRun = self.passWsAndAssign(ws22048)
        self.basicChecks(loadRun, "SANS2D00022048.nxs", runnum, 1, "22048_sans_nxs")

        self.assertEqual(loadRun._period, -1)
        self.assertEqual(loadRun.ext, "nxs")

    def test_single_period_from_workspace_reload_false(self):
        runnum = 22048
        ws22048 = Load(str(runnum))
        loadRun = self.passWsAndAssign(ws22048, {"reload": False})
        self.basicChecks(loadRun, "SANS2D00022048.nxs", runnum, 1, ws22048.name())

        self.assertEqual(loadRun._period, -1)
        self.assertEqual(loadRun.ext, "nxs")

    def test_single_period_trans_raw(self):
        runnum = 988
        loadRun = self.loadAndAssign(runnum, {"trans": True})
        self.basicChecks(loadRun, "SANS2D00000988.raw", runnum, 1, "988_trans_raw")
        self.assertEqual(loadRun._period, -1)
        self.assertEqual(loadRun.ext, "raw")

    def test_multiperiod_nxs_file(self):
        runnum = 5512
        loadRun = self.loadAndAssign(runnum)
        self.basicChecks(loadRun, "SANS2D00005512.nxs", runnum, 13, "5512_sans_nxs_1")
        self.assertEqual(loadRun._period, -1)
        self.assertTrue(loadRun.move2ws(12))
        self.assertEqual(loadRun.wksp_name, "5512_sans_nxs_13")

    def test_multiperiod_from_workspace_reload_false(self):
        runnum = 5512
        ws5512 = Load(str(runnum))
        loadRun = self.passWsAndAssign(ws5512, {"reload": False})
        self.basicChecks(loadRun, "SANS2D00005512.nxs", runnum, 13, ws5512[0].name())
        self.assertEqual(loadRun._period, -1)
        self.assertTrue(loadRun.move2ws(12))
        self.assertEqual(loadRun.wksp_name, ws5512[12].name())

    def test_loading_single_period_in_multiperiod(self):
        runnum = 5512
        loadRun = self.loadAndAssign(runnum, {"entry": 5})
        name = "5512p5_sans_nxs"
        self.basicChecks(loadRun, "SANS2D00005512.nxs", runnum, 1, name)
        self.assertEqual(loadRun._period, 5)
        self.assertTrue(not loadRun.move2ws(1))
        self.assertEqual(loadRun.wksp_name, name)


class LoadSampleTest(unittest.TestCase):
    """LoadSample extends LoadRun in order to move the workspaces to the defined centre"""

    def setUp(self):
        config["default.instrument"] = "SANS2D"
        ici.SANS2D()

    def test_single_period_nxs_file(self):
        ici.SetCentre(1, -2)
        loadSample = steps.LoadSample("22048")
        loadSample.execute(ici.ReductionSingleton(), True)
        self.assertEqual(loadSample.wksp_name, "22048_sans_nxs")
        self.assertTrue(not loadSample.entries)
        cur_pos = ici.ReductionSingleton().instrument.cur_detector_position(loadSample.wksp_name)
        self.assertAlmostEqual(cur_pos[0], 1 / 1000.0)
        self.assertAlmostEqual(cur_pos[1], -2 / 1000.0)

    def test_multiperiod_nxs_file(self):
        ici.SetCentre(1, -2)
        loadSample = steps.LoadSample("5512")
        loadSample.execute(ici.ReductionSingleton(), True)
        self.assertEqual(loadSample.wksp_name, "5512_sans_nxs_1")
        self.assertEqual(loadSample.entries, list(range(0, 13)))
        for index in [0, 5, 12]:
            loadSample.move2ws(index)
            self.assertEqual(loadSample.wksp_name, "5512_sans_nxs_" + str(index + 1))
            cur_pos = ici.ReductionSingleton().instrument.cur_detector_position(loadSample.wksp_name)
            self.assertAlmostEqual(cur_pos[0], 0.001)
            self.assertAlmostEqual(cur_pos[1], -0.002)


@ISISSansSystemTest(SANSInstrument.SANS2D)
class LoadSampleTestSystemTest(systemtesting.MantidSystemTest):
    def runTest(self):
        self._success = False
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(LoadRunTest, "test"))
        suite.addTest(unittest.makeSuite(LoadSampleTest, "test"))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 2000

    def validate(self):
        return self._success


@ISISSansSystemTest(SANSInstrument.SANS2D)
class LoadAddedEventDataSampleTestSystemTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        self._success = False
        config["default.instrument"] = "SANS2D"
        ici.SANS2D()
        ici.MaskFile("MaskSANS2DReductionGUI.txt")
        ici.SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        ici.SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        ici.Gravity(False)
        ici.Set1D()

        ici.add_runs(("22048", "22023"), "SANS2D", "nxs", saveAsEvent=True)

        ici.AssignSample("22023-add.nxs")

        ici.WavRangeReduction(4.6, 12.85, False)

        # Need to do validation ourselves since we have to compare to sets of workspace-file pairs
        if self._validateWorkspaceToNeXusCustom():
            self._success = True

    def _validateWorkspaceToNeXusCustom(self):
        """
        Since we need to compare two have two comparisons, we need to redefine the validateWorkspaceToNexus method here.
        Assumes that the items from self.validate() are many tuples  where the first item is a nexus file and loads it,
        to compare to the supplied workspace which is the second item.
        """
        self.disableChecking.append("Instrument")
        value_pairs = list(self._validateCustom())

        # Make sure we have pairs of two
        if len(value_pairs) % 2 != 0:
            return False

        # For all pairs create a list and run the normal comparison
        validationResult = []

        for index in range(0, len(value_pairs), 2):
            valNames = value_pairs[index : index + 2]

            numRezToCheck = len(valNames)
            mismatchName = None

            validationResult.extend([True])
            for ik in range(0, numRezToCheck, 2):  # check All results
                workspace2 = valNames[ik + 1]
                if workspace2.endswith(".nxs"):
                    Load(Filename=workspace2, OutputWorkspace="RefFile")
                    workspace2 = "RefFile"
                else:
                    raise RuntimeError("Should supply a NeXus file: %s" % workspace2)
                valPair = (valNames[ik], "RefFile")
                if numRezToCheck > 2:
                    mismatchName = valNames[ik]

                if not self.validateWorkspaces(valPair, mismatchName):
                    validationResult[index / 2] = False
                    print("Workspace {0} not equal to its reference file".format(valNames[ik]))
            # end check All results

        # Check if a comparison went wrong
        return all(item is True for item in validationResult)

    def _validateCustom(self):
        return (
            "22023rear_1D_4.6_12.85",
            "SANS2DLoadingAddedEventData.nxs",
            "22023rear_1D_4.6_12.85_incident_monitor",
            "SANS2DLoadingAddedEventDataMonitor.nxs",
        )

    def requiredMemoryMB(self):
        return 2000

    def validateMethod(self):
        return "WorkspaceToNexus"

    def cleanup(self):
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00022023-add.nxs"))

    def validate(self):
        self.disableChecking.append("Instrument")
        return self._success
