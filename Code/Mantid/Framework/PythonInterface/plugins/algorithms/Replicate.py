#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *
import numpy as np

class Replicate(PythonAlgorithm):
    """
    Replicate a n dimensional workspace to make it an n+1 dimensional workspace
    """

    def category(self):
        return "MDAlgorithms"

    def name(self):
        return "Replicate"

    def summary(self):
        return "This algorithm is used to replicate a cut of n dimensions to n+1 dimensions."

    def PyInit(self):
        self.declareProperty(IMDHistoWorkspaceProperty("InputWorkspace", "", Direction.Input),
                             "Workspace to replicate to n+1 dimensions. Contains data to be replicated.")
        self.declareProperty(IMDHistoWorkspaceProperty("ShapeWorkspace", "", Direction.Input),
                             "Template to define shape of output. OutputWorkspace will have the same characteristics but hold different data.")
        self.declareProperty(IMDHistoWorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             "Output replicated workspace")

    def _tile_flattened(self, source, n_replications):
        source = source.flatten()
        return np.tile(source, n_replications)

    def _replicate(self, source, shape):

        if shape.getNumDims()-1 != source.getNumDims():
            raise ValueError("ShapeWorkspace input should have n+1 dimensions more than the InputWorkspace")

        last_dim = shape.getDimension(shape.getNumDims() - 1)
        n_replications = last_dim.getNBins()
        e = self._tile_flattened(np.sqrt(source.getErrorSquaredArray()), n_replications)
        s = self._tile_flattened(source.getSignalArray(), n_replications)
        n = self._tile_flattened(source.getNumEventsArray(), n_replications)
        n_bins = list()
        extents = list()
        names = list()
        units = list()
        for i in range(shape.getNumDims()):
            shape_dimension = shape.getDimension(i)
            n_bins.append(shape_dimension.getNBins())
            extents.append(shape_dimension.getMinimum())
            extents.append(shape_dimension.getMaximum())
            names.append(shape_dimension.getName())
            units.append(shape_dimension.getUnits())
            if i < source.getNumDims():
                source_dimension = source.getDimension(i)
                if source_dimension.getNBins() != shape_dimension.getNBins():
                    raise ValueError("The number of bins for dimension index %i do not match on template and slice" % i)
                if source_dimension.getMinimum() != shape_dimension.getMinimum():
                    raise ValueError("The range minimum for dimension index %i do not match on template and slice" % i)
                if source_dimension.getMaximum() != shape_dimension.getMaximum():
                    raise ValueError("The range maximum for dimension index %i do not match on template and slice" % i)


        alg = AlgorithmManager.create("CreateMDHistoWorkspace")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("SignalInput", s)
        alg.setProperty("ErrorInput", e)
        alg.setProperty("NumberOfEvents", n)
        alg.setProperty("Dimensionality", source.getNumDims() + 1)
        alg.setProperty("Extents", extents)
        alg.setProperty("NumberOfBins", n_bins)
        alg.setProperty("Names", names)
        alg.setProperty("Units", units)
        alg.setProperty("OutputWorkspace", "dummy")
        alg.execute()
        return alg.getProperty("OutputWorkspace").value


    #pylint: disable=too-many-branches
    def PyExec(self):
        source = self.getProperty("InputWorkspace").value
        shape = self.getProperty("ShapeWorkspace").value
        out_ws = self._replicate(source=source, shape=shape)
        self.setProperty("OutputWorkspace", out_ws)


AlgorithmFactory.subscribe(Replicate)

