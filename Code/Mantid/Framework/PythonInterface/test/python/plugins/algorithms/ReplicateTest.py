import unittest
import numpy as np
from mantid.simpleapi import *
from mantid.api import AlgorithmManager, IMDHistoWorkspace, IMDEventWorkspace


class CreateMDTest(unittest.TestCase):
    def _create_with_shape(self, shape, dim_extents=[-10, 10], dim_names=["a", "b", "c", "d", "e", "f", "g"],
                           dim_units="U", signal_value=1):
        n_dims = len(shape)
        flat_length = reduce(lambda x, y: x * y, shape)
        signal = np.resize(np.array([signal_value] * flat_length), shape).flatten()
        error = np.resize(np.array([1] * flat_length), shape).flatten()
        n_events = np.resize(np.array([1] * flat_length), shape).flatten()

        alg = AlgorithmManager.create("CreateMDHistoWorkspace")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("SignalInput", signal)
        alg.setProperty("ErrorInput", error)
        alg.setProperty("NumberOfEvents", n_events)
        alg.setProperty("Dimensionality", n_dims)
        alg.setProperty("Extents", np.tile(dim_extents, n_dims))
        alg.setProperty("NumberOfBins", list(shape))
        alg.setProperty("Names", dim_names[:n_dims])
        alg.setProperty("Units", [dim_units] * n_dims)
        alg.setProperty("OutputWorkspace", "dummy")
        alg.execute()
        return alg.getProperty("OutputWorkspace").value


    def test_init(self):
        alg = AlgorithmManager.create("Replicate")
        alg.initialize()

    def test_non_matching_shape_different_nbins(self):
        alg = AlgorithmManager.create("Replicate")
        alg.setChild(True)
        alg.setRethrows(True)
        alg.initialize()
        shape_ws = self._create_with_shape((1, 1, 1))
        data_ws = self._create_with_shape(
            (1, 2))  # data workspace has the right number of dimensions, but different bin sizes in one of them

        alg.setProperty("InputWorkspace", data_ws)
        alg.setProperty("ShapeWorkspace", shape_ws)
        alg.setProperty("OutputWorkspace", "dummy")
        self.assertRaises(RuntimeError, alg.execute)  # Should fail to execute given the difference in dimension sizes

    def test_non_matching_shape_different_ndims(self):
        alg = AlgorithmManager.create("Replicate")
        alg.setChild(True)
        alg.setRethrows(True)
        alg.initialize()
        shape_ws = self._create_with_shape((1, 1, 1))
        data_ws = self._create_with_shape(
            (1, 1, 1))  # data workspace is expected to have exactly n-1 dimensions of the shape workspace

        alg.setProperty("InputWorkspace", data_ws)
        alg.setProperty("ShapeWorkspace", shape_ws)
        alg.setProperty("OutputWorkspace", "dummy")
        self.assertRaises(RuntimeError, alg.execute)  # Should fail to execute given too few dimensions

    def test_non_matching_shape_different_dimension_max(self):
        alg = AlgorithmManager.create("Replicate")
        alg.setChild(True)
        alg.setRethrows(True)
        alg.initialize()
        shape_ws = self._create_with_shape((1, 1, 1), dim_extents=[-10,10])
        data_ws = self._create_with_shape(
            (1, 1), dim_extents=[-10, 5])  # Max extents for dim will be lower than in the shape workspace

        alg.setProperty("InputWorkspace", data_ws)
        alg.setProperty("ShapeWorkspace", shape_ws)
        alg.setProperty("OutputWorkspace", "dummy")
        self.assertRaises(RuntimeError, alg.execute)  # Should fail to execute

    def test_non_matching_shape_different_dimension_min(self):
        alg = AlgorithmManager.create("Replicate")
        alg.setChild(True)
        alg.setRethrows(True)
        alg.initialize()
        shape_ws = self._create_with_shape((1, 1, 1), dim_extents=[-10,10])
        data_ws = self._create_with_shape(
            (1, 1), dim_extents=[-5, 10])  # Min extents for dim will be higher than in the shape workspace

        alg.setProperty("InputWorkspace", data_ws)
        alg.setProperty("ShapeWorkspace", shape_ws)
        alg.setProperty("OutputWorkspace", "dummy")
        self.assertRaises(RuntimeError, alg.execute)  # Should fail to execute

    def test_execute(self):
        alg = AlgorithmManager.create("Replicate")
        alg.setChild(True)
        alg.setRethrows(True)
        alg.initialize()
        shape_ws = self._create_with_shape((10, 10, 5), dim_extents=[-10,10], signal_value=1)
        data_ws = self._create_with_shape(
            (10, 10), signal_value=4)  # Min extents for dim will be higher than in the shape workspace

        alg.setProperty("InputWorkspace", data_ws)
        alg.setProperty("ShapeWorkspace", shape_ws)
        alg.setProperty("OutputWorkspace", "dummy")
        alg.execute()

        out_ws = alg.getProperty("OutputWorkspace").value

        # Quick sanity tests
        self.assertEquals(out_ws.getNumDims(), 3)
        self.assertEquals(4.00, out_ws.signalAt(0))

        for i in range(out_ws.getNumDims()):
            dim_out = out_ws.getDimension(i)
            dim_in = shape_ws.getDimension(i)
            self.assertEquals(dim_in.getMaximum(), dim_out.getMaximum())
            self.assertEquals(dim_in.getMinimum(), dim_out.getMinimum())
            self.assertEquals(dim_in.getNBins(), dim_out.getNBins())
            self.assertEquals(dim_in.getName(), dim_out.getName())
            self.assertEquals(dim_in.getUnits(), dim_out.getUnits())


if __name__ == '__main__':
    unittest.main()