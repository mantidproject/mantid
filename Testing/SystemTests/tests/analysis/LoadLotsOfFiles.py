# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import config, Load
from mantid.api import FrameworkManager
import os
import re
import systemtesting

BANNED_FILES = ['80_tubes_Top_and_Bottom_April_2015.xml',
                '80_tubes_Top_and_Bottom_May_2016.xml',
                '80tubeCalibration_18-04-2016_r9330-9335.nxs',
                '80tube_DIRECT_3146_M1_30April15_r3146.dat',
                '992 Descriptions.txt',
                'directBeamDatabaseFall2014_IPTS_11601_2.cfg',
                'BASIS_AutoReduction_Mask.xml',
                'BioSANS_dark_current.xml',
                'BioSANS_empty_cell.xml',
                'BioSANS_empty_trans.xml',
                'BioSANS_exp61_scan0004_0001.xml',
                'BioSANS_flood_data.xml',
                'BioSANS_sample_trans.xml',
                'C6H5Cl-Gaussian.log',
                'CNCS_TS_2008_08_18.dat',
                'DISF_NaF.cdl',
                'det_corrected7.dat',
                'det_LET_cycle12-3.dat',
                'DIRECT_M1_21Nov15_6x8mm_0.9_20.0_r6279_extrapolated.dat',
                'eqsans_configuration.1463',
                'FLAT_CELL.061',
                'HYSA_mask.xml',
                'IN10_P3OT_350K.inx',
                'IN13_16347.asc',
                'IN16_65722.asc',
                'IP0005.dat',
                'batch_input.csv',
                'mar11015.msk',
                'LET_hard.msk',  # It seems loade does not understand it?
                'MASK.094AA',
                'MASKSANS2D_094i_RKH.txt',
                'MASKSANS2D.091A',
                'MASKSANS2Doptions.091A',
                'MASK_squareBeamstop_20-June-2015.xml',
                'MaskSANS2DReductionGUI.txt',
                'MaskSANS2DReductionGUI_MaskFiles.txt',
                'MaskSANS2DReductionGUI_LimitEventsTime.txt',
                'MASK_SANS2D_FRONT_Edges_16Mar2015.xml',
                'MASK_SANS2D_REAR_Bottom_3_tubes_16May2014.xml',
                'MASK_SANS2D_REAR_Edges_16Mar2015.xml',
                'MASK_SANS2D_REAR_module2_tube12.xml',
                'MASK_SANS2D_beam_stop_4m_x_100mm_2July2015_medium_beamstop.xml',
                'MASK_SANS2D_BOTH_Extras_24Mar2015.xml',
                'MASK_Tube6.xml',
                'MASK_squareBeamstop_6x8Beam_11-October-2016.xml',
                'MAP17269.raw',  # Don't need to check multiple MAPS files
                'MAP17589.raw',
                'MER06399.raw',  # Don't need to check multiple MERLIN files
                'PG3_11485-1.dat',  # Generic load doesn't do very well with ASCII files
                'PG3_2538_event.nxs',  # Don't need to check all of the PG3 files
                'PG3_9829_event.nxs',
                'REF_M_9684_event.nxs',
                'REF_M_9709_event.nxs',
                'REF_M_24945_event.nxs',
                'REF_M_24949_event.nxs',
                'SANS2D_periodTests.csv',
                'SANS2D_992_91A.csv',
                'SANS2D_mask_batch.csv',
                'sans2d_reduction_gui_batch.csv',
                'squaricn.phonon',
                'test_isotopes.phonon',
                'squaricn.castep',
                'target_circles_mask.xml',
                'tube10_mask.xml',
                'linked_circles_mask.xml',
                'testCansas1DMultiEntry.xml',
                'Wish_Diffuse_Scattering_ISAW_UB.mat',
                'WSH_test.dat',
                'WISH00035991.raw',
                'WISH00038237.raw',
                'SANS2D_multiPeriodTests.csv',
                'SANS2D_periodTests.csv',
                'SANS2DTube_ZerroErrorFreeTest.txt',
                'SANS2DTUBES_ZeroErrorFree_batch.csv',
                'DIRECTM1_15785_12m_31Oct12_v12.dat',
                'MaskSANS2DReductionGUI.txt',
                'sans2d_reduction_gui_batch.csv',
                'MANTID_FLAT_CELL.115',
                'MaskLOQData.txt',
                'DIRECTHAB.983',
                'loq_batch_mode_reduction.csv',
                'det_corrected7.nxs',  # this file can be loaded by LoadDetectorInfo; not sure if generic loader should ever deal with it
                'poldi2013n006903.hdf',
                'poldi2013n006904.hdf',
                'poldi2014n019874.hdf',
                'poldi2014n019881.hdf',
                'poldi2015n000977.hdf',
                'USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt',
                'USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt',
                'USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger.txt',
                'USER_Larmor_163F_HePATest_r13038.txt',
                'Vesuvio_IP_file_test.par',
                'IP0004_10.par',
                'Crystalb3lypScratchAbins.out',
                'V15_0000016544_S000_P01.raw',
                'TolueneTAbins.out',
                'TolueneSmallerOrderAbins.out',
                'TolueneLargerOrderAbins.out',
                'TolueneScale.out',
                'TolueneScratchAbins.out',
                'SingleCrystalDiffuseReduction_UB.mat',
                'Na2SiF6_DMOL3.outmol',
                'FE_ALPHA.cif',
                'Fe-gamma.cif',
                'Fe-alpha.cif',
                'Sm2O3.cif',
                'template_ENGINX_241391_236516_North_bank.prm',
                'test_data_Iqxy.dat',
                'BioSANS_test_data_Iqxy.dat',
                'BioSANS_exp61_scan0004_0001_Iqxy.dat',
                'test_data_Iq.xml',
                'BioSANS_exp61_scan0004_0001_Iq.xml',
                'BioSANS_test_data_Iq.xml',
                'BioSANS_exp61_scan0004_0001_Iq.txt',
                'test_data_Iq.txt',
                'BioSANS_test_data_Iq.txt'
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
                 r'.*_pulseid\.dat',
                 r'.*\.phonon']

BANNED_DIRS = ["DocTest", "UnitTest", "reference"]


def useDir(direc):
    """Only allow directories that aren't test output in the banned list"""
    if os.path.basename(direc.rstrip("/").rstrip("\\")) in BANNED_DIRS:
        return False
    if config["defaultsave.directory"] == direc:
        return False
    return "Data" in direc


def useFile(direc, filename):
    """Returns (useFile, abspath)"""
    # if it is an -stamp file then assume these are cmake created files
    if filename.endswith("-stamp"):
        return False, filename

    # list of explicitly banned files at the top of this script
    if filename in BANNED_FILES:
        return False, filename

    # is an 'expected' file
    if filename.endswith(EXPECTED_EXT):
        return False, filename

    # list of banned files by regexp
    for regexp in BANNED_REGEXP:
        if re.match(regexp, filename, re.I) is not None:
            return False, filename

    filename = os.path.join(direc, filename)
    if os.path.isdir(filename):
        return False, filename
    return True, filename


class LoadLotsOfFiles(systemtesting.MantidSystemTest):
    def __getDataFileList__(self):
        # get a list of directories to look in
        dirs = config['datasearch.directories'].split(';')
        dirs = [item for item in dirs if useDir(item)]
        print("Looking for data files in:", ', '.join(dirs))

        # Files
        datafiles = []
        for direc in dirs:
            myFiles = os.listdir(direc)
            for filename in myFiles:
                (good, fullpath) = useFile(direc, filename)
                if good:
                    # there is a further check just before loading
                    # that then file still exists
                    datafiles.append(fullpath)

        datafiles = sorted(datafiles, reverse=True)

        return datafiles

    def __runExtraTests__(self, wksp, filename):
        """Runs extra tests that are specified in '.expected' files
        next to the data files"""
        expected = filename + EXPECTED_EXT
        if not os.path.exists(expected):  # file exists
            return True
        if os.path.getsize(expected) <= 0:  # non-zero length
            return True

        # Eval statement will use current scope. Allow access to
        # mantid module
        import mantid  # noqa

        print("Found an expected file '%s' file" % expected)
        expectedfile = open(expected)
        tests = expectedfile.readlines()
        failed = []  # still run all of the tests
        for test in tests:
            test = test.strip()
            result = eval(test)
            if not result:
                failed.append((test, result))
        if len(failed) > 0:
            for item in failed:
                print("  Failed test '%s' returned '%s' instead of 'True'" % (item[0], item[1]))
            return False
        return True

    def __loadAndTest__(self, filename):
        """Do all of the real work of loading and testing the file"""
        print("----------------------------------------")
        print("Loading '%s'" % filename)
        from mantid.api import Workspace
        # Output can be a tuple if the Load algorithm has extra output properties
        # but the output workspace should always be the first argument
        outputs = Load(filename)
        if isinstance(outputs, tuple):
            wksp = outputs[0]
        else:
            wksp = outputs

        if not isinstance(wksp, Workspace):
            print("Unexpected output type from Load algorithm: Type found=%s" % str(type(outputs)))
            return False

        if wksp is None:
            print('Load returned None')
            return False

        # generic checks
        if wksp.name() is None or len(wksp.name()) <= 0:
            print("Workspace does not have a name")
            del wksp
            return False

        wid = wksp.id()
        if wid is None or len(wid) <= 0:
            print("Workspace does not have an id")
            del wksp
            return False

        # checks based on workspace type
        if hasattr(wksp, "getNumberHistograms"):
            if wksp.getNumberHistograms() <= 0:
                print("Workspace has zero histograms")
                del wksp
                return False
            if "managed" not in wid.lower() and wksp.getMemorySize() <= 0:
                print("Workspace takes no memory: Memory used=" + str(wksp.getMemorySize()))
                del wksp
                return False

        # checks for EventWorkspace
        if hasattr(wksp, "getNumberEvents"):
            if wksp.getNumberEvents() <= 0:
                print("EventWorkspace does not have events")
                del wksp
                return False

        # do the extra checks
        result = self.__runExtraTests__(wksp, filename)

        # cleanup
        del wksp
        return result

    def requiredMemoryMB(self):
        return 18000

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
            raise RuntimeError("Failed to load %d of %d files"
                               % (len(failed), len(files)))
        else:
            print("Successfully loaded %d files" % len(files))

    def excludeInPullRequests(self):
        return True
