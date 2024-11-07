# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import os
from mantid import AnalysisDataServiceImpl, config, simpleapi


class DakotaChiSquaredTest(unittest.TestCase):
    def makeFiles(self):
        simpleapi.CreateWorkspace(OutputWorkspace="data", DataX="1,2,3,4,5", DataY="1,0,1,4,4", DataE="1,0,1,2,2")
        simpleapi.CreateWorkspace(OutputWorkspace="sim", DataX="1,2,3,4,5", DataY="1,1,1,1,1", DataE="0,0,0,0,0")
        simpleapi.CreateWorkspace(OutputWorkspace="simwrong", DataX="1,2,3,4", DataY="1,1,1,1", DataE="0,0,0,0")

        self.datafile = os.path.join(config.getString("defaultsave.directory"), "DakotaChiSquared_data.nxs")
        self.simfile = os.path.join(config.getString("defaultsave.directory"), "DakotaChiSquared_sim.nxs")
        self.simwrongfile = os.path.join(config.getString("defaultsave.directory"), "DakotaChiSquared_simwrong.nxs")
        self.chifile = os.path.join(config.getString("defaultsave.directory"), "DakotaChiSquared_chi.txt")

        simpleapi.SaveNexus("data", self.datafile)
        simpleapi.SaveNexus("sim", self.simfile)
        simpleapi.SaveNexus("simwrong", self.simwrongfile)

        ads = AnalysisDataServiceImpl.Instance()
        ads.remove("data")
        ads.remove("sim")
        ads.remove("simwrong")

    def cleanup(self):
        if os.path.exists(self.datafile):
            os.remove(self.datafile)
        if os.path.exists(self.simfile):
            os.remove(self.simfile)
        if os.path.exists(self.simwrongfile):
            os.remove(self.simwrongfile)
        if os.path.exists(self.chifile):
            os.remove(self.chifile)

    def test_wrongType(self):
        self.makeFiles()
        try:
            simpleapi.DakotaChiSquared(self.datafile, "CNCS_7860_event.nxs", self.chifile)
        except RuntimeError as e:
            self.assertNotEqual(str(e).find("Wrong workspace type for calculated file"), -1)
        except:
            assert False, "Raised the wrong exception type"
        else:
            assert False, "Didn't raise any exception"
        try:
            simpleapi.DakotaChiSquared("CNCS_7860_event.nxs", self.simfile, self.chifile)
        except RuntimeError as e:
            self.assertNotEqual(str(e).find("Wrong workspace type for data file"), -1)
        except:
            assert False, "Raised the wrong exception type"
        else:
            assert False, "Didn't raise any exception"
        self.cleanup()

    def test_wrongSize(self):
        self.makeFiles()
        try:
            simpleapi.DakotaChiSquared(self.datafile, self.simwrongfile, self.chifile)
        except RuntimeError as e:
            self.assertNotEqual(str(e).find("The file sizes are different"), -1)
        except:
            assert False, "Raised the wrong exception type"
        else:
            assert False, "Didn't raise any exception"
        self.cleanup()

    def test_value(self):
        self.makeFiles()
        try:
            simpleapi.DakotaChiSquared(self.datafile, self.simfile, self.chifile)
            f = open(self.chifile, "r")
            chistr = f.read()
            self.assertEqual(chistr, "4.5 obj_fn\n")
            f.close()
        except:
            assert False, "Raised an exception"
        self.cleanup()

    def test_output(self):
        self.makeFiles()
        try:
            alg = simpleapi.DakotaChiSquared(self.datafile, self.simfile, self.chifile)
            self.assertEqual(len(alg), 2)
            self.assertEqual(alg[0], 4.5)
            self.assertEqual(alg[1].name(), "alg")
            self.assertEqual(alg[1].blocksize(), 5)
            self.assertEqual(alg[1].getNumberHistograms(), 1)
            self.assertEqual(alg[1].dataY(0)[3], 1.5)
            ads = AnalysisDataServiceImpl.Instance()
            ads.remove("alg")
            alg1 = simpleapi.DakotaChiSquared(self.datafile, self.simfile, self.chifile, ResidualsWorkspace="res")
            self.assertEqual(alg1[0], 4.5)
            self.assertEqual(alg1[1].name(), "res")
            ads.remove("res")
        except:
            assert False, "Raised an exception"
        self.cleanup()


if __name__ == "__main__":
    unittest.main()
