# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import AddSampleLog, DeleteWorkspace, GetEiT0atSNS, Load, LoadInstrument
from mantid.kernel import DateAndTime


class GetEiT0atSNSTest(unittest.TestCase):
    def testGETS(self):
        w = Load("ADARAMonitors.nxs")
        run = w.getRun()
        run.setStartAndEndTime(DateAndTime("2015-01-27T11:00:00"), DateAndTime("2015-01-27T11:57:51"))
        LoadInstrument(Workspace=w, InstrumentName="SEQUOIA", RewriteSpectraMap=False)
        AddSampleLog(Workspace=w, LogName="vChTrans", LogText="1", LogType="Number Series")
        AddSampleLog(Workspace=w, LogName="EnergyRequest", LogText="20", LogType="Number Series")
        res = GetEiT0atSNS(w)
        self.assertAlmostEqual(res[0], 20.09, places=2)
        self.assertAlmostEqual(res[1], 30.415, places=2)
        try:
            res = GetEiT0atSNS(w, 0.1)
        except Exception as e:
            s = "GetEiT0atSNS-v1: Could not get Ei, and this is not a white beam run\nNo peak found for the monitor with spectra num: 2"
            self.assertTrue(str(e).startswith(s))
        DeleteWorkspace(w)


if __name__ == "__main__":
    unittest.main()
