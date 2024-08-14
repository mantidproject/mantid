# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import mantid
import mantid.api
import mantid.simpleapi
import mantid.kernel
import numpy
from mantid.simpleapi import GenerateGroupingSNSInelastic
from testhelpers import run_algorithm
import os


class GenerateGroupingSNSInelasticTest(unittest.TestCase):
    __IDF_dirs = mantid.simpleapi.config["datasearch.directories"].split(";")
    __IDF_test = [p for p in __IDF_dirs if "instrument" in p][0]
    SEQUOIA_old_IDF = os.path.join(__IDF_test, "SEQUOIA_Definition_20180513_20190403.xml")
    ALF_old_IDF = os.path.join(__IDF_test, "ALF_Definition_20130317-.xml")
    CNCS_old_IDF = os.path.join(__IDF_test, "CNCS_Definition_1-35154.xml")

    def test_Generate_Grouping_Files_from_CNCS(self):
        """Test scenario: instrument == "CNCS" and InstrumentDefinitionFile is not used.
        Create an instrument definition file associated with CNCS".
        """

        alg_test = run_algorithm("GenerateGroupingSNSInelastic", instrument="CNCS", Filename="GenerateGroupingSNSInelasticTest.xml")

        outfilename = alg_test.getProperty("Filename").value
        instrument_name = alg_test.getProperty("instrument").value

        self.assertTrue(alg_test.isExecuted())
        self.assertTrue(os.path.exists(outfilename))
        self.assertEqual(instrument_name, "CNCS")
        # __y = mantid.simpleapi.LoadDetectorsGroupingFile(outfilename)
        # groupnum=__y.readY(51199)
        # self.assertEqual(groupnum, 51200)

        os.remove(outfilename)

    def test_Instrument_and_instrumentDefinitionFile_both_selected(self):
        """Test scenario: instrument == "SEQUOIA" and an old CNCS InstrumentDefinitionFile is used.
        Checks the correct instrument("SEQUOIA") is used not CNCS.
        """
        # with self.assertRaisesRegex(RuntimeError, "Set instrument to InstrumentDefinitionFile") as cm:

        alg_test = run_algorithm(
            "GenerateGroupingSNSInelastic",
            instrument="SEQUOIA",
            InstrumentDefinitionFile=self.CNCS_old_IDF,
            Filename="GenerateGroupingSNSInelasticTest.xml",
        )

        self.assertTrue(alg_test.isExecuted())
        outfilename = alg_test.getProperty("Filename").value
        with open(outfilename) as f:
            data = f.read()
        self.assertTrue("SEQUOIA" in data)
        self.assertFalse("CNCS" in data)
        os.remove(outfilename)

    def test_Instrument_set_to_InstrumentDefinitionFile(self):
        """Test scenario: instrument == "InstrumentDefinitionFile" and InstrumentDefinitionFile is not used.
        Catch an error to remind user to put a valid file in InstrumentDefinitionFile.
        """

        with self.assertRaisesRegex(RuntimeError, "invalid") as cm:
            GenerateGroupingSNSInelastic(
                instrument="InstrumentDefinitionFile",
                #  InstrumentDefinitionFile="",
                Filename="GenerateGroupingSNSInelasticTest.xml",
            )

    def test_Instrument_notin_DGS(self):
        """Test scenario: instrument == "InstrumentDefinitionFile" and InstrumentDefinitionFile is used, but selected
        file does not belong to DGS/SNS suite.
        Catch an error to remind user to put a valid file in InstrumentDefinitionFile.
        """

        with self.assertRaisesRegex(
            RuntimeError, "Select the instrument definition file from only one of ARCS, SEQUOIA, CNCS, HYSPEC"
        ) as cm:
            GenerateGroupingSNSInelastic(
                instrument="InstrumentDefinitionFile",
                InstrumentDefinitionFile=self.ALF_old_IDF,
                Filename="GenerateGroupingSNSInelasticTest.xml",
            )

    def test_Generate_Grouping_Files_from_InstrumentDefinitionFile(self):
        """Test scenario: instrument == "InstrumentDefinitionFile" and InstrumentDefinitionFile is set to "SEQUOIA_Definition_20180513_20190403.xml".
        Check the correct IDF file is generated and that the associated group number is 115712.
        """

        alg_test = run_algorithm(
            "GenerateGroupingSNSInelastic",
            instrument="InstrumentDefinitionFile",
            InstrumentDefinitionFile=self.SEQUOIA_old_IDF,
            Filename="GenerateGroupingSNSInelasticTest.xml",
        )

        outfilename = alg_test.getProperty("Filename").value
        IDF_name = alg_test.getProperty("InstrumentDefinitionFile").value

        __w = mantid.simpleapi.LoadEmptyInstrument(Filename=IDF_name)
        instrument_name = __w.getInstrument().getName()

        self.assertTrue(alg_test.isExecuted())
        self.assertTrue(os.path.exists(outfilename))
        self.assertEqual(instrument_name, "SEQUOIA")
        __y = mantid.simpleapi.LoadDetectorsGroupingFile(outfilename)
        groupnum = __y.readY(119807)
        self.assertEqual(groupnum, 115712)

        os.remove(outfilename)


if __name__ == "__main__":
    unittest.main()
