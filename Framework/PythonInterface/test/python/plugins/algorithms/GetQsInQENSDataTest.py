from __future__ import (absolute_import, division, print_function)

import unittest
import mantid.simpleapi as ms


class GetQsInQENSDataTest(unittest.TestCase):

    # Possible exception messages from the algorithm
    _case2message = {"NO_DETECTORS": "Unable to compute the Q values"
                    }

    def _spawn_case_workspace(self, case):
        if case == "NO_DETECTORS":
            ws = ms.CreateWorkspace(OutputWorkspace='data', DataX=(0, 1), DataY=(0, 0), NSpec=2,
                                    VerticalAxisUnit="Label", VerticalAxisValues=(0, 1))
        return ws

    def test_except_cases(self):
        for case, message in self._case2message.items():
            workspace = self._spawn_case_workspace(case)
            try:
                qvalues = ms.GetQsInQENSData(workspace, RaiseMode=True)
            except RuntimeError as e:
                assert(str(e).find(message) >= 0)
                ms.DeleteWorkspace(workspace)

if __name__=="__main__":
    unittest.main()
