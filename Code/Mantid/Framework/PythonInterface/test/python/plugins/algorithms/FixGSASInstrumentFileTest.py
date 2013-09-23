import unittest
import numpy
from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService

import os

class FixGSASInstrumentFileTest(unittest.TestCase):

    def test_FixLineSize(self):
        """ Test to fix the line width of prm file
        """
        # Create a test file
        prmfilename = "test.prm"
        self._createPrmFile(prmfilename)

        # Execute
        alg_test = run_algorithm("FixGSASInstrumentFile", InputFilename = prmfilename)

        self.assertTrue(alg_test.isExecuted())

        # Verify 
        ofile = open(prmfilename, "r")
        lines = ofile.readlines()
        ofile.close()
        self.assertEqual(len(lines), 55)

        for line in lines:
            ct = len(line.split("\n")[0])
            self.assertEqual(ct, 80)

        # Delete the test prm file
        os.remove(prmfilename)

        return

    def _createPrmFile(self, filename):
        """ Create a .prm file
        """
        prmfile = open(filename, "w")

        prmfile.write("            12345678901234567890123456789012345678901234567890123456789012345678\n")
        prmfile.write("ID    TEST \n")
        prmfile.write("INS   BANK      5\n")
        prmfile.write("INS   FPATH1     60.000000 \n")
        prmfile.write("INS   HTYPE   PNTR\n")
        prmfile.write("INS  1 ICONS 22565.814     0.000     0.000               0.000    0     0.000\n")
        prmfile.write("INS  1BNKPAR     3.180    90.000     0.000     0.000     0.200    1    1\n")
        prmfile.write("INS  1BAKGD     1    4    Y    0    Y\n")
        prmfile.write("INS  1I HEAD LaB6 60Hz CW=0.9\n")
        prmfile.write("INS  1I ITYP    0    4.9944   93.0000     92690\n")
        prmfile.write("INS  1INAME   powgen\n")
        prmfile.write("INS  1PRCF1    -3   21   0.00200\n")
        prmfile.write("INS  1PRCF11       0.000000       0.000000       0.000000       0.000000\n")
        prmfile.write("INS  1PRCF12      12.930000     219.063000       0.000000       1.926000\n")
        prmfile.write("INS  1PRCF13       0.000000       0.000000       0.000000       0.000000\n")
        prmfile.write("INS  1PRCF14       0.000000       0.000000       0.000000       0.000000\n")
        prmfile.write("INS  1PRCF15       0.000000       0.000000       0.000000       0.000000\n")
        prmfile.write("INS  1PRCF16       0.000000                                             \n")
        prmfile.write("INS  1PAB3     90 \n")
        prmfile.write("INS  1PAB3 1   0.19921   5.02811   0.38462   0.14101\n")
        prmfile.write("INS  1PAB3 2   0.24503   6.18457   0.35559   0.13738\n")
        prmfile.write("INS  1PAB3 3   0.29085   7.34103   0.33063   0.13393\n")
        prmfile.write("INS  1PAB3 4   0.33667   8.49750   0.30895   0.13065\n")
        prmfile.write("INS  1PAB3 5   0.38249   9.65396   0.28993   0.12752\n")
        prmfile.write("INS  1PAB3 6   0.42831  10.81039   0.27312   0.12454\n")
        prmfile.write("INS  1PAB3 7   0.47412  11.96647   0.25810   0.12167\n")
        prmfile.write("INS  1PAB3 8   0.51994  13.12071   0.24443   0.11884\n")
        prmfile.write("INS  1PAB3 9   0.56576  14.26907   0.23160   0.11587\n")
        prmfile.write("INS  1PAB310   0.61158  15.40389   0.21903   0.11251\n")
        prmfile.write("INS  1PAB311   0.65740  16.51425   0.20628   0.10851\n")
        prmfile.write("INS  1PAB312   0.70322  17.58758   0.19317   0.10376\n")
        prmfile.write("INS  1PAB313   0.74904  18.61158   0.17981   0.09828\n")
        prmfile.write("INS  1PAB314   0.79486  19.57595   0.16651   0.09227\n")
        prmfile.write("INS  1PAB315   0.84067  20.47326   0.15364   0.08601\n")
        prmfile.write("INS  1PAB316   0.88649  21.29917   0.14151   0.07979\n")
        prmfile.write("INS  1PAB317   0.93231  22.05226   0.13035   0.07383\n")
        prmfile.write("INS  1PAB318   0.97813  22.73344   0.12026   0.06828\n")
        prmfile.write("INS  1PAB319   1.02395  23.34540   0.11124   0.06322\n")
        prmfile.write("INS  1PAB320   1.06977  23.89208   0.10325   0.05867\n")
        prmfile.write("INS  1PAB321   1.11559  24.37815   0.09621   0.05462\n")
        prmfile.write("INS  1PAB322   1.16140  24.80864   0.09002   0.05103\n")
        prmfile.write("INS  1PAB323   1.20722  25.18868   0.08458   0.04785\n")
        prmfile.write("INS  1PAB324   1.25304  25.52331   0.07978   0.04505\n")
        prmfile.write("INS  1PAB325   1.29886  25.81731   0.07555   0.04258\n")
        prmfile.write("INS  1PAB326   1.34468  26.07517   0.07181   0.04039\n")
        prmfile.write("INS  1PAB327   1.39050  26.30099   0.06849   0.03844\n")
        prmfile.write("INS  1PAB328   1.43632  26.49855   0.06554   0.03671\n")
        prmfile.write("INS  1PAB329   1.48214  26.67124   0.06289   0.03516\n")
        prmfile.write("INS  1PAB330   1.52795  26.82209   0.06052   0.03378\n")
        prmfile.write("INS  1PAB331   1.57377  26.95382   0.05839   0.03253\n")
        prmfile.write("INS  1PAB332   1.61959  27.06886   0.05646   0.03141\n")
        prmfile.write("INS  1PAB333   1.66541  27.16932   0.05472   0.03039\n")
        prmfile.write("INS  1PAB334   1.71123  27.25712   0.05313   0.02947\n")
        prmfile.write("INS  1PAB335   1.75705  27.33390   0.05168   0.02863\n")
        prmfile.write("INS  1PAB336   1.80287  27.40115   0.05035   0.02786\n")

        prmfile.close()

        return


if __name__ == '__main__':
    unittest.main()
