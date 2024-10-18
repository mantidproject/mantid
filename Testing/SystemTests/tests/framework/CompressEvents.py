# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import CompressEvents, Integration, LoadEventNexus, SumSpectra


class CompressEventsTesting(systemtesting.MantidSystemTest):
    event_files = ["PG3_4844_event.nxs"]  # /SNS/PG3/IPTS-2767/0/ for 2.5 hours

    def requiredFiles(self):
        return self.event_files

    def runTest(self):
        for filename in self.event_files:
            wkspname = filename.split(".")[0]
            outname = wkspname + "_out"

            LoadEventNexus(Filename=filename, OutputWorkspace=wkspname, LoadMonitors=False)
            totalEvents = mtd[wkspname].getNumberEvents()

            SumSpectra(InputWorkspace=wkspname, OutputWorkspace=wkspname)

            # max for Integration algorithm is not inclusive
            for name in (outname, wkspname):  # first out of place, then in place
                CompressEvents(InputWorkspace=wkspname, OutputWorkspace=name, WallClockTolerance=10.0)
                integral = Integration(InputWorkspace=name, RangeUpper=20000.0)
                compress10s = integral.readY(0)[0]

                CompressEvents(InputWorkspace=wkspname, OutputWorkspace=name, WallClockTolerance=3600.0)
                integral = Integration(InputWorkspace=name, RangeUpper=20000.0)
                compress1h = integral.readY(0)[0]

                CompressEvents(InputWorkspace=wkspname, OutputWorkspace=name)
                integral = Integration(InputWorkspace=name, RangeUpper=20000.0)
                compressfull = integral.readY(0)[0]

                if not (totalEvents == compress10s == compress1h == compressfull):
                    msg = "{} total={:.0f} 10s={:.0f} 1h={:.0f} full={:.0f}".format(
                        name, totalEvents, compress10s, compress1h, compressfull
                    )
                    raise RuntimeError(msg)
