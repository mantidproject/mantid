import unittest

from MantidFramework import *
mtd.initialise()
from mantidsimple import LoadRaw

class WorkspaceHistoryTest(unittest.TestCase):
    """Test the API::WorkspaceHistory interface
    """
    def setUp(self):
        pass

    def test_lastAlgorithm_returns_the_correct_object(self):
        # Smallest dataset possible
        loq_1 = LoadRaw('LOQ48127.raw', 'loq_1', SpectrumMin=1, SpectrumMax=1).workspace()
        # Retrive the algorithm
        last_alg = loq_1.getHistory().lastAlgorithm()
        self.assertEquals(last_alg.name(), 'LoadRaw')
        self.assertEquals(last_alg.getPropertyValue('SpectrumMin'), '1')
        self.assertEquals(last_alg.getPropertyValue('SpectrumMax'), '1')
