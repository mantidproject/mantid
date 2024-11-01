# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import CorrectLogTimes, CreateSingleValuedWorkspace, DeleteWorkspace, LoadNexusLogs


class CorrectLogTimesTest(unittest.TestCase):
    def testCLTWrongLog(self):
        w = CreateSingleValuedWorkspace(DataValue="1", ErrorValue="1")
        LoadNexusLogs(Workspace=w, Filename="CNCS_7860_event.nxs")

        try:
            CorrectLogTimes(Workspace=w, LogNames="s1")
            self.fail("Should not have got here. Should throw because wrong instrument.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace(w)

    def testCLTsingle(self):
        w = CreateSingleValuedWorkspace(DataValue="1", ErrorValue="1")
        LoadNexusLogs(Workspace=w, Filename="CNCS_7860_event.nxs")
        self.assertNotEqual(w.getRun()["proton_charge"].firstTime(), w.getRun()["Speed4"].firstTime())
        CorrectLogTimes(Workspace=w, LogNames="Speed4")
        self.assertEqual(w.getRun()["proton_charge"].firstTime(), w.getRun()["Speed4"].firstTime())
        self.assertNotEqual(w.getRun()["proton_charge"].firstTime(), w.getRun()["Speed5"].firstTime())
        DeleteWorkspace(w)

    def testCLTall(self):
        w = CreateSingleValuedWorkspace(DataValue="1", ErrorValue="1")
        LoadNexusLogs(Workspace=w, Filename="CNCS_7860_event.nxs")
        self.assertNotEqual(w.getRun()["proton_charge"].firstTime(), w.getRun()["Speed4"].firstTime())
        CorrectLogTimes(Workspace=w, LogNames="")
        self.assertEqual(w.getRun()["proton_charge"].firstTime(), w.getRun()["Speed4"].firstTime())
        self.assertEqual(w.getRun()["proton_charge"].firstTime(), w.getRun()["Speed5"].firstTime())
        DeleteWorkspace(w)


if __name__ == "__main__":
    unittest.main()
