# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from testhelpers import can_be_instantiated, WorkspaceCreationHelper

from mantid.api import IEventWorkspace, IEventList


class IEventWorkspaceTest(unittest.TestCase):
    _test_ws = None
    _nbins = 10
    _npixels = 5

    def setUp(self):
        if self._test_ws is None:
            self.__class__._test_ws = WorkspaceCreationHelper.createEventWorkspace2(self._npixels, self._nbins)

    def test_that_it_cannot_be_directly_instantiated(self):
        self.assertFalse(can_be_instantiated(IEventWorkspace))

    def test_meta_information(self):
        self.assertEqual(self._test_ws.getNumberEvents(), 1000)
        self.assertAlmostEqual(self._test_ws.getTofMin(), 0.5)
        self.assertAlmostEqual(self._test_ws.getTofMax(), 99.5)

    def test_that_clearing_mru_does_not_raise_an_error(self):
        try:
            self._test_ws.clearMRU()
            error_raised = False
        except:
            error_raised = True
        self.assertFalse(error_raised)

    def test_event_list_is_return_as_correct_type(self):
        el = self._test_ws.getSpectrum(0)
        self.assertTrue(isinstance(el, IEventList))
        self.assertEqual(el.getNumberEvents(), 200)

    def test_event_list_getWeights(self):
        el = self._test_ws.getSpectrum(0)
        self.assertTrue(isinstance(el, IEventList))
        TofList = el.getTofs()
        self.assertEqual(len(TofList), el.getNumberEvents())  # check length
        self.assertAlmostEqual(TofList[0], 0.5)  # first value

    def test_event_list_getWeights(self):
        el = self._test_ws.getSpectrum(0)
        self.assertTrue(isinstance(el, IEventList))
        weightList = el.getWeights()
        self.assertEqual(len(weightList), el.getNumberEvents())  # check length
        self.assertAlmostEqual(weightList[0], 1.0)  # first value
        self.assertAlmostEqual(weightList[len(weightList) - 1], 1.0)  # last value

    def test_event_list_getWeightErrors(self):
        el = self._test_ws.getSpectrum(0)
        self.assertTrue(isinstance(el, IEventList))
        weightErrorList = el.getWeightErrors()
        self.assertEqual(len(weightErrorList), el.getNumberEvents())  # check length
        self.assertAlmostEqual(weightErrorList[0], 1.0)  # first value
        self.assertAlmostEqual(weightErrorList[len(weightErrorList) - 1], 1.0)  # last value

    def test_deprecated_getEventList(self):
        el = self._test_ws.getEventList(0)
        self.assertTrue(isinstance(el, IEventList))
        self.assertEqual(el.getNumberEvents(), 200)


if __name__ == "__main__":
    unittest.main()
