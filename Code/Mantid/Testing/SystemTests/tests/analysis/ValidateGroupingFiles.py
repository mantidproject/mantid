from mantid import config
import os
import re
import stresstesting
import glob

EXPECTED_EXT = '.expected'

class ValidateGroupingFiles(stresstesting.MantidStressTest):
    
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
	direc =  os.path.join(direc,'Grouping')
        print "Looking for Grouping files in: %s" % direc
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
        from genxmlif import GenXmlIfError
        from minixsv import pyxsval 
        direc = config['instrumentDefinition.directory']
        self.xsdFile =  os.path.join(direc,'Schema/Grouping/1.0/','GroupingSchema.xsd')
        files = self.__getDataFileList__()

        # run the tests
        failed = []
        for filename in files:
            try:
                print "----------------------------------------"
                print "Validating '%s'" % filename
                pyxsval.parseAndValidateXmlInput(filename, xsdFile=self.xsdFile, validateSchema=0)
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
