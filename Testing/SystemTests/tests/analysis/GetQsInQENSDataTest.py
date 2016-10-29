#pylint: disable=no-init
"""
    Extract or compute the Q values from reduced QENS data
"""
from stresstesting import MantidStressTest
import mantid.simpleapi as sm
import hashlib

class GetQsInQENSDataTest(MantidStressTest):
    """Example:
        if qvalues = [0.3, 0.5, 0.7, 0.9] for a particular file, then
        input_string = ' '.join(['{0:6.3f}'.format(Q) for Q in qvalues ])
        and we calculate the md5 hash of this string, given by
        hashlib.md5(input_string.encode('utf-8')).hexdigest()
    """
    _file2md5 = {"BASIS_59689_divided_sqw.nxs": "",  # silicon 111, nexus format
                 "BASIS_59689_divided.dat": "",  # silicon 111, DAVE format
                 "BASIS_56795_divided_sqw.nxs": "",  # silicon 311, nexus format
                 "BASIS_56795_divided.dat": "",  # silicon 311, DAVE format
                 "irs26173_graphite002_red.nxs": "",  # graphite 002, nexus format
                 "osiris97944_graphite002_red.nxs": ""  # graphite 002, nexus format
                 }

    def runTest(self):
        for filename, reference_hash in self._file2md5.items():
            workspace = sm.LoadNexus(filename)
            qvalues = sm.GetQsInQENSDataTest(workspace)
            input_string = ' '.join(['{0:6.3f}'.format(Q) for Q in qvalues])
            hash = hashlib.md5(input_string.encode('utf-8')).hexdigest()
            self.assertEqual(hash, reference_hash)
            sm.DeleteWorkspace(workspace)
