import unittest

from testhelpers import run_algorithm, can_be_instantiated

from mantid.api import IEventWorkspace

class IEventWorkspaceTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            alg = run_algorithm("Load", Filename='CNCS_7860_event.nxs', OutputWorkspace='CNCS_7860',
                                FilterByTimeStart=60.0,FilterByTimeStop=60.5,LoadMonitors=False,
                                child=True)
            _test_ws = alg.get_property("OutputWorkspace").value

    def test_that_it_cannot_be_directly_instantiated(self):
        self.assertFalse(can_be_instantiated(IEventWorkspace))

    def test_meta_information(self):
        self.assertEquals(self._test_ws.get_number_events(), 146)
        self.assertEquals(self._test_ws.get_minimum_tof(), 44704.9)
        self.assertEquals(self._test_ws.get_maximum_tof(), 55612.4)

    def test_that_clearing_mru_does_not_raise_an_error(self):
        try:
            self._test_ws.clear_mru()
            error_raised = False
        except:
            error_raised = True
        self.assertFalse(error_raised)