# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import systemtesting
from mantid.simpleapi import ReflectometryILLAutoProcess, config, mtd


class D17Cycle192IncoherentSanTest(systemtesting.MantidSystemTest):
    """
        @brief Tests with VoS11 sample at 2 angles with the data from cycle #192
        Uses incoherent summation with sample angle option.
    """

    def __init__(self):
        super(D17Cycle192IncoherentSanTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D17'
        config['logging.loggers.root.level'] = 'Warning'
        config.appendDataSearchSubDir('ILL/D17/')

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument', 'Sample']
        return ['VoS11', 'D17_VoS11.nxs']

    def runTest(self):
        name = 'VoS11'
        directBeams = '541838,541839'
        reflectedBeams = '541882,541883'
        foregroundWidth = [3, 15]
        angleOffset = [2, 5]
        angleWidth = 5
        ReflectometryILLAutoProcess(
            Run=reflectedBeams,
            DirectRun=directBeams,
            OutputWorkspace=name,
            SummationType='Incoherent',
            AngleOption='SampleAngle',
            DeltaQFractionBinning=0.5,
            DirectLowAngleFrgHalfWidth=foregroundWidth,
            DirectHighAngleFrgHalfWidth=foregroundWidth,
            DirectLowAngleBkgOffset=angleOffset,
            DirectLowAngleBkgWidth=angleWidth,
            DirectHighAngleBkgOffset=angleOffset,
            DirectHighAngleBkgWidth=angleWidth,
            ReflLowAngleFrgHalfWidth=foregroundWidth,
            ReflHighAngleFrgHalfWidth=foregroundWidth,
            ReflLowAngleBkgOffset=angleOffset,
            ReflLowAngleBkgWidth=angleWidth,
            ReflHighAngleBkgOffset=angleOffset,
            ReflHighAngleBkgWidth=angleWidth,
            WavelengthLowerBound=[3., 3.],
            WavelengthUpperBound=[27., 25.],
            GlobalScaleFactor=0.13
        )


class D17Cycle192CoherentDanTest(systemtesting.MantidSystemTest):
    """
        @brief Tests with SiO2 sample at 2 angles with the data from cycle #192
        Uses coherent summation with detector angle option.
    """

    def __init__(self):
        super(D17Cycle192CoherentDanTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D17'
        config['logging.loggers.root.level'] = 'Warning'
        config.appendDataSearchSubDir('ILL/D17/')

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument', 'Sample']
        return ['SiO2', 'D17_SiO2.nxs']

    def runTest(self):
        name = 'SiO2'
        directBeams = '541838,541839'
        reflectedBeams = '541853,541854'
        foregroundWidth = [3, 15]
        angleOffset = [2, 5]
        angleWidth = 5
        ReflectometryILLAutoProcess(
            Run=reflectedBeams,
            DirectRun=directBeams,
            OutputWorkspace=name,
            SummationType='Coherent',
            AngleOption='DetectorAngle',
            DeltaQFractionBinning=0.5,
            DirectLowAngleFrgHalfWidth=foregroundWidth,
            DirectHighAngleFrgHalfWidth=foregroundWidth,
            DirectLowAngleBkgOffset=angleOffset,
            DirectLowAngleBkgWidth=angleWidth,
            DirectHighAngleBkgOffset=angleOffset,
            DirectHighAngleBkgWidth=angleWidth,
            ReflLowAngleFrgHalfWidth=foregroundWidth,
            ReflHighAngleFrgHalfWidth=foregroundWidth,
            ReflLowAngleBkgOffset=angleOffset,
            ReflLowAngleBkgWidth=angleWidth,
            ReflHighAngleBkgOffset=angleOffset,
            ReflHighAngleBkgWidth=angleWidth,
            WavelengthLowerBound=[3., 3.],
            WavelengthUpperBound=[27., 25.],
            GlobalScaleFactor=0.13
        )


class D17Cycle181RoundRobinTest(systemtesting.MantidSystemTest):
    """
        @brief Tests with RoundRobin sample at 3 angles with the data from cycle #181
        Uses incoherent summation with sample angle option.
    """

    def __init__(self):
        super(D17Cycle181RoundRobinTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D17'
        config['logging.loggers.root.level'] = 'Warning'
        config.appendDataSearchSubDir('ILL/D17/')

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument', 'Sample']
        return ['Thick_HR_5', 'D17_Thick_HR_5.nxs']

    def runTest(self):
        name = 'Thick_HR_5'
        directBeams = '397812,397806,397808'
        reflectedBeams = '397826+397827,397828,397829+397830+397831+397832'
        foregroundWidth = [4,5,8]
        wavelengthLower = [3., 1.6, 2.]
        wavelengthUpper = [27., 25., 25.]
        angleOffset = 2
        angleWidth = 10
        ReflectometryILLAutoProcess(
            Run=reflectedBeams,
            DirectRun=directBeams,
            OutputWorkspace=name,
            SummationType='Incoherent',
            AngleOption='SampleAngle',
            DirectLowAngleFrgHalfWidth=foregroundWidth,
            DirectHighAngleFrgHalfWidth=foregroundWidth,
            DirectLowAngleBkgOffset=angleOffset,
            DirectLowAngleBkgWidth=angleWidth,
            DirectHighAngleBkgOffset=angleOffset,
            DirectHighAngleBkgWidth=angleWidth,
            ReflLowAngleFrgHalfWidth=foregroundWidth,
            ReflHighAngleFrgHalfWidth=foregroundWidth,
            ReflLowAngleBkgOffset=angleOffset,
            ReflLowAngleBkgWidth=angleWidth,
            ReflHighAngleBkgOffset=angleOffset,
            ReflHighAngleBkgWidth=angleWidth,
            WavelengthLowerBound=wavelengthLower,
            WavelengthUpperBound=wavelengthUpper,
            DeltaQFractionBinning=0.5
        )
