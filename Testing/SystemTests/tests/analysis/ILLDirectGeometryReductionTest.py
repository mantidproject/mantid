# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (config, DeleteWorkspace, DirectILLApplySelfShielding, DirectILLCollectData, DirectILLDiagnostics,
                              DirectILLIntegrateVanadium, DirectILLReduction, DirectILLSelfShielding, DirectILLTubeBackground,
                              SetSample, Subtract)
import systemtesting


class IN4(systemtesting.MantidSystemTest):
    # Sample in reference is not empty string but contains a single space: ' '.
    disableChecking = ['Sample']
    tolerance = 1e-5

    def runTest(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN4'
        DirectILLCollectData(
            Run='ILL/IN4/085801-085802.nxs',
            OutputWorkspace='vanadium',
            OutputEPPWorkspace='vanadium-epps',
            OutputRawWorkspace='vanadium-raw')
        DirectILLIntegrateVanadium(
            InputWorkspace='vanadium',
            OutputWorkspace='integrated',
            EPPWorkspace='vanadium-epps')
        DirectILLDiagnostics(
            InputWorkspace='vanadium-raw',
            OutputWorkspace='diagnostics',
            EPPWorkspace='vanadium-epps',)
        # Samples
        DirectILLCollectData(
            Run='ILL/IN4/087294+087295.nxs',
            OutputWorkspace='sample',
            OutputIncidentEnergyWorkspace='Ei',
            OutputElasticChannelWorkspace='Elp')
        # Containers
        DirectILLCollectData(
            Run='ILL/IN4/087306-087309.nxs',
            OutputWorkspace='container',
            IncidentEnergyWorkspace='Ei',
            ElasticChannelWorkspace='Elp')
        geometry = {
            'Shape': 'HollowCylinder',
            'Height': 4.0,
            'InnerRadius': 1.9,
            'OuterRadius': 2.0,
            'Center': [0.0, 0.0, 0.0]}
        material = {
            'ChemicalFormula': 'Cd S',
            'SampleNumberDensity': 0.01}
        SetSample('sample', geometry, material)
        DirectILLSelfShielding(
            InputWorkspace='sample',
            OutputWorkspace='corrections',
            NumberOfSimulatedWavelengths=10)
        DirectILLApplySelfShielding(
            InputWorkspace='sample',
            OutputWorkspace='corrected',
            SelfShieldingCorrectionWorkspace='corrections',
            EmptyContainerWorkspace='container')
        DirectILLReduction(
            InputWorkspace='corrected',
            OutputWorkspace='SofQW',
            IntegratedVanadiumWorkspace='integrated',
            DiagnosticsWorkspace='diagnostics')

    def validate(self):
        return ['SofQW', 'ILL_IN4_SofQW.nxs']


class IN5(systemtesting.MantidSystemTest):
    # Sample in reference is not empty string but contains a single space: ' '.
    disableChecking = ['Sample']
    tolerance = 1e-5

    def runTest(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN5'
        DirectILLCollectData(
            Run='ILL/IN5/095893.nxs',
            OutputWorkspace='vanadium',
            OutputEPPWorkspace='vanadium-epps',)
        DirectILLDiagnostics(
            InputWorkspace='vanadium',
            OutputWorkspace='diagnostics',
            MaskedComponents='tube_320')
        DirectILLTubeBackground(
            InputWorkspace='vanadium',
            OutputWorkspace='bkg',
            EPPWorkspace='vanadium-epps',
            DiagnosticsWorkspace='diagnostics')
        Subtract(
            LHSWorkspace='vanadium',
            RHSWorkspace='bkg',
            OutputWorkspace='vanadium')
        DeleteWorkspace('bkg')
        DirectILLIntegrateVanadium(
            InputWorkspace='vanadium',
            OutputWorkspace='integrated',
            EPPWorkspace='vanadium-epps')
        DeleteWorkspace('vanadium')
        DeleteWorkspace('vanadium-epps')
        DirectILLCollectData(
            Run='ILL/IN5/096003.nxs',
            OutputWorkspace='sample',
            OutputEppWorkspace='epps')
        DirectILLTubeBackground(
            InputWorkspace='sample',
            OutputWorkspace='bkg',
            EPPWorkspace='epps',
            DiagnosticsWorkspace='diagnostics')
        Subtract(
            LHSWorkspace='sample',
            RHSWorkspace='bkg',
            OutputWorkspace='sample')
        DirectILLReduction(
            InputWorkspace='sample',
            OutputWorkspace='SofQW',
            IntegratedVanadiumWorkspace='integrated',
            DiagnosticsWorkspace='diagnostics')

    def validate(self):
        return ['SofQW', 'ILL_IN5_SofQW.nxs']


class IN6(systemtesting.MantidSystemTest):
    def runTest(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN6'
        DirectILLCollectData(
            Run='ILL/IN6/164192.nxs',
            OutputWorkspace='vanadium',
            OutputEPPWorkspace='vanadium-epps',
            OutputRawWorkspace='vanadium-raw')
        DirectILLIntegrateVanadium(
            InputWorkspace='vanadium',
            OutputWorkspace='integrated',
            EPPWorkspace='vanadium-epps')
        DirectILLDiagnostics(
            InputWorkspace='vanadium-raw',
            OutputWorkspace='diagnostics',
            EPPWorkspace='vanadium-epps',)
        # Simulate sample with vanadium
        DirectILLCollectData(
            Run='ILL/IN6/164192.nxs',
            OutputWorkspace='sample')
        DirectILLReduction(
            InputWorkspace='sample',
            OutputWorkspace='SofQW',
            IntegratedVanadiumWorkspace='integrated',
            EnergyRebinningParams='-100, 0.01, 4.',
            DiagnosticsWorkspace='diagnostics')

    def validate(self):
        return ['SofQW', 'ILL_IN6_SofQW.nxs']
