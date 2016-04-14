import unittest
import platform
import numpy as np
from mantid.simpleapi import VesuvioThickness
from mantid.api import ITableWorkspace


class VesuvioThickness(unittest.TestCase):

    # Original test values from fortran routines
    _masses = "1.0079,27.0,91.0"
    _amplitudes = "0.9301589,2.9496644e-02,4.0345035e-02"
    _trans_guess = 241
    _thickness = 5.0
    _number_density = 1.0

#----------------------------------Algorithm tests----------------------------------------

    def test_basic_input(self):
        dens_table, trans_table = VesuvioThickness(Masses=self._masses,
                                                   Amplitudes=self._amplitudes,
                                                   TransmissionGuess=self._trans_guess,
                                                   Thickness=self._thickness,
                                                   NumbereDensity=self._number_density)
        # Validate shape
        self._validate_shape(dens_table)
        self._validate_shape(trans_table)
        self.assertEqual(dens_table.cell(0,1), "first value")
        self.assertEqual(dens_table.cell(9,1), "last value")
        self.assertEqual(trans_table.cell(0,1), "first value")
        self.assertEqual(trans_table.cell(9,1), "last value")


#--------------------------------Validate results------------------------------------------------
    def _validate_shape(self, table_ws):
        self.assertTrue(isinstance(table_ws, ITableWorkspace))
        self.assertEqual(table_ws.columnCount(), 2)
        self.assertEqual(table_ws.rowCount(), 10)
        self.assertEqual(table_ws.cell(0,0), str(1))
        self.assertEqual(table_ws.cell(9,0), str(10))


#--------------------------------Helper functions--------------------------------------

if __name__=="__main__":
    unittest.main()
