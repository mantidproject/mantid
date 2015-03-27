import unittest
from mantid.simpleapi import *
from mantid.api import *

class TransformToIqtTest(unittest.TestCase):

    def test_with_can_reduction(self):
        """
        Tests running using the container reduction as a resolution.
        """

        sample = Load('irs26176_graphite002_red')
        can = Load('irs26173_graphite002_red')

        params, iqt = TransformToIqt(SampleWorkspace=sample,
                                     ResolutionWorkspace=can,
                                     BinReductionFactor=10)


    def test_with_resolution_reduction(self):
        """
        Tests running using the instrument resolution workspace.
        """

        sample = Load('irs26176_graphite002_red')
        resolution = Load('irs26173_graphite002_res')

        params, iqt = TransformToIqt(SampleWorkspace=sample,
                                     ResolutionWorkspace=resolution,
                                     BinReductionFactor=10)


if __name__ == '__main__':
    unittest.main()
