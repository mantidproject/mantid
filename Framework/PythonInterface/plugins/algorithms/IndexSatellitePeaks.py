import numpy as np
import fractional_indexing as indexing
from mantid.kernel import Direction
from mantid.api import (IPeaksWorkspaceProperty,
                        ITableWorkspaceProperty, PythonAlgorithm, AlgorithmFactory)
from mantid.simpleapi import CreateEmptyTableWorkspace


class IndexSatellitePeaks(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty(IPeaksWorkspaceProperty(name="SatellitePeaks",
                                                     defaultValue="",
                                                     direction=Direction.Input),
                             doc="Positions of satellite peaks with fractional \
                             HKL coordinates")

        self.declareProperty(ITableWorkspaceProperty(name="OutputWorkspace",
                                                     defaultValue="",
                                                     direction=Direction.Output),
                             doc="The indexed satellite peaks. This will be a  \
                             table workspace with miller indicies h, k, l, m1, \
                             m2, ..., mn.")

        self.declareProperty('Tolerance', 0.3, direction=Direction.Input,
                             doc="Tolerance on the noise of the q vectors")
        self.declareProperty('NumOfQs', 1, direction=Direction.Input,
                             doc="Number of independant q vectors")

    def PyExec(self):
        tolerance = self.getProperty("Tolerance").value
        k = self.getProperty("NumOfQs").value
        satellites = self.getProperty("SatellitePeaks").value
        n_trunc_decimals = int(np.ceil(abs(np.log10(tolerance))))

        hkls = indexing.get_hkls(satellites)
        qs = np.round(hkls) - hkls

        clusters, k = indexing.cluster_qs(qs, k=k)

        qs = indexing.average_clusters(qs, clusters)
        qs = indexing.trunc_decimals(qs, n_trunc_decimals)
        qs = indexing.sort_vectors_by_norm(qs)

        self.log().notice("Q vectors are: \n{}".format(qs))

        indicies = indexing.index_q_vectors(qs, tolerance)
        ndim = indicies.shape[1] + 3

        hklm = np.zeros((hkls.shape[0], ndim))
        hklm[:, :3] = np.round(hkls)

        for i, index in enumerate(set(clusters)):
            view = hklm[clusters == index]
            view[:, 3:] = indicies[i]
            hklm[clusters == index] = view

        indexed = self.create_indexed_workspace(satellites, ndim, hklm)
        self.setProperty("OutputWorkspace", indexed)

    def create_indexed_workspace(self, fractional_peaks, ndim, hklm):
        """Create a TableWorkepace that contains indexed peak data.

        This produces a TableWorkepace that looks like a PeaksWorkspace but
        with the additional index columns included. In future releases support
        for indexing should be added to the PeaksWorkspace data type itself.

        :param fractional_peaks: the peaks workspace containing peaks with
            fractional HKL values.
        :param ndim: the number of additional indexing columns to add.
        :param hklm: the new higher dimensional miller indicies to add.
        :returns: a table workspace with the indexed peak data
        """
        # Create table with the number of columns we need
        types = ['int', 'long64', 'double', 'double', 'double', 'double',  'double', 'double',
                 'double', 'double', 'double', 'float', 'str', 'float', 'float', 'V3D', 'V3D']
        indexed = CreateEmptyTableWorkspace()
        names = fractional_peaks.getColumnNames()

        # Insert the extra columns for the addtional indicies
        for i in range(ndim - 3):
            names.insert(5 + i, 'm{}'.format(i + 1))
            types.insert(5 + i, 'double')

        names = np.array(names)
        types = np.array(types)

        # Create columns in the table workspace
        for name, column_type in zip(names, types):
            indexed.addColumn(column_type, name)

        # Copy all columns from original workspace, ignoring HKLs
        column_data = []
        idx = np.arange(0, names.size)
        hkl_mask = (idx < 2) | (idx > 4 + (ndim - 3))
        for name in names[hkl_mask]:
            column_data.append(fractional_peaks.column(name))

        # Insert the addtional HKL columns into the data
        for i, col in enumerate(hklm.T.tolist()):
            column_data.insert(i + 2, col)

        # Insert the columns into the table workspace
        for i in range(fractional_peaks.rowCount()):
            row = [column_data[j][i] for j in range(indexed.columnCount())]
            indexed.addRow(row)

        return indexed


AlgorithmFactory.subscribe(IndexSatellitePeaks)
