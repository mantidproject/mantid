# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import stresstesting
from mantid.simpleapi import *


class CompressEventsTesting(stresstesting.MantidStressTest):
    event_files = ["PG3_4844_event.nxs"] # /SNS/PG3/IPTS-2767/0/ for 2.5 hours

    def requiredFiles(self):
        return self.event_files

    def runTest(self):
        for filename in self.event_files:
            wkspname = filename.split('.')[0]
            outname = wkspname + '_out'

            LoadEventNexus(Filename=filename, OutputWorkspace=wkspname, LoadMonitors=False)
            totalEvents = mtd[wkspname].getNumberEvents()

            SumSpectra(InputWorkspace=wkspname, OutputWorkspace=wkspname)

            # max for Integration algorithm is not inclusive
            for name in (outname, wkspname): # first out of place, then in place
                CompressEvents(InputWorkspace=wkspname, OutputWorkspace=name,
                               WallClockTolerance=10.)
                integral = Integration(InputWorkspace=name, RangeUpper=20000.)
                compress10s = integral.readY(0)[0]

                CompressEvents(InputWorkspace=wkspname, OutputWorkspace=name,
                               WallClockTolerance=3600.)
                integral = Integration(InputWorkspace=name, RangeUpper=20000.)
                compress1h = integral.readY(0)[0]

                CompressEvents(InputWorkspace=wkspname, OutputWorkspace=name)
                integral = Integration(InputWorkspace=name, RangeUpper=20000.)
                compressfull = integral.readY(0)[0]

                if not (totalEvents == compress10s == compress1h == compressfull):
                    # TODO use new style formatting
                    msg = '%s - total=%f 10s=%f 1h=%f full=%f' % (name, totalEvents, compress10s, compress1h, compressfull)
                    raise RuntimeError(msg)
