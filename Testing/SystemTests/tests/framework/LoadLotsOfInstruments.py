# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-few-public-methods
from mantid.api import FrameworkManager
from mantid.kernel import config
from mantid.simpleapi import LoadEmptyInstrument
import os
import glob
import systemtesting

EXPECTED_EXT = ".expected"


class LoadLotsOfInstruments(systemtesting.MantidSystemTest):
    @staticmethod
    def _test_clones():
        r"""Test latest definition files for certain sets of instrument names are actually the same
        Example: we test that CG2_Definition.xml and GPSANS_Definition.xml have the same contents except for
        strings 'CG2' and 'GPSANS'.
        """
        instrument_directory = config["instrumentDefinition.directory"]
        for file_type in ("_Definition.xml", "_Parameters.xml"):
            for clone_set in (("CG2", "GPSANS"), ("CG3", "BIOSANS")):
                original = clone_set[0]  # first item to which we compare the rest of the clones
                original_file = open(os.path.join(instrument_directory, original + file_type)).read()
                for clone in clone_set[1:]:
                    clone_file = open(os.path.join(instrument_directory, clone + file_type)).read()
                    assert original_file == clone_file.replace(clone, original)

    def __getDataFileList__(self):
        # get a list of directories to look in
        direc = config["instrumentDefinition.directory"]
        print("Looking for instrument definition files in: %s" % direc)
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
        print("----------------------------------------")
        print("Loading '%s'" % filename)
        wksp = LoadEmptyInstrument(filename)
        if wksp is None:
            return False

        # TODO standard tests
        if wksp.getNumberHistograms() <= 0:
            del wksp
            return False
        if wksp.getMemorySize() <= 0:
            print("Workspace takes no memory: Memory used=" + str(wksp.getMemorySize()))
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
                    print("FAILED TO LOAD '%s'" % filename)
                    failed.append(filename)
            # pylint: disable=broad-except
            except Exception as e:
                print("FAILED TO LOAD '%s' WITH ERROR:" % filename)
                print(e)
                failed.append(filename)
            finally:
                # Clear everything for the next test
                FrameworkManager.Instance().clear()

        # final say on whether or not it 'worked'
        print("----------------------------------------")
        if len(failed) != 0:
            print("SUMMARY OF FAILED FILES")
            for filename in failed:
                print(filename)
            raise RuntimeError("Failed to load %d of %d files" % (len(failed), len(files)))
        else:
            print("Successfully loaded %d files" % len(files))
        print(files)

        # Additional custom tests
        self._test_clones()
