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
import glob

EXPECTED_EXT = '.expected'


class ValidateParameterFiles(systemtesting.MantidSystemTest):

    xsdFile=''

    def skipTests(self):
        try:
            from genxmlif import GenXmlIfError # noqa
            from minixsv import pyxsval # noqa
        except ImportError:
            return True
        return False

    def __getDataFileList__(self):
        # get a list of directories to look in
        direc = config['instrumentDefinition.directory']
        print("Looking for instrument definition files in: %s" % direc)
        cwd = os.getcwd()
        os.chdir(direc)
        myFiles = glob.glob("*Parameters*.xml")
        os.chdir(cwd)
        files = []
        for filename in myFiles:
            files.append(os.path.join(direc, filename))
        return files

    def runTest(self):
        """Main entry point for the test suite"""
        from minixsv import pyxsval # noqa
        direc = config['instrumentDefinition.directory']
        print(direc)
        self.xsdFile =  os.path.join(direc,'Schema/ParameterFile/1.0/','ParameterFileSchema.xsd')
        files = self.__getDataFileList__()

        # run the tests
        failed = []
        for filename in files:
            try:
                print("----------------------------------------")
                print("Validating '%s'" % filename)
                pyxsval.parseAndValidateXmlInput(filename, xsdFile=self.xsdFile, validateSchema=0)
            except Exception as e:
                print("VALIDATION OF '%s' FAILED WITH ERROR:" % filename)
                print(e)
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


if __name__ == '__main__':
    valid = ValidateParameterFiles()
    valid.runTest()
