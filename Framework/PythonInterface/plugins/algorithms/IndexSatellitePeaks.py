# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from scipy.spatial import KDTree
import fractional_indexing as indexing

from mantid.kernel import Direction, V3D
from mantid.api import (IPeaksWorkspaceProperty, PythonAlgorithm, AlgorithmFactory)
import mantid.simpleapi as api


class IndexSatellitePeaks(PythonAlgorithm):

    def category(self):
        return 'Crystal\\Peaks'

    def seeAlso(self):
        return [ "FindSatellitePeaks" ]

    def name(self):
        return "IndexSatellitePeaks"

    def summary(self):
        return "Algorithm for indexing satellite peaks in superspace"

    def PyInit(self):
        self.declareProperty(IPeaksWorkspaceProperty(name="NuclearPeaks",
                                                     defaultValue="",
                                                     direction=Direction.Input),
                             doc="Main integer HKL peaks. Q vectors will be calculated relative to these peaks.")

        self.declareProperty(IPeaksWorkspaceProperty(name="SatellitePeaks",
                                                     defaultValue="",
                                                     direction=Direction.Input),
                             doc="Positions of satellite peaks with fractional \
                             HKL coordinates")

        self.declareProperty(IPeaksWorkspaceProperty(name="OutputWorkspace",
                                                     defaultValue="",
                                                     direction=Direction.Output),
                             doc="The indexed satellite peaks. This will be a  \
                             table workspace with miller indicies h, k, l, m1, \
                             m2, ..., mn.")

        self.declareProperty('Tolerance', 0.3, direction=Direction.Input,
                             doc="Tolerance on the noise of the q vectors")

        self.declareProperty('NumOfQs', 1, direction=Direction.Input,
                             doc="Number of independant q vectors")

        self.declareProperty('ClusterThreshold', 1.5, direction=Direction.Input,
                             doc="Threshold for automaticallty deciding on the number of q vectors to use. If NumOfQs found is set then this \
                             is property is ignored.")

    def PyExec(self):
        tolerance = self.getProperty("Tolerance").value
        k = int(self.getProperty("NumOfQs").value)
        nuclear = self.getProperty("NuclearPeaks").value
        satellites = self.getProperty("SatellitePeaks").value
        cluster_threshold = self.getProperty("ClusterThreshold").value
        n_trunc_decimals = int(np.ceil(abs(np.log10(tolerance))))

        if nuclear.getNumberPeaks() == 0:
            raise RuntimeError("The NuclearPeaks parameter must have at least one peak")

        if satellites.getNumberPeaks() == 0:
            raise RuntimeError("The SatellitePeaks parameter must have at least one peak")

        nuclear_hkls = indexing.get_hkls(nuclear)
        sats_hkls = indexing.get_hkls(satellites)

        qs = indexing.find_q_vectors(nuclear_hkls, sats_hkls)
        self.log().notice("K value is {}".format(k))

        k = None if k == -1 else k
        clusters, k = indexing.cluster_qs(qs, k=k, threshold=cluster_threshold)

        qs = indexing.average_clusters(qs, clusters)
        qs = indexing.trunc_decimals(qs, n_trunc_decimals)
        qs = indexing.sort_vectors_by_norm(qs)

        self.log().notice("Q vectors are: \n{}".format(qs))

        indices = indexing.index_q_vectors(qs, tolerance)
        ndim = indices.shape[1] + 3

        hkls = indexing.find_nearest_integer_peaks(nuclear_hkls, sats_hkls)

        hklm = np.zeros((hkls.shape[0], ndim))
        hklm[:, :3] = np.round(hkls)

        raw_qs = hkls - sats_hkls
        peak_map = KDTree(qs)
        for i, q in enumerate(raw_qs):
            distance, index = peak_map.query(q, k=1)
            hklm[i, 3:] = indices[index]

        indexed = self.create_indexed_peaksworkspace(satellites, qs, hklm)
        self.setProperty("OutputWorkspace", indexed)

    def create_indexed_peaksworkspace(self, fractional_peaks, qs, hklm):
        """Create a PeaksWorkspace that contains indexed peak data.

        :param fractional_peaks: the peaks workspace containing peaks with
            fractional HKL values.
        :param qs: The set of modulation vectors determined
        :param hklm: the new higher dimensional miller indices to add.
        :returns: a peaks workspace with the indexed peak data
        """
        # pad to 6 columns so we can assume a (hkl) (mnp) layout
        hklm = np.pad(hklm, pad_width=(0, 6 - hklm.shape[1]), mode='constant',
                      constant_values=0)
        indexed = api.CloneWorkspace(fractional_peaks, StoreInADS=False)
        # save modulation vectors. ensure qs has 3 rows
        qs = np.pad(qs, pad_width=((0, 3 - qs.shape[0]), (0, 0)), mode='constant',
                    constant_values=0)
        lattice = fractional_peaks.sample().getOrientedLattice()
        lattice.setModVec1(V3D(*qs[0]))
        lattice.setModVec2(V3D(*qs[1]))
        lattice.setModVec3(V3D(*qs[2]))
        # save indices
        for row, peak in enumerate(indexed):
            row_indices = hklm[row]
            peak.setHKL(*row_indices[:3])
            peak.setIntMNP(V3D(*row_indices[3:]))

        return indexed


AlgorithmFactory.subscribe(IndexSatellitePeaks)
