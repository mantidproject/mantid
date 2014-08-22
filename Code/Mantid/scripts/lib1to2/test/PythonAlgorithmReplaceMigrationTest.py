"""Tests for the migration with Python algorithms
"""
import unittest
import os

from MigrationTest import MigrationTest

class PythonAlgorithmlReplaceMigrationTest(MigrationTest):

    def tearDown(self):
        """Clean up after a test"""
        self.remove_test_files()

    def test_no_property_alg_is_correct(self):
        inputstring = \
        """
        from MantidFramework import *
        mtd.initialize()

        class MyAlgorithm(PythonAlgorithm):

            def PyInit(self):
                pass

            def PyExec(self):
                pass

        mtd.registerPyAlgorithm(MyAlgorithm())
        """
        expected = \
        """
        from mantid import *
        from mantid.kernel import *
        from mantid.api import *

        class MyAlgorithm(PythonAlgorithm):

            def PyInit(self):
                pass

            def PyExec(self):
                pass

        registerAlgorithm(MyAlgorithm)
        """
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

    def test_basic_property_types_replace_correctly(self):
        inputstring = \
        """
        from MantidFramework import *
        mtd.initialize()
        Name, DefaultValue, Validator = None, Description = '', Direction = Direction.Input):
        class MyAlgorithm(PythonAlgorithm):

            def PyInit(self):
                self.declareProperty(Name="StringKeywords",DefaultValue="", Validator=MandatoryValidator(),Description="description",Direction=Direction.Input)
                self.declareProperty("StringNonKeywords","", MandatoryValidator(),"description",Direction.Input)
                self.declareProperty("StringNotAllArgs","")
                self.declareProperty("StringMixKeywordsAndPos","",Description="desc")

                self.declareProperty(Name="NonStringKeywords",DefaultValue="", Validator=MandatoryValidator(),Description="description",Direction=Direction.Input)
                self.declareProperty("NonStringNonKeywords", 1, MandatoryValidator(),"description",Direction.Input)
                self.declareProperty("NonStringNotAllArgs",1)
                self.declareProperty("NonStringMixKeywordsAndPos",1,Description="desc")


            def PyExec(self):
                pass

        mtd.registerPyAlgorithm(MyAlgorithm())
        """
        expected = \
        """
        from mantid import *
        from mantid.kernel import *
        from mantid.api import *

        class MyAlgorithm(PythonAlgorithm):

            def PyInit(self):
                self.declareProperty(name="StringKeywords",defaultValue="", validator=MandatoryValidator(),doc="description",direction=Direction.Input)
                self.declareProperty("StringNonKeywords","", MandatoryValidator(),"description",Direction.Input)
                self.declareProperty("StringNotAllArgs","")
                self.declareProperty("StringMixKeywordsAndPos","",Description="desc")

                self.declareProperty(Name="NonStringKeywords",DefaultValue="", Validator=MandatoryValidator(),Description="description",Direction=Direction.Input)
                self.declareProperty("NonStringNonKeywords", 1, IntMandatoryValidator(),"description",Direction.Input)
                self.declareProperty("NonStringNotAllArgs",1)
                self.declareProperty("NonStringMixKeywordsAndPos",1,doc="desc")

            def PyExec(self):
                pass

        registerAlgorithm(MyAlgorithm)
        """
        self.do_migration(inputstring)
        self.check_outcome(inputstring, expected)

if __name__ == "__main__":
    unittest.main()
