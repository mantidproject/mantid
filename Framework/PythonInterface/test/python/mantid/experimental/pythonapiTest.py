import unittest
import mantid.experimental.pythonapi as mantid


class TestPythonAPI(unittest.TestCase):

    def test_inplace(self):
        ws = mantid.CreateSampleWorkspace()
        # Verify that CreateSampleWorkspace is still producing an input workspace
        # with same values as used when computing 2.94020841 below.
        self.assertAlmostEqual(ws.dataX(0)[0], 0.0)
        old_id = id(ws)

        mantid.ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='Energy')

        # ConvertUnits is in-place, so ws is still the same
        self.assertEqual(id(ws), old_id)
        # And has new units
        self.assertAlmostEqual(ws.dataX(0)[0], 2.94020841e+00)

    def test_output_as_argument(self):
        ws_in = mantid.CreateSampleWorkspace()
        ws = mantid.CreateSampleWorkspace()
        self.assertNotAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)

        mantid.Rebin(InputWorkspace=ws_in, OutputWorkspace=ws, Params=2)

        self.assertAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)
        self.assertEqual(len(mantid.AnalysisDataService), 0)

    def test_output_as_lhs(self):
        ws_in = mantid.CreateSampleWorkspace()
        ws = mantid.CreateSampleWorkspace()
        self.assertNotAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)

        ws = mantid.Rebin(InputWorkspace=ws_in, Params=2)

        self.assertAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)
        self.assertEqual(len(mantid.AnalysisDataService), 0)

    def test_output_as_lhs_and_argument(self):
        ws_in = mantid.CreateSampleWorkspace()
        ws = mantid.CreateSampleWorkspace()
        self.assertNotAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)

        # Probably this should not be used, but Mantid supports it.
        ws2 = mantid.Rebin(InputWorkspace=ws_in, OutputWorkspace=ws, Params=2)

        self.assertAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)
        self.assertAlmostEqual(ws2.dataX(0)[1]-ws2.dataX(0)[0], 2.0)
        self.assertEqual(len(mantid.AnalysisDataService), 0)

    def test_in_equal_out(self):
        ws = mantid.CreateSampleWorkspace()
        self.assertNotAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)

        mantid.Rebin(InputWorkspace=ws, OutputWorkspace=ws, Params=2)

        self.assertAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)
        self.assertEqual(len(mantid.AnalysisDataService), 0)

    def test_scope(self):
        def scope():
            # Creating workspace with same variable name
            ws = mantid.CreateSampleWorkspace()
            # Make sure it has not the same bin size
            self.assertNotAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)
        ws_in = mantid.CreateSampleWorkspace()
        ws = mantid.Rebin(InputWorkspace=ws_in, Params=2)
        self.assertAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)
        scope()
        # After calling the function, the original ws still exists and has
        # its original bin size, as it should.
        self.assertAlmostEqual(ws.dataX(0)[1]-ws.dataX(0)[0], 2.0)
        self.assertEqual(len(mantid.AnalysisDataService), 0)

    def test_python_container(self):
        workspaces = []
        for i in range(2):
            ws = mantid.CreateSampleWorkspace()
            ws = mantid.Rebin(InputWorkspace=ws, Params=1.0+i)
            workspaces.append(ws)

        for i in range(2):
            # All workspaces still valid and in the state they were created in
            self.assertAlmostEqual(workspaces[i].dataX(0)[1]-workspaces[i].dataX(0)[0], 1.0+i)
        self.assertEqual(len(mantid.AnalysisDataService), 0)


if __name__ == '__main__':
    unittest.main()
