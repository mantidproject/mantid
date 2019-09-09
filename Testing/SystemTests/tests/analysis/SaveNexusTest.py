# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name,too-few-public-methods
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.api import FrameworkManager
import os
import glob

EXPECTED_EXT = '.expected'

# files blacklisted due to containing duplicate named components in instrument.
file_blacklist = [("HET_Definition_old.xml","duplicate monitor names"),
                  ("MAPS_Definition.xml","duplicate bank names"),
                  ("MAPS_Definition_2017_06_02.xml", "duplicate bank names"),
                  ("MARI_Definition.xml", "duplicate monitor names"),
                  ("MARI_Definition_19900101_20160911.xml", "duplicate monitor names"),
                  ("MERLIN_Definition.xml", "duplicate monitor names"),
                  ("MERLIN_Definition_2017_02.xml", "duplicate monitor names"),
                  ("MERLIN_Definition_2018_03.xml", "duplicate monitor names"),
                  ("MERLIN_Definition_after2013_4.xml", "duplicate monitor names"),
                  ("NIM_Definition.xml", "duplicate monitor names"),
                  ("POLARIS_Definition_115.xml", "duplicate monitor names"),
                  ("POLARIS_Definition_121.xml", "duplicate monitor names"),
                  ("SANDALS_Definition.xml", "duplicate monitor names")]

print("FOLLOWING FILES ARE BLACKLISTED")
for entry in file_blacklist:
    print(entry[0])
    print("^reason:",entry[1])


class LoadAndSaveLotsOfInstruments(object):
    def __getDataFileList__(self):
        # get a list of directories to look in
        direc = config['instrumentDefinition.directory']
        print("Looking for instrument definition files in: %s" % direc)
        cwd = os.getcwd()
        os.chdir(direc)
        myFiles = glob.glob("*Definition*.xml")
        os.chdir(cwd)
        # Files and their corresponding sizes. the low-memory win machines
        # fair better loading the big files first
        files = []
        valid_files = list(set(myFiles) - set([entry[0] for entry in file_blacklist]))
        valid_files.sort()
        for filename in valid_files:
            files.append(os.path.join(direc, filename))
        files.sort()
        return files

    def __removeFiles__(self, files):
        for ws in files:
            try:
                path = os.path.join(os.path.expanduser("~"), ws)
                os.remove(path)
            except:
                pass

    def __loadSaveAndTest__(self, filename):
        """Do all of the real work of loading and saving the file"""
        print("----------------------------------------")
        print("Loading '%s'" % filename)
        wksp = LoadEmptyInstrument(filename)
        save_file_name = "system_test_save.nxs"
        save_path = os.path.join(os.path.expanduser("~"), save_file_name)
        comp_info = wksp.componentInfo()
        if not (comp_info.hasSource()):
            print("Instrument definition has no source. Skipping test.")
        elif not all([i == 0 for i in comp_info.position(comp_info.sample())]):
            print("Sample in instrument definition not at origin. Skipping test.")
        else:
            print("saving '%s'" % filename)
            SaveNexusGeometry(wksp, save_path)
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
            if not os.path.isfile(save_path):
                print("file '%s' was not saved" % filename)
                del wksp
                return False
        # cleanup
        del wksp
        self.__removeFiles__([save_file_name])
        return True

    def runTest(self):
        """Main entry point for the test suite"""
        files = self.__getDataFileList__()

        # run the tests
        failed = []
        for filename in files:
            try:
                if not self.__loadSaveAndTest__(filename):
                    print("FAILED TO LOAD AND SAVE '%s'" % filename)
                    failed.append(filename)
            #pylint: disable=broad-except
            except Exception as e:
                print("FAILED TO LOAD AND SAVE '%s' WITH ERROR:" % filename)
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
            raise RuntimeError("Failed to load and save %d of %d files"
                               % (len(failed), len(files)))
        else:
            print("Successfully loaded and saved %d files" % len(files))
        print(files)
