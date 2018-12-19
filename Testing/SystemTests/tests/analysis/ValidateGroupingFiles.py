# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)
from mantid import config
import os
import systemtesting
import glob

EXPECTED_EXT = '.expected'


class ValidateGroupingFiles(systemtesting.MantidSystemTest):

    xsdFile =''

    def skipTests(self):
        try:
            import minixsv # noqa
        except ImportError:
            return True
        return False

    def __getDataFileList__(self):
        # get a list of directories to look in
        direc = config['instrumentDefinition.directory']
        direc =  os.path.join(direc,'Grouping')
        print("Looking for Grouping files in: %s" % direc)
        cwd = os.getcwd()
        os.chdir(direc)
        myFiles = glob.glob("*Grouping*.xml")
        os.chdir(cwd)
        files = []
        for filename in myFiles:
            files.append(os.path.join(direc, filename))
        return files

    def runTest(self):
        """Main entry point for the test suite"""
        from minixsv import pyxsval
        direc = config['instrumentDefinition.directory']
        self.xsdFile =  os.path.join(direc,'Schema/Grouping/1.0/','GroupingSchema.xsd')
        files = self.__getDataFileList__()

        # run the tests
        failed = []
        for filename in files:
            try:
                print("----------------------------------------")
                print("Validating '%s'" % filename)
                pyxsval.parseAndValidateXmlInput(filename, xsdFile=self.xsdFile, validateSchema=0)
            except Exception as err:
                print("VALIDATION OF '%s' FAILED WITH ERROR:" % filename)
                print(err)
                failed.append(filename)

        # final say on whether or not it 'worked'
        print("----------------------------------------")
        if len(failed) != 0:
            print("SUMMARY OF FAILED FILES")
            for filename in failed:
                print(filename)
            raise RuntimeError("Failed Validation for %d of %d files"
                               % (len(failed), len(files)))
        else:
            print("Successfully Validated %d files" % len(files))
