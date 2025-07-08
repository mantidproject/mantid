# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.geometry import Component


class ComponentTest(unittest.TestCase):
    _testws = None

    def setUp(self):
        import testhelpers

        if not self.__class__._testws:
            alg = testhelpers.run_algorithm("LoadEmptyInstrument", Filename="POLDI_Definition_2015.xml", child=True)
            self.__class__._testws = alg.getProperty("OutputWorkspace").value

    def test_component_getFittingParameters(self):
        source_comp = self._testws.getInstrument().getComponentByName("source")
        self.assertTrue(isinstance(source_comp, Component))
        self.assertTrue(source_comp.hasParameter("WavelengthDistribution"))
        self.assertEqual(source_comp.getParameterType("WavelengthDistribution"), "fitting")
        self.assertTrue(source_comp.getFittingParameter("WavelengthDistribution", 1.5) > 0.0)

    def test_non_existing_fitting_param_name(self):
        source = self._testws.getInstrument().getComponentByName("source")
        self.assertFalse(source.hasParameter("nonExistingFitParam"))
        with self.assertRaises(RuntimeError) as err:
            source.getFittingParameter("nonExistingFitParam", 2.5)
        self.assertEqual(err.exception.args[0], "Fitting parameter=nonExistingFitParam could not be extracted from component=source")

    def test_access_existing_param_as_fitting_param(self):
        detector_comp = self._testws.getInstrument().getComponentByName("detector")
        self.assertTrue(detector_comp.hasParameter("radius"))
        with self.assertRaises(RuntimeError) as err:
            detector_comp.getFittingParameter("radius", 2.5)
        self.assertEqual(err.exception.args[0], "Fitting parameter=radius could not be extracted from component=detector")


if __name__ == "__main__":
    unittest.main()
