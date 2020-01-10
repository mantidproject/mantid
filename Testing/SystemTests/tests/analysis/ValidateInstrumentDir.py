# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)

import glob
import os

from mantid import config
import six
import systemtesting

if six.PY2:
    from io import open  # noqa

# Constants
FILE_TO_TEST = None  # "MARI_Definition.xml"
INSTRUMENT_DIR = config['instrumentDefinition.directory']


class ValidateXML(object):

    def skipTests(self):
        try:
            import lxml  # noqa
        except ImportError:
            return True

        return False

    def runTest(self):
        """Main entry point for the test suite"""
        from lxml import etree

        # read local schema
        xsd_file = open(self.xsdpath(), "r", encoding="utf-8")
        xsd_doc = etree.parse(xsd_file)

        def validate_definition(filepath):
            schema = etree.XMLSchema(xsd_doc)
            with open(filepath, "r", encoding="utf-8") as xml_file:
                is_valid = schema.validate(etree.XML(xml_file.read().encode("utf-8")))
            if is_valid:
                return is_valid, None
            else:
                return is_valid, schema.error_log.filter_from_errors()[0]

        if FILE_TO_TEST is None:
            files = self.filelist()
        else:
            files = [os.path.join(INSTRUMENT_DIR, FILE_TO_TEST)]

        # run the tests
        failed = []
        for filename in files:
            print("----------------------------------------")
            print("Validating '%s'" % filename)
            valid, errors = validate_definition(filename)
            if not valid:
                print("VALIDATION OF '%s' FAILED WITH ERROR:" % filename)
                print(errors)
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


class ValidateInstrumentDefinitionFiles(ValidateXML, systemtesting.MantidSystemTest):

    def xsdpath(self):
        return os.path.join(INSTRUMENT_DIR, "Schema", "IDF", "1.0",
                            "IDFSchema.xsd")

    def filelist(self):
        print("Looking for instrument definition files in: %s" % INSTRUMENT_DIR)
        return glob.glob("{}/*Definition*.xml".format(INSTRUMENT_DIR))


class ValidateParameterFiles(ValidateXML, systemtesting.MantidSystemTest):

    def xsdpath(self):
        return os.path.join(INSTRUMENT_DIR, "Schema", "ParameterFile", "1.0",
                            "ParameterFileSchema.xsd")

    def filelist(self):
        print("Looking for instrument definition files in: %s" % INSTRUMENT_DIR)
        return glob.glob("{}/*Parameters*.xml".format(INSTRUMENT_DIR))


class ValidateFacilitiesFile(ValidateXML, systemtesting.MantidSystemTest):

    def xsdpath(self):
        return os.path.join(INSTRUMENT_DIR, "Schema", "Facilities", "1.0",
                            "FacilitiesSchema.xsd")

    def filelist(self):
        return [os.path.join(INSTRUMENT_DIR, 'Facilities.xml')]


class ValidateGroupingFiles(ValidateXML, systemtesting.MantidSystemTest):

    def xsdpath(self):
        return os.path.join(INSTRUMENT_DIR, "Schema", "Grouping", "1.0",
                            "GroupingSchema.xsd")

    def filelist(self):
        grouping_dir = os.path.join(INSTRUMENT_DIR, "Grouping")
        print("Looking for grouping files in: %s" % grouping_dir)
        return glob.glob("{}/*Grouping*.xml".format(grouping_dir))
