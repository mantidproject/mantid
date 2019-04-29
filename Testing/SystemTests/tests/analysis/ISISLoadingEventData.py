# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
import systemtesting
from mantid.simpleapi import LoadEventNexus


class ISISLoadingEventData(systemtesting.MantidSystemTest):
    """Older ISIS event files were actually, effectively, very finely-binned
    histograms where each "event" was assigned the TOF at the centre of a bin.
    Attempting to bin this type of data from event mode to histogram mode would
    then lead to odd effects as you would have clusters of events that were
    articifically at the "same" time period.
    For these files Mantid randomizes the TOF from within the bin it was
    assigned as it loads. Later files have this operation done by the DAE.
    """

    def runTest(self):
        ev_ws = LoadEventNexus('LET00006278.nxs')
        # isis_vms_compat/SPB[2]
        self.assertEqual(ev_ws.sample().getGeometryFlag(), 1,
                         "Geometry flag mismatch. vms_compat block not read correctly")
        # Isis correct the tof using loadTimeOfFlight method.
        self.assertDelta(ev_ws.getSpectrum(10).getTofs()[1], 1041.81, 0.01,
                         "The ISIS event correction is incorrect (check LoadEventNexus::loadTimeOfFlight)")

    def validate(self):
        return True
