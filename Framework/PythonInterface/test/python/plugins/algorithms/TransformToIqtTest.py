import unittest
from mantid.simpleapi import *
from mantid.api import *


class TransformToIqtTest(unittest.TestCase):


    def setUp(self):
        """
        Generate reference result param table.
        """

        CreateEmptyTableWorkspace(OutputWorkspace='__TransformToIqtTest_param')
        self._param_table = mtd['__TransformToIqtTest_param']

        self._param_table.addColumn('int', 'SampleInputBins')
        self._param_table.addColumn('float', 'BinReductionFactor')
        self._param_table.addColumn('int', 'SampleOutputBins')
        self._param_table.addColumn('float', 'EnergyMin')
        self._param_table.addColumn('float', 'EnergyMax')
        self._param_table.addColumn('float', 'EnergyWidth')
        self._param_table.addColumn('float', 'Resolution')
        self._param_table.addColumn('int', 'ResolutionBins')

        self._param_table.addRow([1725, 10.0, 172, -0.5, 0.5, 0.00581395, 0.0175, 6])


    def test_with_can_reduction(self):
        """
        Tests running using the container reduction as a resolution.
        """

        sample = Load('irs26176_graphite002_red')
        can = Load('irs26173_graphite002_red')

        params, iqt = TransformToIqt(SampleWorkspace=sample,
                                     ResolutionWorkspace=can,
                                     BinReductionFactor=10)

        self.assertEqual(CheckWorkspacesMatch(params, self._param_table), "Success!")


    def test_with_resolution_reduction(self):
        """
        Tests running using the instrument resolution workspace.
        """

        sample = Load('irs26176_graphite002_red')
        resolution = Load('irs26173_graphite002_res')

        params, iqt = TransformToIqt(SampleWorkspace=sample,
                                     ResolutionWorkspace=resolution,
                                     BinReductionFactor=10)

        self.assertEqual(CheckWorkspacesMatch(params, self._param_table), "Success!")


    def test_cropping_of_data(self):
        """
        Test to see if data is more than 1 in y axis for the first spectra. Any data like this should be cropped
        """

        sample = Load('irs26176_graphite002_red')
        resolution = Load('irs26173_graphite002_res')

        params, iqt = TransformToIqt(SampleWorkspace=sample,
                                     ResolutionWorkspace=resolution,
                                     BinReductionFactor=10)

        iqt_y_data = iqt.dataY(0)
        for bin_index in range(len(iqt_y_data)):
            self.assertTrue(iqt_y_data[bin_index] < 1)


    def test_output_size(self):
        """
        Test to ensure the workspace has not been over-cropped
        """

        sample = Load('irs26176_graphite002_red.nxs')
        resolution = Load('irs26173_graphite002_res.nxs')

        params, iqt = TransformToIqt(SampleWorkspace=sample,
                                     ResolutionWorkspace=resolution,
                                     EnergyMin=-0.5,
                                     EnergyMax=0.5,
                                     BinReductionFactor=10)

        expected_bins = 78 # Expected bin number after cropping for this data set
        self.assertEquals(iqt.blocksize(), expected_bins)

if __name__ == '__main__':
    unittest.main()
