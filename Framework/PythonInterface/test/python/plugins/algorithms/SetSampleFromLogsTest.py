# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import Load, SetSampleFromLogs, AddSampleLog
from math import pi

PAC06_RADIUS = 0.00295
PAC06_HEIGHT = 0.0568


class SetSampleFromLogsTest(unittest.TestCase):
    def loadPG3Data(self, wkspname: str):
        wksp = Load(Filename="PG3_46577.nxs.h5", MetaDataOnly=True, OutputWorkspace=wkspname)
        return wksp

    def validateSample(self, sample, formula: str, height: float):
        self.assertIsNotNone(sample)
        self.assertEqual(sample.getMaterial().name(), formula)

        assert sample.hasEnvironment()
        volume = pi * PAC06_RADIUS * PAC06_RADIUS * height
        self.assertEqual(sample.getShape().volume(), volume)

    def testSpecifyEverything(self):
        """Specify everything. This gets the sample geometry directly from the PAC06 container"""
        wksp = self.loadPG3Data("testSimple")
        material = {"ChemicalFormula": "Si", "SampleMassDensity": 1.165}
        environment = {"Name": "InAir", "Container": "PAC06"}
        geometry = {"Height": PAC06_HEIGHT * 100}  # convert to cm

        SetSampleFromLogs(InputWorkspace=wksp, Environment=environment, Material=material, Geometry=geometry)

        # verify the results
        self.validateSample(wksp.sample(), "Si", PAC06_HEIGHT)

        del wksp  # remove from the ADS

    def testSpecifyNothing(self):
        """Specify almost nothing. This gets the information from the logs."""
        wksp = self.loadPG3Data("testSpecifyNothing")
        material = {}
        environment = {"Name": "InAir"}
        geometry = {}
        SetSampleFromLogs(InputWorkspace=wksp, Environment=environment, Material=material, Geometry=geometry)

        # verify the results
        self.validateSample(wksp.sample(), "Si", 0.04)

        del wksp  # remove from the ADS

    def testSpecifyNothingIgnoreGeometry(self):
        """Specify almost nothing and ignore the geometry from the logs. This gets information from the logs."""
        wksp = self.loadPG3Data("testSpecifyNothingIgnoreGeometry")
        material = {}
        environment = {"Name": "InAir"}
        geometry = {}
        SetSampleFromLogs(InputWorkspace=wksp, Environment=environment, Material=material, Geometry=geometry, FindGeometry=False)

        # verify the results
        self.validateSample(wksp.sample(), "Si", PAC06_HEIGHT)

        del wksp  # remove from the ADS

    def testVolumeZero_raises(self):
        wksp = self.loadPG3Data("testVolumeZero")
        # Set height to 0, should fail
        AddSampleLog(wksp, LogName="BL11A:CS:ITEMS:HeightInContainer", LogText="0.", LogType="Number Series", NumberType="Double")

        material = {}
        environment = {"Name": "InAir"}
        geometry = {}

        with self.assertRaisesRegex(RuntimeError, "Resulting sample shape has volume of 0"):
            SetSampleFromLogs(InputWorkspace=wksp, Environment=environment, Material=material, Geometry=geometry)

        del wksp  # remove from the ADS


if __name__ == "__main__":
    unittest.main()
