import stresstesting
from mantid.simpleapi import *
from isis_reflectometry import quick

class ReflectometryQuickPointDetectorMakeTransmission(stresstesting.MantidStressTest):
    """
    This is a system test for the top-level quick routines. Quick is the name given to the 
    ISIS reflectometry reduction scripts. Uses the point detector functionality with real transmission corrections.
    
    """
    
    def runTest(self):
        defaultInstKey = 'default.instrument'
        defaultInstrument = config[defaultInstKey]
        try:
            config[defaultInstKey] = 'INTER'
            LoadISISNexus(Filename='13463', OutputWorkspace='13463')
            LoadISISNexus(Filename='13464', OutputWorkspace='13464')
            LoadISISNexus(Filename='13460', OutputWorkspace='13460')
    
            transmissionRuns = '13463,13464'
            runNo = '13460'
            incidentAngle = 0.7
            transmissionWs=quick.make_trans_corr(transmissionRuns, stitch_start_overlap=10,
                                  stitch_end_overlap=12, stitch_params=[1.5,0.02,17])
            quick.quick(runNo, trans=transmissionWs, theta=incidentAngle) 
        finally:
            config[defaultInstKey] = defaultInstrument
            
    def validate(self):
        self.disableChecking.append('Instrument')
        return '13460_IvsQ','QuickReferenceResult.nxs'
