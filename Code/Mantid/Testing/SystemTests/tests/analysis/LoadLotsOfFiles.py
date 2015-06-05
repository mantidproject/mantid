#pylint: disable=invalid-name,no-init
from mantid.simpleapi import *
from mantid.api import FrameworkManager
import copy
import os
import re
import stresstesting

BANNED_FILES = ['992 Descriptions.txt',
                'directBeamDatabaseFall2014_IPTS_11601_2.cfg',
                'BASIS_AutoReduction_Mask.xml',
                'BioSANS_dark_current.xml',
                'BioSANS_empty_cell.xml',
                'BioSANS_empty_trans.xml',
                'BioSANS_exp61_scan0004_0001.xml',
                'BioSANS_flood_data.xml',
                'BioSANS_sample_trans.xml',
                'CNCS_TS_2008_08_18.dat',
                'DISF_NaF.cdl',
                'det_corrected7.dat',
                'det_LET_cycle12-3.dat',
                'eqsans_configuration.1463',
                'FLAT_CELL.061',
                'HYSA_mask.xml',
                'IN10_P3OT_350K.inx',
                'IN13_16347.asc',
                'IN16_65722.asc',
                'IP0005.dat',
                'batch_input.csv',
                'mar11015.msk',
                'LET_hard.msk', #It seems loade does not understand it?
                'MASK.094AA',
                'MASKSANS2D_094i_RKH.txt',
                'MASKSANS2D.091A',
                'MASKSANS2Doptions.091A',
                'MaskSANS2DReductionGUI.txt',
                'MaskSANS2DReductionGUI_MaskFiles.txt',
                'MaskSANS2DReductionGUI_LimitEventsTime.txt',
                'MAP17269.raw', # Don't need to check multiple MAPS files
                'MAP17589.raw',
                'MER06399.raw', # Don't need to check multiple MERLIN files
                'PG3_11485-1.dat', # Generic load doesn't do very well with ASCII files
                'PG3_2538_event.nxs', # Don't need to check all of the PG3 files
                'PG3_9829_event.nxs',
                'REF_M_9684_event.nxs',
                'REF_M_9709_event.nxs',
                'SANS2D_periodTests.csv',
                'SANS2D_992_91A.csv',
                'SANS2D_mask_batch.csv',
                'sans2d_reduction_gui_batch.csv',
                'squaricn.phonon',
                'squaricn.castep',
                'target_circles_mask.xml',
                'linked_circles_mask.xml',
                'testCansas1DMultiEntry.xml',
                'Wish_Diffuse_Scattering_ISAW_UB.mat',
                'WSH_test.dat',
                'SANS2D_multiPeriodTests.csv',
                'SANS2D_periodTests.csv',
                'DIRECTM1_15785_12m_31Oct12_v12.dat',
                'MaskSANS2DReductionGUI.txt',
                'sans2d_reduction_gui_batch.csv'
                'MANTID_FLAT_CELL.115',
                'MaskLOQData.txt',
                'DIRECTHAB.983',
                'loq_batch_mode_reduction.csv',
                'det_corrected7.nxs', # this file can be loaded by LoadDetectorInfo but I am not sure if generic loader should ever deal with it
                'poldi2013n006903.hdf',
                'poldi2013n006904.hdf',
                'poldi2014n019874.hdf',
                'poldi2014n019881.hdf',
                'poldi2015n000977.hdf'
                ]

EXPECTED_EXT = '.expected'

BANNED_REGEXP = [r'SANS2D\d+.log$',
                 r'SANS2D00000808_.+.txt$',
                 r'.*_reduction.log$',
                 r'.+_characterization_\d+_\d+_\d+.*\.txt',
                 r'.*\.cal',
                 r'.*\.detcal',
                 r'.*Grouping\.xml',
                 r'.*\.map',
                 r'.*\.irf',
                 r'.*\.hkl',
                 r'EVS.*\.raw',
                 r'.*_pulseid\.dat']

# This list stores files that will be loaded first.
# Implemented as simple solution to avoid failures on
# WinXP where small files have trouble allocating larger
# amounts of contiguous memory.
# Usage of XP is getting lower so we don't want to compromise the
# performance of the code elsewhere just to pass here
PRIORITY_FILES = ['HYS_13658_event.nxs',
                  'ILLIN5_Sample_096003.nxs',
                  'ILLIN5_Vana_095893.nxs']

def useDir(direc):
    """Only allow directories that aren't test output or
    reference results."""
    if "reference" in direc:
        return False
    if config["defaultsave.directory"] == direc:
        return False
    return "Data" in direc

def useFile(direc, filename):
    """Returns (useFile, abspath)"""
    # if it is an -stamp file then assume these are cmake created files
    if filename.endswith("-stamp"):
        return (False, filename)

    # list of explicitly banned files at the top of this script
    if filename in BANNED_FILES:
        return (False, filename)

    # is an 'expected' file
    if filename.endswith(EXPECTED_EXT):
        return (False, filename)

    # list of banned files by regexp
    for regexp in BANNED_REGEXP:
        if re.match(regexp, filename, re.I) is not None:
            return (False, filename)

    filename = os.path.join(direc, filename)
    if os.path.isdir(filename):
        return (False, filename)
    return (True, filename)

class LoadLotsOfFiles(stresstesting.MantidStressTest):
    def __getDataFileList__(self):
        # get a list of directories to look in
        dirs = config['datasearch.directories'].split(';')
        dirs = [item for item in dirs if useDir(item)]
        print "Looking for data files in:", ', '.join(dirs)

        # Files and their corresponding sizes. the low-memory win machines
        # fair better loading the big files first
        files = {}
        priority_abspaths = copy.deepcopy(PRIORITY_FILES)
        for direc in dirs:
            myFiles = os.listdir(direc)
            for filename in myFiles:
                (good, fullpath) = useFile(direc, filename)
                #print "***", good, filename
                if good:
                    files[fullpath] = os.path.getsize(fullpath)
                    try:
                        cur_index = PRIORITY_FILES.index(filename)
                        priority_abspaths[cur_index] = fullpath
                    except ValueError:
                        pass

        datafiles = sorted(files, key=lambda key: files[key], reverse=True)

        # Put the priority ones first
        for insertion_index, fname in enumerate(priority_abspaths):
            try:
                cur_index = datafiles.index(fname)
            except ValueError:
                continue
            value = datafiles.pop(cur_index)
            datafiles.insert(insertion_index, fname)

        return datafiles

    def __runExtraTests__(self, wksp, filename):
        """Runs extra tests that are specified in '.expected' files
        next to the data files"""
        expected = filename + EXPECTED_EXT
        if not os.path.exists(expected): #file exists
            return True
        if os.path.getsize(expected) <= 0: #non-zero length
            return True

        # Eval statement will use current scope. Allow access to
        # mantid module
        import mantid

        print "Found an expected file '%s' file" % expected
        expectedfile = open(expected)
        tests = expectedfile.readlines()
        failed = [] # still run all of the tests
        for test in tests:
            test = test.strip()
            result = eval(test)
            if not result == True:
                failed.append((test, result))
        if len(failed) > 0:
            for item in failed:
                print "  Failed test '%s' returned '%s' instead of 'True'" % (item[0], item[1])
            return False
        return True


    def __loadAndTest__(self, filename):
        """Do all of the real work of loading and testing the file"""
        print "----------------------------------------"
        print "Loading '%s'" % filename
        from mantid.api import Workspace
        from mantid.api import IMDEventWorkspace
        # Output can be a tuple if the Load algorithm has extra output properties
        # but the output workspace should always be the first argument
        outputs = Load(filename)
        if type(outputs) == tuple:
            wksp = outputs[0]
        else:
            wksp = outputs

        if not isinstance(wksp, Workspace):
            print "Unexpected output type from Load algorithm: Type found=%s" % str(type(outputs))
            return False

        if wksp is None:
            print 'Load returned None'
            return False

        # generic checks
        if wksp.getName() is None or len(wksp.getName()) <= 0:
            print "Workspace does not have a name"
            del wksp
            return False

        id = wksp.id()
        if id is None or len(id) <= 0:
            print "Workspace does not have an id"
            del wksp
            return False

        # checks based on workspace type
        if hasattr(wksp, "getNumberHistograms"):
            if wksp.getNumberHistograms() <= 0:
                print "Workspace has zero histograms"
                del wksp
                return False
            if "managed" not in id.lower() and wksp.getMemorySize() <= 0:
                print "Workspace takes no memory: Memory used=" + str(wksp.getMemorySize())
                del wksp
                return False

        # checks for EventWorkspace
        if hasattr(wksp, "getNumberEvents"):
            if wksp.getNumberEvents() <= 0:
                print "EventWorkspace does not have events"
                del wksp
                return False

        # do the extra checks
        result = self.__runExtraTests__(wksp, filename)

        # cleanup
        del wksp
        return result

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
