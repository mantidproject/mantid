#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *
from ISISCommandInterface import *

class SANSCentreSample(stresstesting.MantidStressTest):

    def runTest(self):

        SANS2D()

        Set1D()
        Detector("rear-detector")
        MaskFile('MASKSANS2D.091A')

        AssignSample('992.raw')

        FindBeamCentre(60, 280, 19, 100.0/1000.0, -200.0/1000.0)

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')

        return '992_sans_raw','SANSCentreSample.nxs'
