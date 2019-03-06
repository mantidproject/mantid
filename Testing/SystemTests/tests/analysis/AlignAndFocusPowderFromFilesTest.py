# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import *
from mantid.api import WorkspaceFactory
import numpy as np
import os
import time


def do_cleanup(cacheDir):
    if os.path.exists(cacheDir) and os.path.isdir(cacheDir):
        print('Deleting cache: {}'.format(cacheDir))
        for filename in os.listdir(cacheDir):
            filename = os.path.join(cacheDir, filename)
            os.unlink(filename)
        os.rmdir(cacheDir)
    return True


def getCacheDir():
    """determine where to save - the current working directory"""
    direc = os.path.abspath(os.path.curdir)
    return os.path.join(direc, 'alignandfocuscache')


class SimplestCompare(systemtesting.MantidSystemTest):
    cal_file  = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = 'PG3_9829_event.nxs'

    def cleanup(self):
        return do_cleanup(self.cacheDir)

    def requiredMemoryMB(self):
        return 3*1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        self.cacheDir = getCacheDir()

        PDLoadCharacterizations(Filename=self.char_file, OutputWorkspace='characterizations',
                                SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0')

        self.wksp_mem = os.path.basename(self.data_file).split('.')[0]
        self.wksp_mem, self.wksp_file = self.wksp_mem + '_mem', self.wksp_mem + '_file'

        # load then process
        LoadEventAndCompress(Filename=self.data_file, OutputWorkspace=self.wksp_mem, MaxChunkSize=16, FilterBadPulses=0)
        LoadDiffCal(Filename=self.cal_file, InputWorkspace=self.wksp_mem, WorkspaceName='PG3')
        PDDetermineCharacterizations(InputWorkspace=self.wksp_mem, Characterizations='characterizations',
                                     ReductionProperties='__snspowderreduction_inner')
        AlignAndFocusPowder(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem,
                            GroupingWorkspace='PG3_group', CalibrationWorkspace='PG3_cal', MaskWorkspace='PG3_mask',
                            Params=-.0002, CompressTolerance=0.01,
                            PrimaryFlightPath=60, SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0',
                            ReductionProperties='__snspowderreduction_inner')
        NormaliseByCurrent(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem)
        ConvertUnits(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, Target='dSpacing')

        # everything inside the algorithm
        AlignAndFocusPowderFromFiles(Filename=self.data_file, OutputWorkspace=self.wksp_file,
                                     GroupingWorkspace='PG3_group', CalibrationWorkspace='PG3_cal',
                                     MaskWorkspace='PG3_mask',
                                     Params=-.0002, CompressTolerance=0.01,
                                     PrimaryFlightPath=60, SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0',
                                     ReductionProperties='__snspowderreduction_inner')
        NormaliseByCurrent(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file)
        ConvertUnits(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file, Target='dSpacing')

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return (self.wksp_mem, self.wksp_file)


class UseCache(systemtesting.MantidSystemTest):
    cal_file  = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = 'PG3_9829_event.nxs'

    def cleanup(self):
        return do_cleanup(self.cacheDir)

    def requiredMemoryMB(self):
        return 3*1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        self.cacheDir = getCacheDir()

        PDLoadCharacterizations(Filename=self.char_file, OutputWorkspace='characterizations',
                                SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0')

        # first pass will create the cache file, second will use it
        self.wksp_make = 'makecache'
        self.wksp_use = 'usecache'

        # load then process
        LoadEventNexus(Filename=self.data_file, OutputWorkspace='meta', MetaDataOnly=True)
        LoadDiffCal(Filename=self.cal_file, InputWorkspace='meta', WorkspaceName='PG3')
        PDDetermineCharacterizations(InputWorkspace='meta', Characterizations='characterizations',
                                     ReductionProperties='__snspowderreduction_inner')
        DeleteWorkspace(Workspace='meta')

        duration = {}
        for name in (self.wksp_make, self.wksp_use):
            time_start = time.time()
            AlignAndFocusPowderFromFiles(Filename=self.data_file, OutputWorkspace=name,
                                         CacheDir=self.cacheDir,
                                         GroupingWorkspace='PG3_group', CalibrationWorkspace='PG3_cal',
                                         MaskWorkspace='PG3_mask',
                                         Params=-.0002, CompressTolerance=0.01,
                                         PrimaryFlightPath=60, SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0',
                                         ReductionProperties='__snspowderreduction_inner')
            duration[name] = time.time() - time_start
            NormaliseByCurrent(InputWorkspace=name, OutputWorkspace=name)
            ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target='dSpacing')

        # verify that the second time is faster
        self.assertLessThan(duration[self.wksp_use], duration[self.wksp_make],
                            'Should have been faster with cache {} > {}'.format(duration[self.wksp_use], duration[self.wksp_make]))

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return (self.wksp_make, self.wksp_use)


class DifferentGrouping(systemtesting.MantidSystemTest):
    cal_file  = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = 'PG3_9829_event.nxs'

    def cleanup(self):
        return True  # nothing to cleanup

    def requiredMemoryMB(self):
        return 3*1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        self.cacheDir = getCacheDir()

        PDLoadCharacterizations(Filename=self.char_file, OutputWorkspace='characterizations',
                                SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0')

        # first pass will create the cache file, second will use it
        wksp_single = 'PG3_9829_single'
        wksp_banks = 'PG3_9829_banks'
        grp_banks = 'PG3_banks'

        # load then process
        LoadEventNexus(Filename=self.data_file, OutputWorkspace='meta', MetaDataOnly=True)
        LoadDiffCal(Filename=self.cal_file, InputWorkspace='meta', WorkspaceName='PG3')
        PDDetermineCharacterizations(InputWorkspace='meta', Characterizations='characterizations',
                                     ReductionProperties='__snspowderreduction_inner')
        CreateGroupingWorkspace(InputWorkspace='meta', OutputWorkspace=grp_banks, GroupDetectorsBy='bank')
        DeleteWorkspace(Workspace='meta')

        for name, grouping in zip((wksp_single, wksp_banks), ('PG3_group', grp_banks)):
            print('processing {} with {}'.format(name, grouping))
            AlignAndFocusPowderFromFiles(Filename=self.data_file, OutputWorkspace=name,
                                         GroupingWorkspace=grouping, CalibrationWorkspace='PG3_cal',
                                         MaskWorkspace='PG3_mask',
                                         Params=-.0002, CompressTolerance=0.01,
                                         PrimaryFlightPath=60, SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0',
                                         ReductionProperties='__snspowderreduction_inner')
            NormaliseByCurrent(InputWorkspace=name, OutputWorkspace=name)
            ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target='dSpacing')

        self.assertEqual(mtd[wksp_single].getNumberHistograms(), 1, 'focusing into single spectrum')
        self.assertEqual(mtd[wksp_banks].getNumberHistograms(), 23,
                         'focusing into detector banks {}!={}'.format(mtd[wksp_banks].getNumberHistograms(), 23))

    def validate(self):
        pass


class AbsorptionCompare(systemtesting.MantidSystemTest):
    cal_file  = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = 'PG3_4866_event.nxs'

    def cleanup(self):
        return True

    def requiredMemoryMB(self):
        return 3*1024  # GiB

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        PDLoadCharacterizations(Filename=self.char_file, OutputWorkspace='characterizations',
                                SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0')

        self.wksp_mem = os.path.basename(self.data_file).split('.')[0]
        self.wksp_mem, self.wksp_file = self.wksp_mem + '_mem', self.wksp_mem + '_file'

        # load then process
        LoadEventAndCompress(Filename=self.data_file, OutputWorkspace=self.wksp_mem, MaxChunkSize=16, FilterBadPulses=0)
        LoadDiffCal(Filename=self.cal_file, InputWorkspace=self.wksp_mem, WorkspaceName='PG3')
        PDDetermineCharacterizations(InputWorkspace=self.wksp_mem, Characterizations='characterizations',
                                     ReductionProperties='__snspowderreduction_inner')

        # set-up the absorption calculation
        num_wl_bins = 200
        prop_manager = PropertyManagerDataService.retrieve('__snspowderreduction_inner')
        wl_min, wl_max = prop_manager['wavelength_min'].value, prop_manager['wavelength_max'].value  # 0.05, 2.20
        absorptionWS = WorkspaceFactory.create(mtd[self.wksp_mem],
                                               NVectors=mtd[self.wksp_mem].getNumberHistograms(), XLength=num_wl_bins+1,
                                               YLength=num_wl_bins)
        xaxis = np.arange(0., float(num_wl_bins + 1)) * (wl_max - wl_min) / (num_wl_bins) + wl_min
        for i in range(absorptionWS.getNumberHistograms()):
            absorptionWS.setX(i, xaxis)
        absorptionWS.getAxis(0).setUnit('Wavelength')
        mantid.api.AnalysisDataService.addOrReplace('V_abs', absorptionWS)
        SetSample(InputWorkspace='V_abs',
                  Material={'ChemicalFormula': 'V', 'SampleNumberDensity': 0.0721},
                  Geometry={'Shape': 'Cylinder', 'Height': 6.97, 'Radius': (0.63 / 2), 'Center': [0., 0., 0.]})
        self.assertEqual(absorptionWS.getNumberBins(), num_wl_bins)
        # calculate the absorption
        CylinderAbsorption(InputWorkspace='V_abs', OutputWorkspace='V_abs',
                           NumberOfSlices=20, NumberOfAnnuli=3)

        # do the work in memory
        ConvertUnits(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, Target='Wavelength')
        Divide(LHSWorkspace=self.wksp_mem, RHSWorkspace='V_abs', OutputWorkspace=self.wksp_mem)
        ConvertUnits(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, Target='TOF')
        AlignAndFocusPowder(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem,
                            GroupingWorkspace='PG3_group', CalibrationWorkspace='PG3_cal', MaskWorkspace='PG3_mask',
                            Params=-.0002, CompressTolerance=0.01,
                            PrimaryFlightPath=60, SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0',
                            ReductionProperties='__snspowderreduction_inner')
        NormaliseByCurrent(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem)
        ConvertUnits(InputWorkspace=self.wksp_mem, OutputWorkspace=self.wksp_mem, Target='dSpacing')

        # everything inside the algorithm
        AlignAndFocusPowderFromFiles(Filename=self.data_file, OutputWorkspace=self.wksp_file,
                                     GroupingWorkspace='PG3_group', CalibrationWorkspace='PG3_cal',
                                     MaskWorkspace='PG3_mask',
                                     AbsorptionWorkspace='V_abs',
                                     Params=-.0002, CompressTolerance=0.01,
                                     PrimaryFlightPath=60, SpectrumIDs='1', L2='3.18', Polar='90', Azimuthal='0',
                                     ReductionProperties='__snspowderreduction_inner')
        NormaliseByCurrent(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file)
        ConvertUnits(InputWorkspace=self.wksp_file, OutputWorkspace=self.wksp_file, Target='dSpacing')

    def validateMethod(self):
        self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return (self.wksp_mem, self.wksp_file)
