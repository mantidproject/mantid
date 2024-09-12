# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""
Extract or compute the Q values from reduced QENS data
"""

from systemtesting import MantidSystemTest
import mantid.simpleapi as sm
import hashlib


class GetQsInQENSDataSystemTest(MantidSystemTest):
    """Example:
    if qvalues = [0.3, 0.5, 0.7, 0.9] for a particular file, then
    input_string = ' '.join(['{0:6.3f}'.format(Q) for Q in qvalues ])
    and we calculate the md5 hash of this string, given by
    hashlib.md5(input_string.encode('utf-8')).hexdigest()
    """

    _file2md5 = {
        "BASIS_59689_divided_sqw.nxs": "626f115ca81cb9bab5ac419ccd00af3a",  # silicon 111, nexus format
        "BASIS_59689_divided.dat": "626f115ca81cb9bab5ac419ccd00af3a",  # silicon 111, DAVE format
        "BASIS_56795_divided_sqw.nxs": "3cf207a10f451730b1b5899fc8d535a4",  # silicon 311, nexus format
        "BASIS_56795_divided.dat": "3cf207a10f451730b1b5899fc8d535a4",  # silicon 311, DAVE format
        "irs26176_graphite002_red.nxs": "9791b1c50205a3b6f08b9ea0882dfd3d",  # graphite 002, nexus format
        "osiris97944_graphite002_red.nxs": "cdf5550f077eb50c201bf81e8da59502",  # graphite 002, nexus format
    }

    def runTest(self):
        for filename, reference_hash in self._file2md5.items():
            workspace = sm.Load(filename)
            qvalues = sm.GetQsInQENSData(workspace)
            input_string = " ".join(["{0:6.3f}".format(Q) for Q in qvalues])
            hash = hashlib.md5(input_string.encode("utf-8")).hexdigest()
            self.assertEqual(hash, reference_hash)
            sm.DeleteWorkspace(workspace)
