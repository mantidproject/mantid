# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid import mtd
from mantid.simpleapi import (config, CropWorkspace, DeleteWorkspace, DirectILLApplySelfShielding, DirectILLCollectData,
                              DirectILLDiagnostics, DirectILLIntegrateVanadium, DirectILLReduction, DirectILLSelfShielding,
                              DirectILLTubeBackground, SetSample, Subtract, Load)
import systemtesting
from testhelpers import (assertRaisesNothing, create_algorithm)


class IN4(systemtesting.MantidSystemTest):

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
            SubalgorithmLogging='Logging ON',
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
        CropWorkspace(
            InputWorkspace='SofQW',
            OutputWorkspace='cropped',
            XMin=1.,
            XMax=2.75,
            StartWorkspaceIndex=83,
            EndWorkspaceIndex=222)
        # The 'run_title' property has been deliberately erased from the test numor.
        # We need to add it manually because Save/LoadNexus will do the same for
        # the reference file.
        mtd['cropped'].mutableRun().addProperty('run_title', '', True)

    def validate(self):
        self.tolerance_is_rel_err = True
        self.tolerance = 1e-4
        return ['cropped', 'ILL_IN4_SofQW.nxs']


class IN5(systemtesting.MantidSystemTest):

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
        CropWorkspace(
            InputWorkspace='SofQW',
            OutputWorkspace='cropped',
            XMin=0.5,
            XMax=2.1,
            StartWorkspaceIndex=375,
            EndWorkspaceIndex=720)
        # The 'run_title' property has been deliberately erased from the test numor.
        # We need to add it manually because Save/LoadNexus will do the same for
        # the reference file.
        mtd['cropped'].mutableRun().addProperty('run_title', '', True)

    def validate(self):
        self.tolerance = 1e-7
        self.tolerance_is_rel_err = True
        return ['cropped', 'ILL_IN5_SofQW.nxs']


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
        CropWorkspace(
            InputWorkspace='SofQW',
            OutputWorkspace='cropped',
            XMin=0.7,
            XMax=2.1,
            StartWorkspaceIndex=9588,
            EndWorkspaceIndex=10280)
        # GetEiMonDet produces slightly different results on different system. We better
        # check the calibrated energy logs here and set them to fixed values to
        # enable CompareWorkspaces to do its job. This hack could be removed later if
        # CompareWorkspaces supported tolerances for sample log comparison.
        ws = mtd['cropped']
        run = ws.mutableRun()
        self.assertAlmostEqual(run.getProperty('Ei').value, 4.72233, places=4)
        run.addProperty('Ei', 4.72, True)
        self.assertAlmostEqual(run.getProperty('wavelength').value, 4.16207, places=4)
        run.addProperty('wavelength', 4.16, True)
        # The 'run_title' property has been deliberately erased from the test numor.
        # We need to add it manually because Save/LoadNexus will do the same for
        # the reference file.
        mtd['cropped'].mutableRun().addProperty('run_title', '', True)

    def validate(self):
        self.tolerance = 1e-2
        self.tolerance_is_rel_err = True
        return ['cropped', 'ILL_IN6_SofQW.nxs']


class IN5_Tube_Background(systemtesting.MantidSystemTest):

    def runTest(self):
        Load(Filename='ILL/IN5/Epp.nxs', OutputWorkspace='Epp')
        Load(Filename='ILL/IN5/Sample.nxs', OutputWorkspace='Sample')
        Load(Filename='ILL/IN5/Vmask.nxs', OutputWorkspace='Vmask')
        args = {'InputWorkspace':'Sample',
                'DiagnosticsWorkspace':'Vmask',
                'EPPWorkspace':'Epp',
                'OutputWorkspace':'Flat'}
        alg = create_algorithm('DirectILLTubeBackground', **args)
        assertRaisesNothing(self, alg.execute)
