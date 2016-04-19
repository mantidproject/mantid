import unittest
import platform
import numpy as np
from mantid.simpleapi import VesuvioThickness
from mantid.api import ITableWorkspace


class VesuvioThicknessTest(unittest.TestCase):

    # Original test values from fortran routines
    _masses = "1.0079,27.0,91.0"
    _amplitudes = "0.9301589,2.9496644e-02,4.0345035e-02"
    _trans_guess = 0.831
    _thickness = 5.0
    _number_density = 1.0

#----------------------------------Algorithm tests----------------------------------------

    def test_basic_input(self):
        dens_table, trans_table = VesuvioThickness(Masses=self._masses,
                                                   Amplitudes=self._amplitudes,
                                                   TransmissionGuess=self._trans_guess,
                                                   Thickness=self._thickness,
                                                   NumberDensity=self._number_density)
        # Validate shape
        self._validate_shape(dens_table)
        self._validate_shape(trans_table)
        self.assertAlmostEqual(dens_table.cell(0,1), 22.4062053)
        self.assertAlmostEqual(dens_table.cell(9,1), 24.4514601)
        self.assertAlmostEqual(trans_table.cell(0,1), 0.99245745)
        self.assertAlmostEqual(trans_table.cell(9,1), 0.83100000)


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
