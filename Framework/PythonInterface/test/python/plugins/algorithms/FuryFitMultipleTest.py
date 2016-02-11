import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup, ITableWorkspace


class FuryFitMultipleTest(unittest.TestCase):

    _iqt_ws = None
    _function = r'name=LinearBackground,A0=0.027668,A1=0,ties=(A1=0);name=UserFunction,Formula=Intensity*exp(-(x/Tau)^Beta),Intensity=0.972332,Tau=0.0247558,Beta=1;ties=(f1.Intensity=1-f0.A0)'

    def setUp(self):
        self._iqt_ws = Load(Filename='iris26176_graphite002_iqt.nxs',
                            OutputWorkspace='iris26176_graphite002_iqt')

    def _validate_result(self, params, result, fit_group):
        self.assertTrue(isinstance(params, ITableWorkspace))
        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertTrue(isinstance(fit_group, WorkspaceGroup))

    def test_basic(self):
        """
        Tests a basic run of FuryfitMultiple.
        """
        FuryFitMultiple(InputWorkspace=self._iqt_ws,
                        Function=self._function,
                        FitType='1S_s',
                        StartX=0,
                        EndX=0.2,
                        SpecMin=0,
                        SpecMax=16,
                        ConstrainIntensities=True,
                        Save=False,
                        Plot='None')
        params = mtd['irs26176_graphite002_fury_1Smult_s0_to_16_Parameters']
        result = mtd['irs26176_graphite002_fury_1Smult_s0_to_16_Result']
        fit_group  = mtd['irs26176_graphite002_fury_1Smult_s0_to_16_Workspaces']
        self._validate_result(params, result, fit_group)

#----------------------------------------Failure cases-------------------------------------

    def test_minimum_spectra_number_less_than_0(self):
        self.assertRaises(RuntimeError, FuryFitMultiple,
                          InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          EndX=0.2,
                          SpecMin=-1,
                          SpecMax=16)

    def test_maximum_spectra_more_than_workspace_spectra(self):
        self.assertRaises(RuntimeError, FuryFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          EndX=0.2,
                          SpecMin=0,
                          SpecMax=20)

    def test_minimum_spectra_more_than_maximum_spectra(self):
        self.assertRaises(RuntimeError, FuryFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          EndX=0.2,
                          SpecMin=10,
                          SpecMax=5)

    def test_minimum_x_less_than_0(self):
        self.assertRaises(RuntimeError, FuryFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          StartX=-0.2,
                          EndX=0.2,
                          SpecMin=0,
                          SpecMax=16)
                
    def test_maximum_x_more_than_workspace_max_x(self):
        self.assertRaises(RuntimeError, FuryFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          StartX=0,
                          EndX=0.4,
                          SpecMin=0,
                          SpecMax=16)                
                
    def test_minimum_spectra_more_than_maximum_spectra(self):
        self.assertRaises(RuntimeError, FuryFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          StartX=0.2,
                          EndX=0.1,
                          SpecMin=0,
                          SpecMax=16)


if __name__=="__main__":
    unittest.main()
