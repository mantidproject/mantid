# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.simpleapi import *


class MuonProcessTest(systemtesting.MantidSystemTest):
    def runTest(self):
        # Create custom grouping
        grouping = WorkspaceFactory.createTable()
        grouping.addColumn("vector_int", "Detectors")
        grouping.addRow([range(33, 65)])
        grouping.addRow([range(1, 33)])
        mtd.addOrReplace("MuonProcess_Grouping", grouping)

        # Create custom dead times
        deadTimes = WorkspaceFactory.createTable()
        deadTimes.addColumn("int", "Index")
        deadTimes.addColumn("double", "Value")
        for i in range(1, 65):
            deadTimes.addRow([i, i * 0.01])
        mtd.addOrReplace("MuonProcess_DeadTimes", deadTimes)

        load_result = LoadMuonNexus(Filename="MUSR00015192", OutputWorkspace="MuonProcess_Loaded")
        loaded_time_zero = load_result[2]

        MuonProcess(
            InputWorkspace="MuonProcess_Loaded",
            Mode="Combined",
            DetectorGroupingTable="MuonProcess_Grouping",
            ApplyDeadTimeCorrection=True,
            DeadTimeTable="MuonProcess_DeadTimes",
            SummedPeriodSet="2",
            SubtractedPeriodSet="1",
            TimeZero=0.6,
            LoadedTimeZero=loaded_time_zero,
            Xmin=0.11,
            Xmax=10.0,
            RebinParams="0.032",
            OutputType="PairAsymmetry",
            PairFirstIndex=0,
            PairSecondIndex=1,
            Alpha=0.8,
            OutputWorkspace="MuonProcess_MUSR00015192",
        )

    def validate(self):
        self.disableChecking.append("Uncertainty")
        return "MuonProcess_MUSR00015192", "MuonLoad_MUSR00015192.nxs"

    def cleanup(self):
        mtd.remove("MuonProcess_MUSR00015192")
        mtd.remove("MuonProcess_Grouping")
        mtd.remove("MuonProcess_DeadTimes")
        mtd.remove("MuonProcess_Loaded")
