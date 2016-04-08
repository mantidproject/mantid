import unittest
from mantid.simpleapi import *
from mantid.api import MatrixWorkspace, WorkspaceGroup, ITableWorkspace


class IqtFitSequentialTest(unittest.TestCase):


    def setUp(self):


#-----------------------------------Validation of result-------------------------------------

    def _validate_output(self, params, result, fit_group):



    def _validate_table_shape(self, tableWS):


    def _validate_matrix_shape(self, matrixWS):


    def _validate_group_shape(self, groupWS):


    def _validate_table_values(self, tableWS):


    def _validate_matrix_values(self, matrixWS):


    def _validate_group_values(self, groupWS):


#---------------------------------------Success cases--------------------------------------

    def test_basic(self):


#----------------------------------------Failure cases-------------------------------------

    def test_minimum_spectra_number_less_than_0(self):


    def test_maximum_spectra_more_than_workspace_spectra(self):


    def test_minimum_spectra_more_than_maximum_spectra(self):


    def test_minimum_x_less_than_0(self):


    def test_maximum_x_more_than_workspace_max_x(self):


    def test_minimum_spectra_more_than_maximum_spectra(self):



if __name__=="__main__":
    unittest.main()
