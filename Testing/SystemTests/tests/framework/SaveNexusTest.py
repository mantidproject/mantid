# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-few-public-methods
from mantid.api import FrameworkManager
from mantid.kernel import config
from mantid.simpleapi import LoadEmptyInstrument, SaveNexusGeometry
import os
import glob
import tempfile

temp_dir = tempfile.gettempdir()
EXPECTED_EXT = ".expected"

# files blacklisted
duplicate_bank_names = ["MAPS_Definition.xml", "MAPS_Definition_2017_06_02.xml"]
duplicate_monitor_names = [
    "HET_Definition_old.xml",
    "MARI_Definition.xml",
    "MARI_Definition_19900101_20160911.xml",
    "MERLIN_Definition.xml",
    "MERLIN_Definition_2017_02.xml",
    "MERLIN_Definition_2018_03.xml",
    "MERLIN_Definition_after2013_4.xml",
    "NIM_Definition.xml",
    "POLARIS_Definition_115.xml",
    "POLARIS_Definition_121.xml",
    "SANDALS_Definition.xml",
]
no_source = ["ARGUS_Definition.xml"]
sample_not_at_origin = [
    "CRISP_Definition.xml",
    "CRISP_Definition_2018.xml",
    "IMAT_Definition_2015.xml",
    "LARMOR_Definition.xml",
    "LARMOR_Definition_19000000-20150317.xml",
    "LARMOR_Definition_20150429_20180602.xml",
    "LARMOR_Definition_8tubes.xml",
    "LARMOR_Definition_SEMSANS.xml",
    "LOQ_Definition_20010430-20020226.xml",
    "LOQ_Definition_20020226-.xml",
    "LOQ_Definition_20121016-.xml",
    "LOQ_Definition_20151016.xml",
    "LOQ_trans_Definition.xml",
    "LOQ_trans_Definition_M4.xml",
    "POLREF_Definition.xml",
    "POLREF_Definition_2016.xml",
    "SANS2D_Definition.xml",
    "SANS2D_Definition_Tubes.xml",
    "V20_IMAGING_Definition.xml",
    "ZOOM_Definition.xml",
]
known_error_files = duplicate_bank_names + duplicate_monitor_names + no_source + sample_not_at_origin
direc = config["instrumentDefinition.directory"]


class LoadAndSaveLotsOfInstruments(object):
    def __getDataFileList__(self):
        # get a list of directories to look in
        print("Looking for instrument definition files in: %s" % direc)
        cwd = os.getcwd()
        os.chdir(direc)
        instrument_def_files = glob.glob("*Definition*.xml")
        os.chdir(cwd)
        # Files and their corresponding sizes. the low-memory win machines
        # fair better loading the big files first
        files = []
        for filename in instrument_def_files:
            files.append(os.path.join(direc, filename))
        files.sort()
        return files

    def __removeFiles__(self, files):
        for file in files:
            try:
                path = os.path.join(temp_dir, file)
                os.remove(path)
            except:
                pass

    def __loadSaveAndTest__(self, filename):
        """Do all of the real work of loading and saving the file"""
        print("----------------------------------------")
        print("Loading '%s'" % filename)
        wksp = LoadEmptyInstrument(filename=filename, StoreInADS=False)
        save_file_name = "system_test_save.nxs"
        save_path = os.path.join(temp_dir, save_file_name)
        print("saving '%s'" % filename)
        SaveNexusGeometry(wksp, save_path)
        if not os.path.isfile(save_path):
            print("file '%s' was not saved" % filename)
            return False
        # cleanup
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
            # pylint: disable=broad-except
            except ValueError as e:
                print("FAILED TO LOAD AND SAVE '%s' WITH ERROR:" % filename)
                print(e)
                failed.append(filename)
            finally:
                # Clear everything for the next test
                FrameworkManager.Instance().clear()

        # final say on whether or not it 'worked'
        print("----------------------------------------")
        if set(failed) != set(known_error_files):
            newfound_errors = list(set(failed) - set(known_error_files))
            for file in newfound_errors:
                print("Failed: '%s'" % file)
            raise RuntimeError("System test failed.")
        else:
            print("Successfully loaded and saved %d files" % len(files))
