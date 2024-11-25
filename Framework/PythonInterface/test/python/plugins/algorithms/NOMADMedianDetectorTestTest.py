# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.api import AlgorithmManager
from mantid.simpleapi import LoadNexusProcessed, NOMADMedianDetectorTest

# standard imports
import tempfile
import unittest


class NOMADMedianDetectorTestTest(unittest.TestCase):
    def test_initalize(self):
        alg = AlgorithmManager.create("NOMADMedianDetectorTest")
        alg.initialize()
        self.assertTrue(alg.isInitialized())

    def test_exec(self):
        _, file_xml_mask = tempfile.mkstemp(suffix=".xml")
        _, file_txt_mask = tempfile.mkstemp(suffix=".txt")
        LoadNexusProcessed(Filename="NOM_144974_SingleBin.nxs", OutputWorkspace="NOM_144974")
        NOMADMedianDetectorTest(
            InputWorkspace="NOM_144974",
            ConfigurationFile="NOMAD_mask_gen_config.yml",
            SolidAngleNorm=False,
            OutputMaskXML=file_xml_mask,
            OutputMaskASCII=file_txt_mask,
        )
        # verify the XML mask
        with open(file_xml_mask) as f:
            contents = f.read()
        for segment in ["0-3122", "48847-48900", "65020-65029", "98295-101375"]:  # test a few
            assert segment in contents

        # verify the single-column ASCII mask
        with open(file_txt_mask) as f:
            contents = f.read()
        for detector_id in [0, 3122, 48847, 48900, 65020, 65029, 98295]:
            assert f" {detector_id}\n" in contents


if __name__ == "__main__":
    unittest.main()
