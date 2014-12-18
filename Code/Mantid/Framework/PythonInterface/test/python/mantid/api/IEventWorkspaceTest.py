import unittest

from testhelpers import run_algorithm, can_be_instantiated, WorkspaceCreationHelper

from mantid.api import (IEventWorkspace, IEventList, IWorkspaceProperty,
                        AlgorithmManager)

class IEventWorkspaceTest(unittest.TestCase):

    _test_ws = None
    _nbins = 10
    _npixels = 5

    def setUp(self):
        if self._test_ws is None:
            self.__class__._test_ws = \
              WorkspaceCreationHelper.CreateEventWorkspace2(self._npixels, self._nbins)

    def test_that_it_cannot_be_directly_instantiated(self):
        self.assertFalse(can_be_instantiated(IEventWorkspace))

    def test_meta_information(self):
        self.assertEquals(self._test_ws.getNumberEvents(), 1000)
        self.assertAlmostEquals(self._test_ws.getTofMin(), 0.5)
        self.assertAlmostEquals(self._test_ws.getTofMax(), 99.5)

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
        self.assertEquals(el.getNumberEvents(), 200)

    def test_event_list_getWeights(self):
        el = self._test_ws.getEventList(0)
        self.assertTrue(isinstance(el, IEventList))
        TofList = el.getTofs()
        self.assertEquals(len(TofList), el.getNumberEvents()) #check length
        self.assertAlmostEquals(TofList[0], 0.5) #first value

    def test_event_list_getWeights(self):
        el = self._test_ws.getEventList(0)
        self.assertTrue(isinstance(el, IEventList))
        weightList = el.getWeights()
        self.assertEquals(len(weightList), el.getNumberEvents()) #check length
        self.assertAlmostEquals(weightList[0], 1.0) #first value
        self.assertAlmostEquals(weightList[len(weightList)-1], 1.0) #last value

    def test_event_list_getWeightErrors(self):
        el = self._test_ws.getEventList(0)
        self.assertTrue(isinstance(el, IEventList))
        weightErrorList = el.getWeightErrors()
        self.assertEquals(len(weightErrorList), el.getNumberEvents()) #check length
        self.assertAlmostEquals(weightErrorList[0], 1.0) #first value
        self.assertAlmostEquals(weightErrorList[len(weightErrorList)-1], 1.0) #last value


if __name__ == '__main__':
    unittest.main()
