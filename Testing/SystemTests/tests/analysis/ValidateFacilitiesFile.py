# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
from mantid import config
import os
import systemtesting


EXPECTED_EXT = '.expected'


class ValidateFacilitiesFile(systemtesting.MantidSystemTest):

    def skipTests(self):
        try:
            import minixsv # noqa
        except ImportError:
            return True
        return False

    def runTest(self):
        """Main entry point for the test suite"""
        from minixsv import pyxsval
        direc = config['instrumentDefinition.directory']
        filename = os.path.join(direc,'Facilities.xml')
        xsdFile =  os.path.join(direc,'Schema/Facilities/1.0/','FacilitiesSchema.xsd')

        # run the tests
        failed = []
        try:
            print("----------------------------------------")
            print("Validating Facilities.xml")
            pyxsval.parseAndValidateXmlInput(filename, xsdFile=xsdFile, validateSchema=0)
        except Exception as e:
            print("VALIDATION OF Facilities.xml FAILED WITH ERROR:")
            print(e)
            failed.append(filename)

        # final say on whether or not it 'worked'
        print("----------------------------------------")
        if len(failed) != 0:
            print("SUMMARY OF FAILED FILES")
            raise RuntimeError("Failed Validation of Facilities.xml")
        else:
            print("Successfully Validated Facilities.xml")
