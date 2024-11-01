# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os


class LoadFullprofFileTest(unittest.TestCase):
    def test_LoadHKLFile(self):
        """Test to load a .hkl file"""
        # 1. Create a test file
        hklfilename = "test.hkl"
        self._createHKLFile(hklfilename)

        # 2.
        alg_test = run_algorithm(
            "LoadFullprofFile", Filename=hklfilename, OutputWorkspace="Foo", PeakParameterWorkspace="PeakParameterTable"
        )

        self.assertTrue(alg_test.isExecuted())

        # 3. Verify some values
        tablews = AnalysisDataService.retrieve("PeakParameterTable")
        self.assertEqual(4, tablews.rowCount())

        #   alpha of (11 5 1)/Row 0
        self.assertEqual(0.34252, tablews.cell(0, 3))

        # 4. Delete the test hkl file
        os.remove(hklfilename)
        AnalysisDataService.remove("PeakParameterTable")
        AnalysisDataService.remove("Foo")

        return

    def _createHKLFile(self, filename):
        """Create a .hkl file"""
        hklfile = open(filename, "w")

        hklfile.write("!Phase no.:   1 LaB6 T=300.00 K    381 r")
        hklfile.write(
            "   h   k   l     d-sp        tof        alpha       beta       sigma2     gamma2   m*|F^2|        "
            "lambda     dev.     FWHM     pkX \n"
        )
        hklfile.write(
            "  11   5   1 .3428545     7809.654        0.34252    0.16469      6.94167    0.00000 455.0977         "
            "0.4883     7.2343    13.5123   1.0000\n"
        )
        hklfile.write(
            "  12   1   1 .3440267     7836.354        0.34242    0.16439      7.02898    0.00000 419.2758         "
            "0.4899     7.2500    13.7391   1.0000\n"
        )
        hklfile.write(
            "  12   1   0 .3452109     7863.330        0.34231    0.16409      7.11804    0.00000 328.4061         "
            "0.4916     7.2659    13.6199   1.0000\n"
        )
        hklfile.write(
            "  12   0   0 .3464075     7890.586        0.34221    0.16378      7.20892    0.00000 82.99092         "
            "0.4933     7.2819    13.6682   1.0000"
        )

        hklfile.close()

        return

    def test_LoadPRFFile(self):
        """Test to load a .prf file"""
        # 1. Create  test .prf file
        prffilename = "test.prf"
        self._createPrfFile(prffilename)

        # 2. Execute the algorithm
        alg_test = run_algorithm("LoadFullprofFile", Filename=prffilename, OutputWorkspace="Data", PeakParameterWorkspace="Info")

        self.assertTrue(alg_test.isExecuted())

        # 3. Check data
        dataws = AnalysisDataService.retrieve("Data")
        self.assertEqual(dataws.getNumberHistograms(), 4)
        self.assertEqual(len(dataws.readX(0)), 36)

        #    value
        self.assertEqual(dataws.readX(0)[13], 5026.3223)
        self.assertEqual(dataws.readY(1)[30], 0.3819)

        # 4. Clean
        os.remove(prffilename)
        AnalysisDataService.remove("Data")
        AnalysisDataService.remove("Info")

        return

    def _createPrfFile(self, filename):
        """Create a .prf file for teset"""
        prffile = open(filename, "w")

        prffile.write(
            "LaB6 NIST SRM-660b standard 2013-A Dec PAC    "
            "CELL:    4.15689   4.15689   4.15689   90.0000   90.0000   90.0000   SPGR: P m -3 m\n"
        )
        prffile.write("1   5741     1.00000     1.00000     2.33738 22580.59180     0.00000    1\n")
        prffile.write("1539    0    0\n")
        prffile.write(" T.O.F.    Yobs	Ycal	Yobs-Ycal	Backg	Posr	(hkl)	K       \n")
        prffile.write("   5000.2583      0.3715	  0.4031	 -3.3748	  0.3288\n")
        prffile.write("   5002.2583      0.3590	  0.3963	 -3.3804	  0.3287\n")
        prffile.write("   5004.2593      0.3494	  0.3914	 -3.3851	  0.3286\n")
        prffile.write("   5006.2612      0.3652	  0.3874	 -3.3654	  0.3285\n")
        prffile.write("   5008.2637      0.3676	  0.3832	 -3.3587	  0.3284\n")
        prffile.write("   5010.2666      0.3471	  0.3798	 -3.3758	  0.3284\n")
        prffile.write("   5012.2710      0.3819	  0.3777	 -3.3390	  0.3283\n")
        prffile.write("   5014.2759      0.3271	  0.3773	 -3.3934	  0.3282\n")
        prffile.write("   5016.2817      0.3881	  0.3798	 -3.3348	  0.3281\n")
        prffile.write("   5018.2881      0.3833	  0.3868	 -3.3467	  0.3280\n")
        prffile.write("   5020.2954      0.3684	  0.3921	 -3.3669	  0.3280\n")
        prffile.write("   5022.3037      0.3604	  0.3891	 -3.3719	  0.3279\n")
        prffile.write("   5024.3125      0.3802	  0.3846	 -3.3476	  0.3278\n")
        prffile.write("   5026.3223      0.3487	  0.3812	 -3.3756	  0.3277\n")
        prffile.write("   5028.3325      0.3489	  0.3784	 -3.3726	  0.3277\n")
        prffile.write("   5030.3438      0.3728	  0.3759	 -3.3463	  0.3276\n")
        prffile.write("   5032.3560      0.3425	  0.3743	 -3.3749	  0.3275\n")
        prffile.write("   5034.3691      0.3490	  0.3736	 -3.3678	  0.3274\n")
        prffile.write("   5036.3828      0.3383	  0.3746	 -3.3795	  0.3273\n")
        prffile.write("   5038.3975      0.3677	  0.3789	 -3.3544	  0.3273\n")
        prffile.write("   5040.4126      0.3574	  0.3873	 -3.3730	  0.3272\n")
        prffile.write("   5042.4287      0.3802	  0.3912	 -3.3541	  0.3271\n")
        prffile.write("   5044.4458      0.3741	  0.3880	 -3.3571	  0.3270\n")
        prffile.write("   5046.4639      0.3636	  0.3852	 -3.3647	  0.3269\n")
        prffile.write("   5048.4824      0.3707	  0.3841	 -3.3565	  0.3269\n")
        prffile.write("   5050.5015      0.3697	  0.3824	 -3.3558	  0.3268\n")
        prffile.write("   5052.5220      0.3683	  0.3818	 -3.3566	  0.3267\n")
        prffile.write("   5054.5430      0.3379	  0.3844	 -3.3896	  0.3266\n")
        prffile.write("   5056.5645      0.3444	  0.3865	 -3.3853	  0.3265\n")
        prffile.write("   5058.5874      0.3657	  0.3841	 -3.3616	  0.3265\n")
        prffile.write("   5060.6108      0.3922	  0.3819	 -3.3329	  0.3264\n")
        prffile.write("   5062.6348      0.3568	  0.3815	 -3.3679	  0.3263\n")
        prffile.write("   5064.6602      0.3783	  0.3799	 -3.3448	  0.3262\n")
        prffile.write("   5066.6860      0.3388	  0.3769	 -3.3813	  0.3261\n")
        prffile.write("   5068.7124      0.3528	  0.3744	 -3.3647	  0.3261\n")
        prffile.write("   5070.7402      0.3429	  0.3727	 -3.3729	  0.3260\n")
        prffile.write("6866.4219            	        	        	        	       0	( 13  3  3)	  0  1\n")
        prffile.write("6884.8496            	        	        	        	       0	( 11  7  4)	  0  1\n")
        prffile.write("6884.8496            	        	        	        	       0	( 11  8  1)	  0  1\n")
        prffile.write("6884.8496            	        	        	        	       0	( 13  4  1)	  0  1\n")
        prffile.write("6903.4258            	        	        	        	       0	( 10  7  6)	  0  1\n")
        prffile.write("6903.4258            	        	        	        	       0	( 10  9  2)	  0  1\n")
        prffile.write("6903.4258            	        	        	        	       0	( 11  8  0)	  0  1\n")
        prffile.write("6903.4258            	        	        	        	       0	( 12  5  4)	  0  1\n")
        prffile.write("6903.4258            	        	        	        	       0	( 13  4  0)	  0  1\n")
        prffile.write("6922.1533            	        	        	        	       0	( 12  6  2)	  0  1\n")
        prffile.write("6960.0708            	        	        	        	       0	( 10  9  1)	  0  1\n")
        prffile.write("6960.0708            	        	        	        	       0	( 11  6  5)	  0  1\n")
        prffile.write("6960.0703            	        	        	        	       0	( 13  3  2)	  0  1\n")
        prffile.write("6979.2642            	        	        	        	       0	(  9  8  6)	  0  1\n")
        prffile.write("6979.2642            	        	        	        	       0	( 10  9  0)	  0  1\n")
        prffile.write("6979.2651            	        	        	        	       0	( 12  6  1)	  0  1\n")
        prffile.write("6998.6177            	        	        	        	       0	( 10  8  4)	  0  1\n")
        prffile.write("6998.6187            	        	        	        	       0	( 12  6  0)	  0  1\n")
        prffile.write("7018.1333            	        	        	        	       0	(  9  7  7)	  0  1\n")
        prffile.write("7018.1328            	        	        	        	       0	( 11  7  3)	  0  1\n")
        prffile.write("7018.1333            	        	        	        	       0	( 13  3  1)	  0  1\n")
        prffile.write("7037.8135            	        	        	        	       0	(  9  9  4)	  0  1\n")
        prffile.write("7037.8135            	        	        	        	       0	( 12  5  3)	  0  1\n")
        prffile.write("7037.8135            	        	        	        	       0	( 13  3  0)	  0  1\n")
        prffile.write("7057.6597            	        	        	        	       0	(  8  8  7)	  0  1\n")

        prffile.close()


if __name__ == "__main__":
    unittest.main()
