import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup, ITableWorkspace


class FuryFitMultipleTest(unittest.TestCase):

    _iqt_ws = None
    _function = r'name=LinearBackground,A0=0.027668,A1=0,ties=(A1=0);name=UserFunction,Formula=Intensity*exp(-(x/Tau)^Beta),Intensity=0.972332,Tau=0.0247558,Beta=1;ties=(f1.Intensity=1-f0.A0)'

    def setUp(self):
        self._iqt_ws = Load(Filename='iris26176_graphite002_iqt.nxs',
                            OutputWorkspace='iris26176_graphite002_iqt')

#-----------------------------------Validation of result-------------------------------------

    def _validate_output(self, params, result, fit_group):
        self.assertTrue(isinstance(params, ITableWorkspace))
        self.assertTrue(isinstance(result, MatrixWorkspace))
        self.assertTrue(isinstance(fit_group, WorkspaceGroup))

        self._validate_table_shape(params)
        self._validate_matrix_shape(result)
        self._validate_group_shape(fit_group)


    def _validate_table_shape(self, tableWS):
        # Check length of rows and columns
        rows = tableWS.rowCount()
        columns = tableWS.columnCount()
        self.assertEquals(rows, 17)
        self.assertEquals(columns, 11)

        # Check some column names
        column_names = tableWS.getColumnNames()
        self.assertEquals('axis-1', column_names[0])
        self.assertEquals('f0.A0', column_names[1])
        self.assertEquals('f0.A0_Err', column_names[2])

    def _validate_matrix_shape(self, matrixWS):
        # Check no. bins and no. hists
        nbins = matrixWS.blocksize()
        nhists = matrixWS.getNumberHistograms()
        self.assertEquals(nbins, 17)
        self.assertEquals(nhists, 4)

        # Check histogram names
        text_axis = matrixWS.getAxis(1)
        self.assertTrue(text_axis.isText())
        self.assertEquals('f0.A0',text_axis.label(0))
        self.assertEquals('f1.Intensity',text_axis.label(1))
        self.assertEquals('f1.Tau',text_axis.label(2))
        self.assertEquals('f1.Beta',text_axis.label(3))

        # Check bin units
        self.assertEquals('MomentumTransfer', matrixWS.getAxis(0).getUnit().unitID())

    def _validate_group_shape(self, groupWS):
        # Check number of workspaces and size
        nitems = groupWS.getNumberOfEntries()
        self.assertEquals(nitems, 17)
        sub_ws = groupWS.getItem(0)
        nbins = sub_ws.blocksize()
        nhists = sub_ws.getNumberHistograms()
        self.assertEquals(nbins, 49)
        self.assertEquals(nhists, 3)

        # Check histogram names
        text_axis = sub_ws.getAxis(1)
        self.assertTrue(text_axis.isText())
        self.assertEquals('Data',text_axis.label(0))
        self.assertEquals('Calc',text_axis.label(1))
        self.assertEquals('Diff',text_axis.label(2))

        # Check bin units
        self.assertEquals('ns', str(sub_ws.getAxis(0).getUnit().symbol()))

#---------------------------------------Success cases--------------------------------------

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
        self._validate_output(params, result, fit_group)

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
