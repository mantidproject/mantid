#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

class MuonLoadTest(stresstesting.MantidStressTest):

    def runTest(self):
      # Create custom grouping
        grouping = WorkspaceFactory.createTable()
        grouping.addColumn("vector_int", "Detectors")
        grouping.addRow([range(33,65)])
        grouping.addRow([range(1,33)])
        mtd.addOrReplace("MuonLoad_Grouping", grouping)

      # Create custom dead times
        deadTimes = WorkspaceFactory.createTable()
        deadTimes.addColumn("int", "Index")
        deadTimes.addColumn("double", "Value")
        for i in range(1, 65):
            deadTimes.addRow([i, i * 0.01])
        mtd.addOrReplace("MuonLoad_DeadTimes", deadTimes)

	load_result = LoadMuonNexus(Filename = "MUSR00015192",
                                    OutputWorkspace = "MuonLoad_Loaded")
	loaded_time_zero = load_result[2]

        MuonLoad(InputWorkspace = "MuonLoad_Loaded",
		 Mode = "Combined",
                 DetectorGroupingTable = "MuonLoad_Grouping",
                 ApplyDeadTimeCorrection = True,
                 DeadTimeTable = "MuonLoad_DeadTimes",
		 SummedPeriodSet = "2",
		 SubtractedPeriodSet = "1",
                 TimeZero = 0.6,
		 LoadedTimeZero = loaded_time_zero,
                 Xmin = 0.11,
                 Xmax = 10.0,
                 RebinParams = "0.032",
                 OutputType = "PairAsymmetry",
                 PairFirstIndex = 0,
                 PairSecondIndex = 1,
                 Alpha = 0.8,
                 OutputWorkspace = "MuonLoad_MUSR00015192")

    def validate(self):
        return "MuonLoad_MUSR00015192", "MuonLoad_MUSR00015192.nxs"

    def cleanup(self):
        mtd.remove("MuonLoad_MUSR00015192")
        mtd.remove("MuonLoad_Grouping")
        mtd.remove("MuonLoad_DeadTimes")
	mtd.remove("MuonLoad_Loaded")
