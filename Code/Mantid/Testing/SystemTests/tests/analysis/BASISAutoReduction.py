"""
System Test for BASIS autoreduction
"""
from mantid.simpleapi import *

import stresstesting
import shutil
import os

class BASISAutoReductionTest(stresstesting.MantidStressTest):

    def requiredFiles(self):
        return ['BSS_13387_event.nxs']

    def cleanup(self):
        return True

    def runTest(self):
        idfdir = config['instrumentDefinition.directory']
        autows = 'data_ws'
        autows_monitor = 'monitor_ws'
        Load(Filename='BSS_13387_event.nxs', OutputWorkspace=autows)
        LoadMask(Instrument='BASIS', OutputWorkspace='BASIS_MASK', InputFile='BASIS_AutoReduction_Mask.xml')
        MaskDetectors(Workspace=autows, DetectorList="5,49,69,113,133,177,197,241,261,305,325,369,389,433,453,497,517,561,581,625,645,689,709,753,773,817,837,881,901,945,965,1009,1029,1073,1093,1137,1157,1201,1221,1265,1285,1329,1349,1393,1413,1457,1477,1521,1541,1585,1605,1649,1669,1713,1733,1777,1797,1841,1861,1905,1925,1969,1989,2033,2053,2097,2117,2161,2181,2225,2245,2289,2309,2353,2373,2417,2437,2481,2501,2545,2565,2609,2629,2673,2693,2737,2757,2801,2821,2865,2885,2929,2949,2993,3013,3057,3077,3121,3141,3185,3205,3249,3269,3313,3333,3377,3397,3441,3461,3505,3525,3569,3589-3633,3653-3697,3717-3761,3781-3825,3845-3889,3909-3953,3973-4017,4037-4081,4110,4154,4174,4218,4238,4282,4302,4346,4366,4410,4430,4474,4494,4538,4558,4602,4622,4666,4686,4730,4750,4794,4814,4858,4878,4922,4942,4986,5006,5050,5070,5114,5134,5178,5198,5242,5262,5306,5326,5370,5390,5434,5454,5498,5518,5562,5582,5626,5646,5690,5710,5754,5774,5818,5838,5882,5902,5946,5966,6010,6030,6074,6094,6138,6158,6202,6222,6266,6286,6330,6350,6394,6414,6458,6478,6522,6542,6586,6606,6650,6670,6714,6734,6778,6798,6842,6862,6906,6926,6970,6990,7034,7054,7098,7118,7162,7182,7226,7246,7290,7310,7354,7374,7418,7438,7482,7502,7546,7566,7610,7630,7674,7694-7738,7758-7802,7822-7866,7886-7930,7950-7994,8014-8058,8078-8122,8142-8186,8192-15871") #MaskedWorkspace='BASIS_MASK')
        ModeratorTzeroLinear(InputWorkspace=autows,OutputWorkspace=autows)
        LoadParameterFile(Workspace=autows, Filename=os.path.join(idfdir,'BASIS_silicon_111_Parameters.xml'))
        LoadNexusMonitors(Filename='BSS_13387_event.nxs', OutputWorkspace=autows_monitor)
        Rebin(InputWorkspace=autows_monitor,OutputWorkspace=autows_monitor,Params='10')
        ConvertUnits(InputWorkspace=autows_monitor, OutputWorkspace=autows_monitor, Target='Wavelength')
        OneMinusExponentialCor(InputWorkspace=autows_monitor, OutputWorkspace=autows_monitor, C='0.20749999999999999', C1='0.001276')
        Scale(InputWorkspace=autows_monitor, OutputWorkspace=autows_monitor, Factor='9.9999999999999995e-07')
        ConvertUnits(InputWorkspace=autows, OutputWorkspace=autows, Target='Wavelength', EMode='Indirect')
        RebinToWorkspace(WorkspaceToRebin=autows, WorkspaceToMatch=autows_monitor, OutputWorkspace=autows)
        Divide(LHSWorkspace=autows, RHSWorkspace=autows_monitor,  OutputWorkspace=autows)
        ConvertUnits(InputWorkspace=autows, OutputWorkspace=autows, Target='DeltaE', EMode='Indirect')
        CorrectKiKf(InputWorkspace=autows, OutputWorkspace=autows,EMode='Indirect')

        Rebin(InputWorkspace=autows, OutputWorkspace=autows, Params='-0.12,0.0004,0.12')
		#GroupDetectors(InputWorkspace=autows, OutputWorkspace=autows, MapFile='/SNS/BSS/shared/autoreduce/BASIS_Grouping.xml', Behaviour='Sum')
        SofQW3(InputWorkspace=autows, OutputWorkspace=autows+'_sqw', QAxisBinning='0.2,0.2,2.0', EMode='Indirect', EFixed='2.082')
		#SaveDaveGrp(Filename=dave_grp_filename, InputWorkspace=autows+'_sqw', ToMicroEV=True)
		#SaveNexus(Filename="basis_auto_sqw.nxs", InputWorkspace=autows+'_sqw')

    def validate(self):
		# Need to disable checking of the Spectra-Detector map because it isn't
		# fully saved out to the nexus file; some masked detectors should be picked
		# up with by the mask values in the spectra
        self.tolerance = 1e-7
        self.disableChecking.append('Axes')
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
        return 'data_ws_sqw','BASISAutoReduction.nxs'

