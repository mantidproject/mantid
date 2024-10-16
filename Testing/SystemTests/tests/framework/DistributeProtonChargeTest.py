# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""
System test for distributing proton charge using FilterEvents and FilterByTime
"""

from mantid.api import mtd
from mantid.simpleapi import ChangeLogTime, DeleteWorkspace, FilterByTime, FilterEvents, GenerateEventsFilter, LoadEventNexus
import systemtesting
import numpy as np
import mantid.kernel as mk


class DistributeProtonChargeTest(systemtesting.MantidSystemTest):
    # tolerance in picoCoulombs is equal to 1 proton charge
    tolerance = 1.6e-7

    def requiredMemoryMB(self):
        return 100

    def requiredFiles(self):
        return ["HYS_331132.nxs.h5"]

    def cleanup(self):
        return True

    def runTest(self):
        w = LoadEventNexus(Filename="HYS_331132.nxs.h5", FilterByTimeStop=30)
        # create a time series property
        ost = w.run()["OpticalShutterTrigger1"]
        lto = w.run()["LaserTrigOut"]
        fsp = mk.FloatTimeSeriesProperty("fsp")
        fsp_value = 10
        for i in range(len(ost.times)):
            if i % 1000 == 1:
                print(i)
            if ost.value[i] == 1:
                diff_min = np.abs(lto.times - ost.times[i]).min()
                if diff_min < np.timedelta64(500, "ns"):
                    fsp_value = 0
                else:
                    fsp_value += 1
                fsp.addValue(ost.times[i], float(fsp_value))
        # add property to workspace
        w.mutableRun().addProperty("fsp", fsp, True)

        # optionally offset the log times
        first_laser_time = fsp.times[np.where(fsp.value == 0)[0][0]]
        first_proton_charge_time = w.run()["proton_charge"].times[0]
        diff_lp = float(first_laser_time - first_proton_charge_time) * 1e-3  # in microseconds
        tofmin = w.readX(0)[0]  # in microseconds
        offset = 1e-6 * (tofmin - diff_lp)  # offset in seconds
        w = ChangeLogTime(w, LogName="fsp", TimeOffset=offset)

        # get the proton charge before event filtering
        wholeProtonCharge = mtd["w"].run().getProtonCharge()
        print("W proton charge: " + str(wholeProtonCharge))
        self.assertDelta(wholeProtonCharge, 11.65611529, self.tolerance)

        # filter the events
        GenerateEventsFilter(
            InputWorkspace="w",
            OutputWorkspace="split",
            InformationWorkspace="info",
            UnitOfTime="Nanoseconds",
            LogName="fsp",
            MinimumLogValue=0,
            MaximumLogValue=7,
            LogValueInterval=1,
        )
        FilterEvents(
            InputWorkspace="w",
            SplitterWorkspace="split",
            OutputWorkspaceBaseName="part",
            InformationWorkspace="info",
            FilterByPulseTime=True,
            GroupWorkspaces=True,
            OutputWorkspaceIndexedFrom1=True,
            OutputUnfilteredEvents=True,
        )

        # tally up proton charges of the partial workspaces
        sumPartsProtonCharge = 0.0
        for ip in range(0, 9):
            if ip == 0:
                partial_ws_name = "part_unfiltered"
            else:
                partial_ws_name = "part_" + str(ip)
            pc = mtd[partial_ws_name].run().getProtonCharge()
            print(partial_ws_name + " proton charge: " + str(pc))
            sumPartsProtonCharge += pc
        print("Sum of parts proton charge: " + str(sumPartsProtonCharge))
        self.assertDelta(sumPartsProtonCharge, wholeProtonCharge, self.tolerance)

        # filter original workspace by time and get the proton charge
        FilterByTime(
            InputWorkspace="w", OutputWorkspace="w_short", AbsoluteStartTime="2022-01-10T08:16:50.753504667", StopTime=0.29999999999999999
        )

        wholeShortProtonCharge = mtd["w_short"].run().getProtonCharge()
        print("Whole (short) proton charge: " + str(wholeShortProtonCharge))
        self.assertDelta(wholeShortProtonCharge, 0.116766472, self.tolerance)

        # filter all partial workspaces by time and tally up their proton charges
        FilterByTime(
            InputWorkspace="part",
            OutputWorkspace="part_short",
            AbsoluteStartTime="2022-01-10T08:16:50.753504667",
            StopTime=0.29999999999999999,
        )
        sumShortPartsProtonCharge = 0.0
        for ip in range(0, 9):
            partial_short_ws_name = "part_short_" + str(ip + 1)
            pc = mtd[partial_short_ws_name].run().getProtonCharge()
            print(partial_short_ws_name + " proton charge: " + str(pc))
            sumShortPartsProtonCharge += pc
        print("Sum of parts (short) proton charge: " + str(sumShortPartsProtonCharge))
        self.assertDelta(sumShortPartsProtonCharge, wholeShortProtonCharge, self.tolerance)

        # clean-up
        DeleteWorkspace(w)
        for ip in range(0, 9):
            if ip == 0:
                partial_ws_name = "part_unfiltered"
            else:
                partial_ws_name = "part_" + str(ip)
            partial_short_ws_name = "part_short_" + str(ip + 1)
            DeleteWorkspace(partial_ws_name)
            DeleteWorkspace(partial_short_ws_name)
