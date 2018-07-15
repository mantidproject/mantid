#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *


class ILLSANSTest(stresstesting.MantidStressTest):

    def tearDown(self):
        mtd.clear()

    def runTest(self):

        SetupILLSANSReduction(
            BeamCenterMethod="DirectBeam",
            BeamCenterFile='ILL/001427.nxs',
            Normalisation="Timer",
            DarkCurrentFile= 'ILL/001420.nxs',
            TransmissionMethod="DirectBeam",
            TransmissionEmptyDataFile= 'ILL/001427.nxs',
            BckTransmissionEmptyDataFile= 'ILL/001427.nxs',
            BackgroundFiles='ILL/001422.nxs',
            BckTransmissionSampleDataFile='ILL/001428.nxs',
            DoAzimuthalAverage=True,
            ComputeResolution=False,
            ReductionProperties="props")

        ILLSANSReduction(Filename='ILL/001425.nxs', TransmissionFilename= 'ILL/001431.nxs',
                         ReductionProperties="props", OutputWorkspace="out")

        self.assertEqual(mtd['out'].getNumberHistograms(), 65538)
        self.assertEqual(mtd['out_Iq'].getNumberHistograms(), 1)
