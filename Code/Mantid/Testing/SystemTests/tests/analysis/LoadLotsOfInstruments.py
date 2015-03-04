from mantid.simpleapi import *
from mantid.api import FrameworkManager
import os
import re
import glob
import stresstesting

EXPECTED_EXT = '.expected'

class LoadLotsOfInstruments(stresstesting.MantidStressTest):
    def __getDataFileList__(self):
        # get a list of directories to look in
        direc = config['instrumentDefinition.directory']
        print "Looking for instrument definition files in: %s" % direc
        cwd = os.getcwd()
        os.chdir(direc)
        myFiles = glob.glob("*Definition*.xml")
        os.chdir(cwd)
        # Files and their corresponding sizes. the low-memory win machines
        # fair better loading the big files first
        files = []
        for filename in myFiles:
            files.append(os.path.join(direc, filename))
        files.sort()
        return files


    def __loadAndTest__(self, filename):
        """Do all of the real work of loading and testing the file"""
        print "----------------------------------------"
        print "Loading '%s'" % filename
        wksp = LoadEmptyInstrument(filename)
        if wksp is None:
            return False

        # TODO standard tests
        if wksp.getNumberHistograms() <= 0:
            del wksp
            return False
        if wksp.getMemorySize() <= 0:
            print "Workspace takes no memory: Memory used=" + str(wksp.getMemorySize())
            del wksp
            return False

        # cleanup
        del wksp
        return True

    def runTest(self):
        """Main entry point for the test suite"""
        files = self.__getDataFileList__()

        # run the tests
        failed = []
        for filename in files:
            try:
                if not self.__loadAndTest__(filename):
                    print "FAILED TO LOAD '%s'" % filename
                    failed.append(filename)
            except Exception, e:
                print "FAILED TO LOAD '%s' WITH ERROR:" % filename
                print e
                failed.append(filename)
            finally:
                # Clear everything for the next test
                FrameworkManager.Instance().clear()

        # final say on whether or not it 'worked'
        print "----------------------------------------"
        if len(failed) != 0:
            print "SUMMARY OF FAILED FILES"
            for filename in failed:
                print filename
            raise RuntimeError("Failed to load %d of %d files" \
                                   % (len(failed), len(files)))
        else:
            print "Successfully loaded %d files" % len(files)
	    print files
