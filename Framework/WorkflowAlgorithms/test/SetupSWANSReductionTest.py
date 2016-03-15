import unittest
from mantid.simpleapi import *

import mantid
from mantid.simpleapi import *
from reduction_workflow.instruments.sans.sns_command_interface import *
config = ConfigService.Instance()
config['instrumentName']='SWANS'

class SetupSWANSReductionTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_init(self):
        '''

EQSANS()
SolidAngle(detector_tubes=True)
TotalChargeNormalization()
SetAbsoluteScale(1)
AzimuthalAverage(n_bins=100, n_subpix=1, log_binning=False)
IQxQy(nbins=100)
OutputPath("/home/rhf")
UseConfigTOFTailsCutoff(True)
UseConfigMask(True)
Resolution(sample_aperture_diameter=10)
PerformFlightPathCorrection(True)
SetBeamCenter(0, 0)
NoSensitivityCorrection()
DirectBeamTransmission("/SNS/EQSANS/IPTS-10043/data/EQSANS_25040_event.nxs", "/SNS/EQSANS/IPTS-10043/data/EQSANS_25042_event.nxs", beam_radius=3)
ThetaDependentTransmission(True)
DataPath("/SNS/EQSANS/IPTS-10043/data")
AppendDataFile(["/SNS/EQSANS/IPTS-10043/data/EQSANS_25038_event.nxs"])
CombineTransmissionFits(False)

SaveIq(process='None')
Reduce()

        '''
        SWANS()
        SolidAngle(detector_tubes=True)
        TotalChargeNormalization()
        SetAbsoluteScale(1)
        AzimuthalAverage(n_bins=100, n_subpix=1, log_binning=False)
        IQxQy(nbins=100)
        OutputPath("/home/rhf")
        UseConfigTOFTailsCutoff(True)
        UseConfigMask(True)
        Resolution(sample_aperture_diameter=10)
        PerformFlightPathCorrection(True)
        SetBeamCenter(0, 0)
        NoSensitivityCorrection()
        DirectBeamTransmission("/SNS/VULCAN/IPTS-16013/shared/SANS_detector/RUN80814.dat", "/SNS/VULCAN/IPTS-16013/shared/SANS_detector/RUN80815.dat", beam_radius=3)
        ThetaDependentTransmission(True)
        DataPath("/SNS/VULCAN/IPTS-16013/shared/SANS_detector")
        AppendDataFile(["/SNS/VULCAN/IPTS-16013/shared/SANS_detector/RUN80816.dat"])
        CombineTransmissionFits(False)
        
        SaveIq(process='None')
        Reduce()

if __name__ == '__main__':
    unittest.main()