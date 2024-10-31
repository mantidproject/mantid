# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines tests for the simple property declarations types within
Python algorithms
"""

import unittest
import testhelpers

from mantid.kernel import IntBoundedValidator, Direction
from mantid.api import FileProperty, FileAction, PythonAlgorithm


# ======================================================================


class PythonAlgorithmPropertiesTest(unittest.TestCase):
    def test_simple_property_declarations_have_correct_attrs(self):
        """
        Test the basic property declarations without validators
        """

        class BasicPropsAlg(PythonAlgorithm):
            _testdocstring = "This is a doc string"

            def PyInit(self):
                self.declareProperty("SimpleInput", 1)
                self.declareProperty("Switch", True)
                self.declareProperty("SimpleOutput", 1.0, Direction.Output)
                self.declareProperty("InputString", "", Direction.Input)
                self.declareProperty("PropWithDocDefaultDir", 1, self._testdocstring)
                self.declareProperty("PropWithDocOutputDir", 1.0, self._testdocstring, Direction.Output)

            def PyExec(self):
                pass

        ##########################################################################
        alg = BasicPropsAlg()
        props = alg.getProperties()
        self.assertEqual(0, len(props))
        alg.initialize()
        props = alg.getProperties()
        self.assertEqual(6, len(props))

        input = alg.getProperty("SimpleInput")
        self.assertEqual(input.direction, Direction.Input)
        self.assertEqual(input.value, 1)
        switch = alg.getProperty("Switch")
        self.assertEqual(switch.direction, Direction.Input)
        self.assertEqual(switch.value, True)
        output = alg.getProperty("SimpleOutput")
        self.assertEqual(output.direction, Direction.Output)
        self.assertEqual(output.value, 1.0)
        str_prop = alg.getProperty("InputString")
        self.assertEqual(str_prop.direction, Direction.Input)
        self.assertEqual(str_prop.value, "")

        doc_prop_def_dir = alg.getProperty("PropWithDocDefaultDir")
        self.assertEqual(doc_prop_def_dir.direction, Direction.Input)
        self.assertEqual(doc_prop_def_dir.documentation, alg._testdocstring)
        doc_prop_out_dir = alg.getProperty("PropWithDocOutputDir")
        self.assertEqual(doc_prop_out_dir.direction, Direction.Output)
        self.assertEqual(doc_prop_out_dir.documentation, alg._testdocstring)

    def test_properties_obey_attached_validators(self):
        """
        Test property declarations with validator.
        The validators each have their own test.
        """

        class PropertiesWithValidation(PythonAlgorithm):
            def PyInit(self):
                only_positive = IntBoundedValidator()
                only_positive.setLower(0)
                self.declareProperty("NumPropWithDefaultDir", -1, only_positive)
                self.declareProperty("NumPropWithInOutDir", -1, only_positive, "doc string", Direction.InOut)

            def PyExec(self):
                pass

        ###################################################
        alg = PropertiesWithValidation()
        alg.initialize()
        props = alg.getProperties()
        self.assertEqual(2, len(props))

        def_dir = alg.getProperty("NumPropWithDefaultDir")
        self.assertEqual(def_dir.direction, Direction.Input)
        self.assertNotEqual("", def_dir.isValid)
        self.assertRaises(ValueError, alg.setProperty, "NumPropWithDefaultDir", -10)
        testhelpers.assertRaisesNothing(self, alg.setProperty, "NumPropWithDefaultDir", 11)

    def test_specialized_property_declaration(self):
        """
        Test property declaration using a specialised property.
        The property types should have their own tests too.
        """

        class SpecializedProperties(PythonAlgorithm):
            _testdocstring = "This is a FileProperty"

            def PyInit(self):
                self.declareProperty(FileProperty("NoDocString", "", FileAction.Load))
                self.declareProperty(FileProperty("WithDocString", "", FileAction.Load), self._testdocstring)

            def PyExec(self):
                pass

        ####################################################
        alg = SpecializedProperties()
        alg.initialize()
        props = alg.getProperties()
        self.assertEqual(2, len(props))

        nodoc = alg.getProperty("NoDocString")
        self.assertTrue(isinstance(nodoc, FileProperty))
        self.assertEqual("", nodoc.documentation)
        withdoc = alg.getProperty("WithDocString")
        self.assertTrue(isinstance(withdoc, FileProperty))
        self.assertEqual(alg._testdocstring, withdoc.documentation)

    def test_passing_settings_object_connects_to_correct_object(self):
        from mantid.kernel import EnabledWhenProperty, PropertyCriterion

        class DummyAlg(PythonAlgorithm):
            def PyInit(self):
                self.declareProperty("BasicProp1", 1)
                self.declareProperty("BasicProp2", 1)
                self.setPropertySettings("BasicProp2", EnabledWhenProperty("BasicProp1", PropertyCriterion.IsDefault))

            def PyExec(self):
                pass

        ##
        alg = DummyAlg()
        alg.initialize()
        settings = alg.getProperty("BasicProp2").settings
        self.assertNotEqual(settings, None)
        self.assertTrue(settings.isEnabled(alg))
        alg.setProperty("BasicProp1", 2)  # not default
        self.assertTrue(not settings.isEnabled(alg))


if __name__ == "__main__":
    unittest.main()
