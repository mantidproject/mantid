# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import stresstesting
from mantid.simpleapi import *


# A dummy test class to subclass from.
# Sets up the facility and data search directories.
class _DiffReductionTest(stresstesting.MantidStressTest):

    _facility = ''
    _directories = ''
    _instrument = ''
    _tolerance = 0.00001

    def __init__(self):
        super(_DiffReductionTest, self).__init__()
        self.setUp()

    def setUp(self):
        self._facility = config['default.facility']
        self._instrument = config['default.instrument']
        self._directories = config['datasearch.directories']
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D2B'
        config.appendDataSearchSubDir('ILL/D2B/')
        config.appendDataSearchSubDir('ILL/D20/')

    def tearDown(self):
        config['default.facility'] = self._facility
        config['default.instrument'] = self._instrument
        config['datasearch.directories'] = self._directories
        mtd.clear()

    def runTest(self):
        pass


class D2B_2DTubes_ReductionTest(_DiffReductionTest):

    def runTest(self):
        PowderDiffILLDetScanReduction(Run='508093:508095', Output2DTubes = True, Output1D = False, OutputWorkspace='out')

    def validate(self):
        return ['out_2DTubes', 'D2B_2DTubes.nxs']


class D2B_2D_ReductionTest(_DiffReductionTest):

    def runTest(self):
        PowderDiffILLDetScanReduction(Run='508093:508095', Output2D = True, Output1D = False, OutputWorkspace='out')

    def validate(self):
        return ['out_2D', 'D2B_2D.nxs']


class D2B_1D_ReductionTest(_DiffReductionTest):

    def runTest(self):
        PowderDiffILLDetScanReduction(Run='508093:508095', OutputWorkspace='out')

    def validate(self):
        return ['out_1D', 'D2B_1D.nxs']


class D2B_LowCounts_ReductionTest(_DiffReductionTest):

    def runTest(self):
        PowderDiffILLDetScanReduction(Run='540162', OutputWorkspace='out')

    def validate(self):
        return ['out_1D', 'D2B_LowCounts.nxs']


class D20_NoMask_ReductionTest(_DiffReductionTest):

    def runTest(self):
        PowderDiffILLDetScanReduction(Run='967076', NormaliseTo='None', CropNegativeScatteringAngles=False,
                                      UseCalibratedData=False, InitialMask=0, OutputWorkspace='out')

    def validate(self):
        return ['out_1D', 'D20_NoMask.nxs']


class D20_Mask_ReductionTest(_DiffReductionTest):

    def runTest(self):
        ws = LoadILLDiffraction(Filename='967076')
        MaskDetectors(ws, DetectorList='0-63')
        SumOverlappingTubes(InputWorkspaces=ws, OutputType='1D', OutputWorkspace='red')

    def validate(self):
        return ['red', 'D20_Mask.nxs']


class D2B_Component_ReductionTest(_DiffReductionTest):

    def runTest(self):
        PowderDiffILLDetScanReduction(Run='508093',
                                      UseCalibratedData=False,
                                      Output2DTubes=True, Output1D=False,
                                      CropNegativeScatteringAngles=False,
                                      OutputWorkspace='alltubes',
                                      InitialMask='0', FinalMask='0')
        PowderDiffILLDetScanReduction(Run='508093',
                                      UseCalibratedData=False,
                                      Output2DTubes=True, Output1D=False,
                                      CropNegativeScatteringAngles=False,
                                      OutputWorkspace='tube128',
                                      InitialMask='0', FinalMask='0',
                                      ComponentsToReduce='tube_128')
        CropWorkspace(InputWorkspace='alltubes_2DTubes', OutputWorkspace='alltubes_tube128', XMin=147.471)
        match = CompareWorkspaces(Workspace1='tube128_2DTubes', Workspace2='alltubes_tube128', Tolerance=self._tolerance)
        self.assertTrue(match[0])


class D2B_HeightRange_ReductionTest(_DiffReductionTest):

    def runTest(self):
        PowderDiffILLDetScanReduction(Run='508093:508095', Output2D=True, Output1D=False, InitialMask=0, FinalMask=0,
                                      UseCalibratedData= False, NormaliseTo='None', OutputWorkspace='out')
        PowderDiffILLDetScanReduction(Run='508093:508095', Output2D=True, Output1D=False, InitialMask=0, FinalMask=0,
                                      UseCalibratedData= False, NormaliseTo='None', OutputWorkspace='out_height',
                                      HeightRange='-0.05,0.05')
        ExtractSpectra(InputWorkspace='out_2D', StartWorkspaceIndex=43, EndWorkspaceIndex=84, OutputWorkspace='cropped')
        match = CompareWorkspaces(Workspace1='cropped', Workspace2='out_height_2D', CheckSpectraMap=False, Tolerance=self._tolerance)
        self.assertTrue(match[0])


class D2B_Merge_ReductionTest(_DiffReductionTest):

    def runTest(self):
        PowderDiffILLDetScanReduction(Run='508093:508095', OutputWorkspace='out')
        PowderDiffILLDetScanReduction(Run='508093-508095', OutputWorkspace='sum')
        match = CompareWorkspaces(Workspace1='out_1D', Workspace2='sum_1D')
        self.assertTrue(match[0])
