import unittest
import mantid.simpleapi as simpleapi


class TestSimpleAPI(unittest.TestCase):

    def tearDown(self):
        simpleapi.AnalysisDataService.clear()

    def test_inplace(self):
        ws = simpleapi.CreateSampleWorkspace()
        # Verify that CreateSampleWorkspace is still producing an input workspace
        # with same values as used when computing 2.94020841 below.
        self.assertAlmostEqual(ws.dataX(0)[0], 0.0)
        old_id = id(ws)
        simpleapi.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='Energy')
        # ConvertUnits is in-place, so ws is still the same
        self.assertEqual(id(ws), old_id)
        # And has new units
        self.assertAlmostEqual(ws.dataX(0)[0], 2.94020841e+00)

    def test_output_silent_replace(self):
        ws_in = simpleapi.CreateSampleWorkspace()
        ws = simpleapi.CreateSampleWorkspace()
        name = str(ws.name())
        simpleapi.Rebin(InputWorkspace=ws_in, OutputWorkspace=ws, Params=2)
        # Rebin makes a copy of ws_in and gives it the same name as ws. When
        # that output is put in the ADS 'addOrReplace' is called, i.e., the
        # original workspace it kicked out of the ADS. Since in simpleapi a
        # Python workspace variable is a weak_ptr it es deleted.
        # Note also that simpleapi does not update the raw pointer inside ws,
        # i.e., ws does not point to the newly created output workspace either.
        self.assertTrue(name in simpleapi.AnalysisDataService)
        self.assertRaises(RuntimeError, ws.name)

    def test_scope_violation(self):
        def scope_violator():
            # This implicitly write to a global variable (in the ADS) with name "ws".
            ws = simpleapi.CreateSampleWorkspace()
        ws = simpleapi.CreateSampleWorkspace()
        scope_violator()
        # ws still refers to the old object
        self.assertRaises(RuntimeError, ws.name)

    def test_python_container(self):
        workspaces = []
        for i in range(2):
            ws = simpleapi.CreateSampleWorkspace()
            workspaces.append(ws)
        # Cannot use Mantid workspaces in Python container, risk of deletion.
        self.assertRaises(RuntimeError, workspaces[0].name)


if __name__ == '__main__':
    unittest.main()
