from mantid import config
import os
import re
import stresstesting
import glob


EXPECTED_EXT = '.expected'

class ValidateFacilitiesFile(stresstesting.MantidStressTest):
    
    def skipTests(self):
        try:
            import genxmlif
            import minixsv
        except ImportError:
            return True
        return False    
         

    def runTest(self):
        """Main entry point for the test suite"""
        from genxmlif import GenXmlIfError
        from minixsv import pyxsval 
        direc = config['instrumentDefinition.directory']
        filename = os.path.join(direc,'Facilities.xml')
        xsdFile =  os.path.join(direc,'Schema/Facilities/1.0/','FacilitiesSchema.xsd')

        # run the tests
        failed = []
        try:
            print "----------------------------------------"
            print "Validating Facilities.xml"
            pyxsval.parseAndValidateXmlInput(filename, xsdFile=xsdFile, validateSchema=0)
        except Exception, e:
            print "VALIDATION OF Facilities.xml FAILED WITH ERROR:"
            print e
            failed.append(filename)

        # final say on whether or not it 'worked'
        print "----------------------------------------"
        if len(failed) != 0:
            print "SUMMARY OF FAILED FILES"
            raise RuntimeError("Failed Validation of Facilities.xml")
        else:
            print "Succesfully Validated Facilities.xml"