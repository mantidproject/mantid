import unittest
from testhelpers import run_algorithm
from mantid.kernel import std_vector_str

class ITableWorkspaceTest(unittest.TestCase):
  
    _test_ws = None
      
    def setUp(self):
        if self._test_ws is None:
            alg = run_algorithm('RawFileInfo', Filename='LOQ48127.raw',GetRunParameters=True, child=True)
            self.__class__._test_ws = alg.getProperty('RunParameterTable').value
  
    def test_meta_information_is_correct(self):
        self.assertEquals(self._test_ws.columnCount(), 19)
        self.assertEquals(self._test_ws.rowCount(), 1)
        
        column_names = self._test_ws.getColumnNames()
        self.assertEquals(len(column_names), 19)
        self.assertEquals(type(column_names), std_vector_str)
        
    def test_cell_access_returns_variables_as_native_python_types(self):
        self.assertAlmostEquals(self._test_ws.cell('r_gd_prtn_chrg',0), 10.040912628173828, 15)
        self.assertAlmostEquals(self._test_ws.cell(0, 7), 10.040912628173828, 15)
        
        self.assertEquals(self._test_ws.cell('r_goodfrm', 0), 9229)
        self.assertEquals(self._test_ws.cell(0, 9), 9229)
        
        self.assertEquals(self._test_ws.cell('r_enddate', 0), "18-DEC-2008")
        self.assertEquals(self._test_ws.cell(0, 16), "18-DEC-2008")
        
if __name__ == '__main__':
    unittest.main()
