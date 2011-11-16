import unittest

from testhelpers import run_algorithm

from mantid.api import analysis_data_svc, MatrixWorkspace, Workspace

class AnalysisDataServiceTest(unittest.TestCase):
  
    def test_len_returns_correct_value(self):
        self.assertEquals(len(analysis_data_svc), 0)
        
    def test_retrieval_of_non_existent_data_raises_error(self):
        try:
            analysis_data_svc['NotHere']
            self.fail('AnalysisDataService did not throw when object does not exist')
        except RuntimeError:
            pass
        
    def test_len_increases_when_item_added(self):
        wsname = 'ADSTest_test_len_increases_when_item_added'
        run_algorithm('Load', Filename='LOQ48127.raw', OutputWorkspace=wsname, SpectrumMax=1)    
        self.assertEquals(len(analysis_data_svc), 1)
        # Remove to clean the test up
        analysis_data_svc.remove(wsname)
        
    def test_len_decreases_when_item_removed(self):
        wsname = 'ADSTest_test_len_decreases_when_item_removed'
        run_algorithm('Load', Filename='LOQ48127.raw', OutputWorkspace=wsname, SpectrumMax=1)    
        self.assertEquals(len(analysis_data_svc), 1)
        # Remove to clean the test up
        del analysis_data_svc[wsname]
        self.assertEquals(len(analysis_data_svc), 0)
    
    def test_key_operator_does_same_as_retrieve(self):
        wsname = 'ADSTest_test_key_operator_does_same_as_retrieve'
        run_algorithm('LoadRaw', Filename='LOQ48127.raw', OutputWorkspace=wsname, SpectrumMax=1) 
        ws_from_op = analysis_data_svc[wsname]
        ws_from_method = analysis_data_svc.retrieve(wsname)
        
        self.assertNotEquals(ws_from_op.name(), '')
        self.assertEquals(ws_from_op.name(), ws_from_method.name())
        self.assertEquals(ws_from_op.get_memory_size(), ws_from_method.get_memory_size())

        # Type check
        #self.assertTrue(isinstance(ws_from_op, MatrixWorkspace))
        #self.assertTrue(isinstance(ws_from_method, MatrixWorkspace))

        # Remove to clean the test up
        analysis_data_svc.remove(wsname)

        