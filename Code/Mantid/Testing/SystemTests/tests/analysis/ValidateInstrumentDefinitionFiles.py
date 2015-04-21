#pylint: disable=no-init
from mantid import config
import os
import stresstesting
import glob


EXPECTED_EXT = '.expected'

class ValidateInstrumentDefinitionFiles(stresstesting.MantidStressTest):

    def skipTests(self):
        try:
            import genxmlif
            import minixsv
        except ImportError:
            return True
        return False

    def __getDataFileList__(self):
        # get a list of directories to look in
        direc = config['instrumentDefinition.directory']
        print "Looking for instrument definition files in: %s" % direc
        cwd = os.getcwd()
        os.chdir(direc)
        myFiles = glob.glob("*Definition*.xml")
        os.chdir(cwd)
        files = []
        for filename in myFiles:
            files.append(os.path.join(direc, filename))
        return files

    def runTest(self):
        """Main entry point for the test suite"""
        from genxmlif import GenXmlIfError
        from minixsv import pyxsval

        # need to extend minixsv library to add method for that forces it to
        # validate against local schema when the xml file itself has
        # reference to schema online. The preference is to systemtest against
        # a local schema file to avoid this systemtest failing is
        # external url temporariliy not available. Secondary it also avoid
        # having to worry about proxies.

        class MyXsValidator(pyxsval.XsValidator):
            ########################################
            # force validation of XML input against local file
            #
            def validateXmlInputForceReadFile (self, xmlInputFile, inputTreeWrapper, xsdFile):
                xsdTreeWrapper = self.parse (xsdFile)
                xsdTreeWrapperList = []
                xsdTreeWrapperList.append(xsdTreeWrapper)
                self._validateXmlInput (xmlInputFile, inputTreeWrapper, xsdTreeWrapperList)
                for xsdTreeWrapper in xsdTreeWrapperList:
                    xsdTreeWrapper.unlink()
                return inputTreeWrapper

        def parseAndValidateXmlInputForceReadFile (inputFile, xsdFile=None, **kw):
            myXsValidator = MyXsValidator(**kw)
            # parse XML input file
            inputTreeWrapper = myXsValidator.parse (inputFile)
            # validate XML input file
            return myXsValidator.validateXmlInputForceReadFile (inputFile, inputTreeWrapper, xsdFile)



        direc = config['instrumentDefinition.directory']
        self.xsdFile =  os.path.join(direc,'Schema/IDF/1.0/','IDFSchema.xsd')
        files = self.__getDataFileList__()

        # run the tests
        failed = []
        for filename in files:
            try:
                print "----------------------------------------"
                print "Validating '%s'" % filename
                parseAndValidateXmlInputForceReadFile(filename, xsdFile=self.xsdFile)
            except Exception, e:
                print "VALIDATION OF '%s' FAILED WITH ERROR:" % filename
                print e
                failed.append(filename)

        # final say on whether or not it 'worked'
        print "----------------------------------------"
        if len(failed) != 0:
            print "SUMMARY OF FAILED FILES"
            for filename in failed:
                print filename
            raise RuntimeError("Failed Validation for %d of %d files" \
                                   % (len(failed), len(files)))
        else:
            print "Succesfully Validated %d files" % len(files)
            
if __name__ == '__main__':
    
    valid = ValidateInstrumentDefinitionFiles()
    valid.runTest()