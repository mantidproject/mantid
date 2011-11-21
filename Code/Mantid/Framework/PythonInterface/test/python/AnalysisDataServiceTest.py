import unittest

from testhelpers import run_algorithm

from mantid.api import analysis_data_svc, MatrixWorkspace, Workspace

class AnalysisDataServiceTest(unittest.TestCase):
  
    def test_len_returns_correct_value(self):
        self.assertEquals(len(analysis_data_svc), 0)
        
    def test_retrieval_of_non_existent_data_raises_KeyError(self):
        try:
            analysis_data_svc['NotHere']
            self.fail('AnalysisDataService did not throw when object does not exist')
        except KeyError:
            pass

    def _run_createws(self, wsname):
        """
            Run create workspace storing the output in the named workspace
        """
        data = [1.0,2.0,3.0]
        run_algorithm('CreateWorkspace',OutputWorkspace=wsname,DataX=data,DataY=data,NSpec=1,UnitX='Wavelength')
        
        
    def test_len_increases_when_item_added(self):
        wsname = 'ADSTest_test_len_increases_when_item_added'
        self._run_createws(wsname)
        self.assertEquals(len(analysis_data_svc), 1)
        # Remove to clean the test up
        analysis_data_svc.remove(wsname)
        
    def test_len_decreases_when_item_removed(self):
        wsname = 'ADSTest_test_len_decreases_when_item_removed'
        self._run_createws(wsname)
        self.assertEquals(len(analysis_data_svc), 1)
        # Remove to clean the test up
        del analysis_data_svc[wsname]
        self.assertEquals(len(analysis_data_svc), 0)
    
    def test_key_operator_does_same_as_retrieve(self):
        wsname = 'ADSTest_test_key_operator_does_same_as_retrieve'
        self._run_createws(wsname)
        ws_from_op = analysis_data_svc[wsname]
        ws_from_method = analysis_data_svc.retrieve(wsname)
        
        # Type check
        self.assertTrue(isinstance(ws_from_op, MatrixWorkspace))
        self.assertTrue(isinstance(ws_from_method, MatrixWorkspace))

        self.assertNotEquals(ws_from_op.name(), '')
        self.assertEquals(ws_from_op.name(), ws_from_method.name())
        self.assertEquals(ws_from_op.get_memory_size(), ws_from_method.get_memory_size())

        # Remove to clean the test up
        analysis_data_svc.remove(wsname)

    def test_removing_item_invalidates_extracted_handles(self):
        # If a reference to a DataItem has been extracted from the ADS
        # and it is then removed. The extracted handle should no longer
        # be able to access the DataItem
        wsname = 'ADSTest_test_removing_item_invalidates_extracted_handles'
        self._run_createws(wsname)
        ws_handle = analysis_data_svc[wsname]
        succeeded = False
        try:
            ws_handle.id() # Should be okay
            succeeded = True
        except RuntimeError:
            pass
        self.assertTrue(succeeded, "DataItem handle should be valid and allow function calls")
        analysis_data_svc.remove(wsname)
        self.assertRaises(RuntimeError, ws_handle.id)
        