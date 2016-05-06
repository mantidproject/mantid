import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup, ITableWorkspace


class IqtFitMultipleTest(unittest.TestCase):

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

        self._validate_table_values(params)
        self._validate_matrix_values(result)
        self._validate_group_values(fit_group)


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


    def _validate_table_values(self, tableWS):
        # Check column data
        column = tableWS.column(0)
        self.assertEquals(round(column[0], 6), 0.483619)
        self.assertEquals(round(column[1], 6), 0.607871)
        self.assertEquals(round(column[-1], 5), 1.84519)

        # Check row data
        row = tableWS.row(0)
        self.assertEquals(round(row['axis-1'], 6),  0.483619)
        self.assertEquals(round(row['f1.Intensity'], 6), 0.979517)
        self.assertEquals(round(row['f1.Tau'], 6), 0.024672)

    def _validate_matrix_values(self, matrixWS):
        # Check f0.A0
        a0 = matrixWS.readY(0)
        self.assertEquals(round(a0[0], 7), 0.0204827)
        self.assertEquals(round(a0[-1],7), 0.0229125)

        # Check f1.Intensity
        intensity = matrixWS.readY(1)
        self.assertEquals(round(intensity[0], 6), 0.979517)
        self.assertEquals(round(intensity[-1],6), 0.977088)

        # Check f1.Tau
        tau = matrixWS.readY(2)
        self.assertEquals(round(tau[0], 6), 0.024672)
        self.assertEquals(round(tau[-1],8), 0.00253487)

        # Check f1.Beta
        beta = matrixWS.readY(3)
        self.assertEquals(round(beta[0], 6), 0.781177)
        self.assertEquals(round(beta[-1],6), 0.781177)

    def _validate_group_values(self, groupWS):
        sub_ws = groupWS.getItem(0)
        # Check Data
        data = sub_ws.readY(0)
        self.assertEquals(round(data[0], 5), 1)
        self.assertEquals(round(data[-1],7),0.0450769)
        # Check Calc
        calc = sub_ws.readY(1)
        self.assertEquals(round(calc[0], 5), 1)
        self.assertEquals(round(calc[-1],6),0.026465)
        # Check Diff
        diff = sub_ws.readY(2)
        self.assertEquals(round(diff[0], 19), -5.31797e-14)
        self.assertEquals(round(diff[-1],6), 0.018612)

#---------------------------------------Success cases--------------------------------------

    def test_basic(self):
        """
        Tests a basic run of IqtfitMultiple.
        """
        result, params, fit_group = IqtFitMultiple(InputWorkspace=self._iqt_ws,
                                                    Function=self._function,
                                                    FitType='1S_s',
                                                    StartX=0,
                                                    EndX=0.2,
                                                    SpecMin=0,
                                                    SpecMax=16,
                                                    ConstrainIntensities=True)
        self._validate_output(params, result, fit_group)

#----------------------------------------Failure cases-------------------------------------

    def test_minimum_spectra_number_less_than_0(self):
        self.assertRaises(ValueError, IqtFitMultiple,
                          InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          EndX=0.2,
                          SpecMin=-1,
                          SpecMax=16,
                          OutputResultWorkspace='result',
                          OutputParameterWorkspace='table',
                          OutputWorkspaceGroup='fit_group')

    def test_maximum_spectra_more_than_workspace_spectra(self):
        self.assertRaises(RuntimeError, IqtFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          EndX=0.2,
                          SpecMin=0,
                          SpecMax=20,
                          OutputResultWorkspace='result',
                          OutputParameterWorkspace='table',
                          OutputWorkspaceGroup='fit_group')

    def test_minimum_spectra_more_than_maximum_spectra(self):
        self.assertRaises(RuntimeError, IqtFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          EndX=0.2,
                          SpecMin=10,
                          SpecMax=5,
                          OutputResultWorkspace='result',
                          OutputParameterWorkspace='table',
                          OutputWorkspaceGroup='fit_group')

    def test_minimum_x_less_than_0(self):
        self.assertRaises(ValueError, IqtFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          StartX=-0.2,
                          EndX=0.2,
                          SpecMin=0,
                          SpecMax=16,
                          OutputResultWorkspace='result',
                          OutputParameterWorkspace='table',
                          OutputWorkspaceGroup='fit_group')

    def test_maximum_x_more_than_workspace_max_x(self):
        self.assertRaises(RuntimeError, IqtFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          StartX=0,
                          EndX=0.4,
                          SpecMin=0,
                          SpecMax=16,
                          OutputResultWorkspace='result',
                          OutputParameterWorkspace='table',
                          OutputWorkspaceGroup='fit_group')

    def test_minimum_spectra_more_than_maximum_spectra(self):
        self.assertRaises(RuntimeError, IqtFitMultiple, InputWorkspace=self._iqt_ws,
                          Function=self._function,
                          FitType='1S_s',
                          StartX=0.2,
                          EndX=0.1,
                          SpecMin=0,
                          SpecMax=16,
                          OutputResultWorkspace='result',
                          OutputParameterWorkspace='table',
                          OutputWorkspaceGroup='fit_group')


if __name__=="__main__":
    unittest.main()
