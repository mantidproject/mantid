# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import *


class ILL_SANS_D11_MONO_TEST(systemtesting.MantidSystemTest):
    '''
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the old D11
    '''

    def __init__(self):
        super(ILL_SANS_D11_MONO_TEST, self).__init__()
        self.setUp()
        self.facility = config['default.facility']
        self.instrument = config['default.instrument']
        self.directories = config['datasearch.directories']

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'
        config.appendDataSearchSubDir('ILL/D11/')

    def tearDown(self):
        mtd.clear()
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['out', 'ILL_SANS_D11_MONO.nxs']

    def runTest(self):
        # Load a pre-drawn mask
        LoadNexusProcessed(Filename='D11_mask.nxs',
                           OutputWorkspace='mask')
        # First reduce the water measured at 8m
        SANSILLReduction(Runs='010455',
                         ProcessAs='DarkCurrent',
                         OutputWorkspace='cad')
        # Process the empty beam for water
        SANSILLReduction(Runs='010414',
                         ProcessAs='EmptyBeam',
                         DarkCurrentWorkspace='cad',
                         OutputWorkspace='mt',
                         OutputFluxWorkspace='flux')
        # Water container transmission
        SANSILLReduction(Runs='010446',
                         ProcessAs='Transmission',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         FluxWorkspace='flux',
                         OutputWorkspace='wc_tr')
        # Water container
        SANSILLReduction(Runs='010454',
                         ProcessAs='EmptyContainer',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         TransmissionWorkspace='wc_tr',
                         OutputWorkspace='wc')
        # Water transmission
        SANSILLReduction(Runs='010445',
                         ProcessAs='Transmission',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         FluxWorkspace='flux',
                         OutputWorkspace='w_tr')
        # Water
        SANSILLReduction(Runs='010453',
                         ProcessAs='Water',
                         DarkCurrentWorkspace='cad',
                         MaskWorkspace='mask',
                         EmptyBeamWorkspace='mt',
                         EmptyContainerWorkspace='wc',
                         TransmissionWorkspace='w_tr',
                         OutputSensitivityWorkspace='sens',
                         FluxWorkspace='flux',
                         OutputWorkspace='water')
        # The sample is measured at 20m, its transmission is measured at 8m
        # Measure the transmissions next
        # Sample container transmission
        SANSILLReduction(Runs='010444',
                         ProcessAs='Transmission',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         FluxWorkspace='flux',
                         OutputWorkspace='sc_tr')
        # Sample transmission
        SANSILLReduction(Runs='010585',
                         ProcessAs='Transmission',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         OutputWorkspace='s_tr',
                         FluxWorkspace='flux')
        # Reduce the sample
        # Process the dark current Cd/B4C for sample
        SANSILLReduction(Runs='010462',
                         ProcessAs='DarkCurrent',
                         OutputWorkspace='scad')
        # Process the empty beam for sample
        SANSILLReduction(Runs='010413',
                         ProcessAs='EmptyBeam',
                         DarkCurrentWorkspace='scad',
                         OutputWorkspace='smt',
                         OutputFluxWorkspace='sflux')
        # Sample container
        SANSILLReduction(Runs='010460',
                         ProcessAs='EmptyContainer',
                         DarkCurrentWorkspace='scad',
                         EmptyBeamWorkspace='smt',
                         TransmissionWorkspace='sc_tr',
                         OutputWorkspace='sc')
        # Sample with flux and sensitivity
        SANSILLReduction(Runs='010569',
                         ProcessAs='Sample',
                         DarkCurrentWorkspace='scad',
                         EmptyContainerWorkspace='sc',
                         EmptyBeamWorkspace='smt',
                         SensitivityWorkspace='sens',
                         MaskWorkspace='mask',
                         TransmissionWorkspace='s_tr',
                         OutputWorkspace='sample_sens',
                         FluxWorkspace='sflux')
        # Sample with flux and water normalisation
        SANSILLReduction(Runs='010569',
                         ProcessAs='Sample',
                         DarkCurrentWorkspace='scad',
                         EmptyContainerWorkspace='sc',
                         EmptyBeamWorkspace='smt',
                         FlatFieldWorkspace='water',
                         MaskWorkspace='mask',
                         TransmissionWorkspace='s_tr',
                         OutputWorkspace='sample_water',
                         FluxWorkspace='sflux')
        GroupWorkspaces(InputWorkspaces=['water', 'sens', 'sample_sens', 'sample_water'],
                        OutputWorkspace='out')


class ILL_SANS_D22_MONO_TEST(systemtesting.MantidSystemTest):
    '''
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the old D22
    '''

    def __init__(self):
        super(ILL_SANS_D22_MONO_TEST, self).__init__()
        self.setUp()
        self.facility = config['default.facility']
        self.instrument = config['default.instrument']
        self.directories = config['datasearch.directories']

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D22'
        config.appendDataSearchSubDir('ILL/D22/')

    def tearDown(self):
        mtd.clear()
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['out', 'ILL_SANS_D22_MONO.nxs']

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename='D22_mask.nxs',
                           OutputWorkspace='mask')

        # Absorber
        SANSILLReduction(Runs='241238',
                         ProcessAs='DarkCurrent',
                         OutputWorkspace='cad')

        # Beam
        SANSILLReduction(Runs='241226',
                         ProcessAs='EmptyBeam',
                         DarkCurrentWorkspace='cad',
                         OutputWorkspace='mt',
                         OutputFluxWorkspace='fl')

        # Container transmission known
        CreateSingleValuedWorkspace(DataValue=0.94638, ErrorValue=0.0010425, OutputWorkspace='ctr')
        AddSampleLog(Workspace='ctr', LogName='ProcessedAs', LogText='Transmission')
        AddSampleLog(Workspace='ctr', LogName='wavelength', LogText='6.0', LogType='Number', LogUnit='Angstrom')

        # Container
        SANSILLReduction(Runs='241239',
                         ProcessAs='EmptyContainer',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         TransmissionWorkspace='ctr',
                         OutputWorkspace='can')

        # Sample transmission known
        CreateSingleValuedWorkspace(DataValue=0.52163, ErrorValue=0.00090538, OutputWorkspace='str')
        AddSampleLog(Workspace='str', LogName='ProcessedAs', LogText='Transmission')
        AddSampleLog(Workspace='str', LogName='wavelength', LogText='6.0', LogType='Number', LogUnit='Angstrom')

        # Sample
        SANSILLReduction(Runs='241240',
                         ProcessAs='Sample',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         TransmissionWorkspace='str',
                         EmptyContainerWorkspace='can',
                         MaskWorkspace='mask',
                         FluxWorkspace='fl',
                         OutputWorkspace='out')


class ILL_SANS_D22_MULTISENS(systemtesting.MantidSystemTest):
    '''
    Tests a standard monochromatic reduction with the v2 of the algorithm and data from the old D22
    Tests creation of a sensitivity map without a shadow using 2 water measurements: with and w/o offset
    '''

    def __init__(self):
        super(ILL_SANS_D22_MULTISENS, self).__init__()
        self.setUp()
        self.facility = config['default.facility']
        self.instrument = config['default.instrument']
        self.directories = config['datasearch.directories']

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D22'
        config.appendDataSearchSubDir('ILL/D22/')

    def tearDown(self):
        mtd.clear()
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.directories

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['outputs', 'ILL_SANS_D22_MULTISENS.nxs']

    def runTest(self):
        # create necessary masks:
        MaskBTP(Instrument='D22', Pixel='0-12,245-255')
        RenameWorkspace(InputWorkspace='D22MaskBTP', OutputWorkspace='top_bottom')
        MaskBTP(Instrument='D22', Tube='10-31', Pixel='105-150')
        Plus(LHSWorkspace='top_bottom', RHSWorkspace='D22MaskBTP', OutputWorkspace='mask_offset')
        MaskBTP(Instrument='D22', Tube='54-75', Pixel='108-150')
        Plus(LHSWorkspace='top_bottom', RHSWorkspace='D22MaskBTP', OutputWorkspace='mask_central')

        # Load the mask
        LoadNexusProcessed(Filename='D22_mask.nxs',
                           OutputWorkspace='mask')

        # Absorber
        SANSILLReduction(Runs='241238',
                         ProcessAs='DarkCurrent',
                         OutputWorkspace='cad')

        # Beam
        SANSILLReduction(Runs='241226',
                         ProcessAs='EmptyBeam',
                         DarkCurrentWorkspace='cad',
                         OutputWorkspace='mt',
                         OutputFluxWorkspace='fl')

        # Container transmission known
        CreateSingleValuedWorkspace(DataValue=0.94638, ErrorValue=0.0010425, OutputWorkspace='ctr')
        AddSampleLog(Workspace='ctr', LogName='ProcessedAs', LogText='Transmission')
        AddSampleLog(Workspace='ctr', LogName='wavelength', LogText='6.0', LogType='Number', LogUnit='Ansgrom')

        # Container
        SANSILLReduction(Runs='241239',
                         ProcessAs='EmptyContainer',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         TransmissionWorkspace='ctr',
                         OutputWorkspace='can')

        # Water Reference
        SANSILLReduction(Runs='344411',
                         ProcessAs='Water',
                         MaskWorkspace='mask_central',
                         OutputWorkspace='ref1',
                         OutputSensitivityWorkspace='sens1')

        SANSILLReduction(Runs='344407',
                         ProcessAs='Water',
                         MaskWorkspace='mask_offset',
                         OutputWorkspace='ref2',
                         OutputSensitivityWorkspace='sens2')

        GroupWorkspaces(InputWorkspaces=['ref1', 'ref2'], OutputWorkspace='sensitivity_input')
        CalculateEfficiency(InputWorkspace='sensitivity_input', MergeGroup=True, OutputWorkspace='sens')

        # Sample transmission known
        CreateSingleValuedWorkspace(DataValue=0.52163, ErrorValue=0.00090538, OutputWorkspace='str')
        AddSampleLog(Workspace='str', LogName='ProcessedAs', LogText='Transmission')
        AddSampleLog(Workspace='str', LogName='wavelength', LogText='6.0', LogType='Number', LogUnit='Ansgrom')

        # Sample
        SANSILLReduction(Runs='241240',
                         ProcessAs='Sample',
                         DarkCurrentWorkspace='cad',
                         EmptyBeamWorkspace='mt',
                         TransmissionWorkspace='str',
                         EmptyContainerWorkspace='can',
                         MaskWorkspace='mask',
                         SensitivityWorkspace='sens',
                         OutputWorkspace='out',
                         FluxWorkspace='fl')

        GroupWorkspaces(InputWorkspaces=['out', 'sens', 'ref1', 'ref2'],
                        OutputWorkspace='outputs')
