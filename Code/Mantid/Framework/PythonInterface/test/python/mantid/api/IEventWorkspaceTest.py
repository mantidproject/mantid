import unittest

from testhelpers import run_algorithm, can_be_instantiated

from mantid.api import (IEventWorkspace, IEventList, IWorkspaceProperty,
                        AlgorithmManager)

class IEventWorkspaceTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            alg = run_algorithm("Load", Filename='CNCS_7860_event.nxs', OutputWorkspace='CNCS_7860',
                                FilterByTimeStart=60.0,FilterByTimeStop=60.5,LoadMonitors=False,child=True)
            self.__class__._test_ws = alg.getProperty("OutputWorkspace").value

    def test_that_it_cannot_be_directly_instantiated(self):
        self.assertFalse(can_be_instantiated(IEventWorkspace))

    def test_meta_information(self):
        self.assertEquals(self._test_ws.getNumberEvents(), 164)
        self.assertAlmostEquals(self._test_ws.getTofMin(), 44704.8984375)
        self.assertAlmostEquals(self._test_ws.getTofMax(), 55612.3984375)

    def test_that_clearing_mru_does_not_raise_an_error(self):
        try:
            self._test_ws.clearMRU()
            error_raised = False
        except:
            error_raised = True
        self.assertFalse(error_raised)
        
    def test_event_list_is_return_as_correct_type(self):
        el = self._test_ws.getEventList(0)
        self.assertTrue(isinstance(el, IEventList))
        self.assertEquals(el.getNumberEvents(), 0)
    
if __name__ == '__main__':
    unittest.main()
