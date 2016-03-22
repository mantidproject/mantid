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


---


GPSANS()
SetSampleDetectorDistance(19534)
SolidAngle(detector_tubes=True)
MonitorNormalization()
SetAbsoluteScale(1)
AzimuthalAverage(n_bins=100, n_subpix=1, log_binning=False, align_log_with_decades=True)
IQxQy(nbins=100)
SetWedges(number_of_wedges=2, wedge_angle=30, wedge_offset=0)
SetBeamCenter(0, 0)
NoSensitivityCorrection()
SetTransmission(1, 0)
ThetaDependentTransmission(True)
DataPath("/home/rhf/Documents/SANS/GPSANS-Data/20150925-Grasp_Raw_read_ORNL")
AppendDataFile(["/home/rhf/Documents/SANS/GPSANS-Data/20150925-Grasp_Raw_read_ORNL/HiResSANS_exp3_scan0010_0001.xml"])
SaveIq(pro



        '''
        SWANS(keep_events=True)
        OutputPath("/tmp")
        
        # Beam center files
        #DirectBeamCenter("/SNS/VULCAN/IPTS-16013/shared/SANS_detector/RUN80837.dat")
        SetBeamCenter(63,63)
        
        AppendDataFile(["/SNS/VULCAN/IPTS-16013/shared/SANS_detector/RUN80818.dat"])
        
        NoSolidAngle()
        NoIQxQy()
        Resolution(sample_aperture_diameter=10.0)
        SetTOFTailsCutoff(low_cut=5000, high_cut=60000)
        Reduce()

if __name__ == '__main__':
    unittest.main()