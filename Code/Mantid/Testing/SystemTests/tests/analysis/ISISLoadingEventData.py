import stresstesting
from mantid.simpleapi import * 

class ISISLoadingEventData(stresstesting.MantidStressTest):
    """ There is no event data inside mantid/Test directory. 
        Hence, all the units test that are specific to ISIS
        when loading EventData should go to here. 
    """
    def runTest(self):
        ev_ws = LoadEventNexus('LET00006278.nxs')
        # isis_vms_compat/SPB[2]
        self.assertEqual(ev_ws.sample().getGeometryFlag(), 1, "It does not read correctly the vms compat (check ")        
        # Isis correct the tof using loadTimeOfFlight method.
        self.assertDelta(ev_ws.getEventList(10).getTofs()[1], 1041.89,0.01, "The ISIS event correction is incorrect (check LoadEventNexus::loadTimeOfFlight")
    def validate(self):
        return True
