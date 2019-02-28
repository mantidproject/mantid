# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod
import os
from six import with_metaclass

import systemtesting
from mantid.simpleapi import *
from mantid import config
from mantid.dataobjects import MaskWorkspace


def create_file_path(base_name):
    temp_save_dir = config['defaultsave.directory']
    if temp_save_dir == '':
        temp_save_dir = os.getcwd()
    filename = os.path.join(temp_save_dir, base_name + '.nxs')
    return filename


class SaveLoadNexusProcessedTestBase(with_metaclass(ABCMeta, systemtesting.MantidSystemTest)):

    filename = None
    test_ws_name = 'input_ws'
    loaded_ws_name = 'loaded'

    @abstractmethod
    def createTestWorkspace(self):
        raise NotImplementedError("Derived tests should implement createTestWorkspace(self)")

    @abstractmethod
    def savedFilename(self):
        raise NotImplementedError("Derived tests should implement savedFilename(self)")

    def runTest(self):
        self.createTestWorkspace()
        self.filename = create_file_path(self.savedFilename())
        SaveNexusProcessed(InputWorkspace=self.test_ws_name, Filename=self.filename)
        LoadNexusProcessed(Filename=self.filename, OutputWorkspace=self.loaded_ws_name)

    def compareWorkspaces(self):
        result = CompareWorkspaces(self.test_ws_name, self.loaded_ws_name, CheckInstrument=False, CheckSample=True)
        return result[0]

    def validate(self):
        return self.compareWorkspaces()

    def cleanup(self):
        if self.filename is None:
            print("self.filename unset by derived test")
        else:
            try:
                os.remove(self.filename)
            except OSError as ex:
                # can't really do much
                print("Error removing {}: {}".format(str(ex), filename))


class SaveLoadNexusProcessedBasicTest(SaveLoadNexusProcessedTestBase):

    def createTestWorkspace(self):
        CreateSampleWorkspace(OutputWorkspace=self.test_ws_name, Function='Flat background',
                              BankPixelWidth=1, XMax=15,
                              BinWidth=0.75, NumBanks=4)
        MaskBins(InputWorkspace=self.test_ws_name, OutputWorkspace=self.test_ws_name,
                 XMin=0, XMax=2, SpectraList='0')
        MaskBins(InputWorkspace=self.test_ws_name, OutputWorkspace=self.test_ws_name,
                 XMin=0, XMax=1.5, SpectraList='1')

    def savedFilename(self):
        return 'tmp_saveload_nexusprocessed_maskbins'


class SaveLoadNexusProcessedMaskWorkspaceTest(SaveLoadNexusProcessedTestBase):

    def createTestWorkspace(self):
        CreateSampleWorkspace(OutputWorkspace=self.test_ws_name, Function='Flat background',
                              BankPixelWidth=1, XMax=15,
                              BinWidth=0.75, NumBanks=4)
        MaskDetectors(Workspace=self.test_ws_name, WorkspaceIndexList=[1, 3])
        ExtractMask(InputWorkspace=self.test_ws_name, OutputWorkspace=self.test_ws_name)

    def savedFilename(self):
        return 'tmp_saveload_nexusprocessed_maskworkspace'

    def validate(self):
        loaded_ws = AnalysisDataService.Instance()[self.loaded_ws_name]
        self.assertTrue(isinstance(loaded_ws, MaskWorkspace))
        return self.compareWorkspaces()


class SaveLoadNexusProcessedEmptySampleNameTest(SaveLoadNexusProcessedTestBase):

    def createTestWorkspace(self):
        CreateSampleWorkspace(OutputWorkspace=self.test_ws_name,
                              BankPixelWidth=1, XMax=15.,
                              BinWidth=15., NumBanks=1)
        assert(mtd[self.test_ws_name].sample().getName() == '')

    def savedFilename(self):
        return 'tmp_saveload_nexusprocessed_emptysamplename'

    def validate(self):
        return self.compareWorkspaces()


class SaveLoadNexusProcessedNoDetectorsSpectraNumbersTest(SaveLoadNexusProcessedTestBase):

    def createTestWorkspace(self):
        ws = CreateWorkspace([0.], [-1., -2., -3.], NSpec=3, StoreInADS=False)
        ExtractSingleSpectrum(ws, 1, OutputWorkspace=self.test_ws_name)

    def savedFilename(self):
        return 'tmp_saveload_nexusprocessed_nodetectorsspectranumbers'

    def validate(self):
        return self.compareWorkspaces()
