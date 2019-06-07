# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import systemtesting
from mantid.simpleapi import *


class ILL_D11_Test(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILL_D11_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'
        config.appendDataSearchSubDir('ILL/D11/')

    def tearDown(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-5
        self.disableChecking = ['Instrument']
        return ['iq', 'ILL_SANS_D11_IQ.nxs']

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename='D11_mask.nxs', OutputWorkspace='mask')
        # Process the dark current Cd/B4C for water
        SANSILLReduction(Run='010455', ProcessAs='Absorber', OutputWorkspace='Cdw')
        # Process the empty beam for water
        SANSILLReduction(Run='010414', ProcessAs='Beam', AbsorberInputWorkspace='Cdw', OutputWorkspace='Dbw')
        # Water container transmission
        SANSILLReduction(Run='010446', ProcessAs='Transmission', AbsorberInputWorkspace='Cdw',
                         BeamInputWorkspace='Dbw', OutputWorkspace='wc_tr')
        # Water container
        SANSILLReduction(Run='010454', ProcessAs='Container', AbsorberInputWorkspace='Cdw',
                         BeamInputWorkspace='Dbw', TransmissionInputWorkspace='wc_tr', OutputWorkspace='wc')
        # Water transmission
        SANSILLReduction(Run='010445', ProcessAs='Transmission', AbsorberInputWorkspace='Cdw',
                         BeamInputWorkspace='Dbw', OutputWorkspace='w_tr')
        # Water
        SANSILLReduction(Run='010453', ProcessAs='Reference', AbsorberInputWorkspace='Cdw', MaskedInputWorkspace='mask',
                         ContainerInputWorkspace='wc', BeamInputWorkspace='Dbw', TransmissionInputWorkspace='wc_tr',
                         SensitivityOutputWorkspace='sens', OutputWorkspace='water')
        # Process the dark current Cd/B4C for sample
        SANSILLReduction(Run='010462', ProcessAs='Absorber', OutputWorkspace='Cd')
        # Process the empty beam for sample
        SANSILLReduction(Run='010413', ProcessAs='Beam', AbsorberInputWorkspace='Cd', OutputWorkspace='Db', FluxOutputWorkspace='fl')
        # Sample container transmission
        SANSILLReduction(Run='010444', ProcessAs='Transmission', AbsorberInputWorkspace='Cd',
                         BeamInputWorkspace='Dbw', OutputWorkspace='sc_tr')
        # Sample container
        SANSILLReduction(Run='010460', ProcessAs='Container', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db',
                         TransmissionInputWorkspace='sc_tr', OutputWorkspace='sc')
        # Sample transmission
        SANSILLReduction(Run='010585', ProcessAs='Transmission', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Dbw',
                         OutputWorkspace='s_tr')
        # Sample
        SANSILLReduction(Run='010569', ProcessAs='Sample', AbsorberInputWorkspace='Cd', ContainerInputWorkspace='sc',
                         BeamInputWorkspace='Db', SensitivityInputWorkspace='sens', MaskedInputWorkspace='mask',
                         TransmissionInputWorkspace='s_tr', OutputWorkspace='sample_flux', FluxInputWorkspace='fl')
        # Convert to I(Q)
        SANSILLIntegration(InputWorkspace='sample_flux', OutputWorkspace='iq')


class ILL_D22_Test(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILL_D22_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D22'
        config.appendDataSearchSubDir('ILL/D22/')

    def tearDown(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-5
        self.disableChecking = ['Instrument']
        return ['iq', 'ILL_SANS_D22_IQ.nxs']

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename='D22_mask.nxs', OutputWorkspace='mask')

        # Absorber
        SANSILLReduction(Run='241238', ProcessAs='Absorber', OutputWorkspace='Cd')

        # Beam
        SANSILLReduction(Run='241226', ProcessAs='Beam', AbsorberInputWorkspace='Cd', OutputWorkspace='Db', FluxOutputWorkspace='fl')

        # Container transmission known
        CreateSingleValuedWorkspace(DataValue=0.94638, ErrorValue=0.0010425, OutputWorkspace='ctr')
        AddSampleLog(Workspace='ctr', LogName='ProcessedAs', LogText='Transmission')

        # Container
        SANSILLReduction(Run='241239', ProcessAs='Container', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db',
                         TransmissionInputWorkspace='ctr', OutputWorkspace='can')

        # Sample transmission known
        CreateSingleValuedWorkspace(DataValue=0.52163, ErrorValue=0.00090538, OutputWorkspace='str')
        AddSampleLog(Workspace='str', LogName='ProcessedAs', LogText='Transmission')

        # Reference
        # Actually this is not water, but a deuterated buffer, but is fine for the test
        SANSILLReduction(Run='241261', ProcessAs='Reference', MaskedInputWorkspace='mask',
                         AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db', ContainerInputWorkspace='can',
                         OutputWorkspace='ref', SensitivityOutputWorkspace='sens')

        # remove the errors on the sensitivity, since they are too large because it is not water
        CreateWorkspace(DataX=mtd['sens'].extractX(), DataY=mtd['sens'].extractY(), NSpec=mtd['sens'].getNumberHistograms(),
                        OutputWorkspace='sens', ParentWorkspace='sens')
        AddSampleLog(Workspace='sens', LogName='ProcessedAs', LogText='Reference')

        # Sample
        SANSILLReduction(Run='241240', ProcessAs='Sample', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db',
                         TransmissionInputWorkspace='str', ContainerInputWorkspace='can', MaskedInputWorkspace='mask',
                         SensitivityInputWorkspace='sens', OutputWorkspace='sample', FluxInputWorkspace='fl')

        # Integration
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', CalculateResolution='None')


class ILL_D33_VTOF_Test(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILL_D33_VTOF_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D33'
        config.appendDataSearchSubDir('ILL/D33/')

    def tearDown(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-5
        self.disableChecking = ['Instrument']
        return ['iq', 'ILL_SANS_D33_VTOF_IQ.nxs']

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename='D33_mask.nxs', OutputWorkspace='mask')

        # Beam
        SANSILLReduction(Run='093406', ProcessAs='Beam', OutputWorkspace='beam', FluxOutputWorkspace='flux')

        # Container Transmission
        SANSILLReduction(Run='093407', ProcessAs='Transmission', BeamInputWorkspace='beam', OutputWorkspace='ctr')

        # Container
        SANSILLReduction(Run='093409', ProcessAs='Container', BeamInputWorkspace='beam',
                         TransmissionInputWorkspace='ctr', OutputWorkspace='can')

        # Sample transmission
        SANSILLReduction(Run='093408', ProcessAs='Transmission', BeamInputWorkspace='beam', OutputWorkspace='str')

        # Sample
        SANSILLReduction(Run='093410', ProcessAs='Sample', BeamInputWorkspace='beam', TransmissionInputWorkspace='str',
                         ContainerInputWorkspace='can', MaskedInputWorkspace='mask', OutputWorkspace='sample', FluxInputWorkspace='flux')
        # I(Q)
        SANSILLIntegration(InputWorkspace='sample', CalculateResolution='None', OutputBinning='0.005,-0.1,1',
                           OutputWorkspace='iq', BinMaskingCriteria='x<1 || x>10')


class ILL_D33_LTOF_Test(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILL_D33_LTOF_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D33'
        config.appendDataSearchSubDir('ILL/D33/')

    def tearDown(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-5
        self.disableChecking = ['Instrument']
        return ['iq', 'ILL_SANS_D33_LTOF_IQ.nxs']

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename='D33_mask.nxs', OutputWorkspace='mask')

        # Beam
        SANSILLReduction(Run='093411', ProcessAs='Beam', OutputWorkspace='beam', FluxOutputWorkspace='flux')

        # Container Transmission
        SANSILLReduction(Run='093412', ProcessAs='Transmission', BeamInputWorkspace='beam', OutputWorkspace='ctr')

        # Container
        SANSILLReduction(Run='093414', ProcessAs='Container', BeamInputWorkspace='beam',
                         TransmissionInputWorkspace='ctr', OutputWorkspace='can')

        # Sample Transmission
        SANSILLReduction(Run='093413', ProcessAs='Transmission', BeamInputWorkspace='beam', OutputWorkspace='str')

        # Sample
        SANSILLReduction(Run='093415', ProcessAs='Sample', BeamInputWorkspace='beam', TransmissionInputWorkspace='str',
                         ContainerInputWorkspace='can', MaskedInputWorkspace='mask', OutputWorkspace='sample', FluxInputWorkspace='flux')

        # I(Q)
        SANSILLIntegration(InputWorkspace='sample', CalculateResolution='None', OutputBinning='0.005,-0.1,1',
                           OutputWorkspace='iq', BinMaskingCriteria='x<1 || x>10')


class ILL_D33_Test(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILL_D33_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D33'
        config.appendDataSearchSubDir('ILL/D33/')

    def tearDown(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-5
        self.disableChecking = ['Instrument']
        return ['iq', 'ILL_SANS_D33_IQ.nxs']

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename='D33_mask.nxs', OutputWorkspace='mask')

        # Process the dark current Cd/B4C for water
        SANSILLReduction(Run='027885', ProcessAs='Absorber', OutputWorkspace='Cdw')

        # Process the empty beam for water
        SANSILLReduction(Run='027858', ProcessAs='Beam', AbsorberInputWorkspace='Cdw', OutputWorkspace='Dbw')

        # Water container transmission
        SANSILLReduction(Run='027860', ProcessAs='Transmission',
                         AbsorberInputWorkspace='Cdw', BeamInputWorkspace='Dbw',
                         OutputWorkspace='wc_tr')

        # Water container
        SANSILLReduction(Run='027886', ProcessAs='Container',
                         AbsorberInputWorkspace='Cdw', BeamInputWorkspace='Dbw',
                         TransmissionInputWorkspace='wc_tr', OutputWorkspace='wc')

        # Water transmission
        SANSILLReduction(Run='027861', ProcessAs='Transmission',
                         AbsorberInputWorkspace='Cdw', BeamInputWorkspace='Dbw', OutputWorkspace='w_tr')

        # Water
        SANSILLReduction(Run='027887', ProcessAs='Reference', MaskedInputWorkspace='mask',
                         AbsorberInputWorkspace='Cdw', ContainerInputWorkspace='wc',
                         BeamInputWorkspace='Dbw', TransmissionInputWorkspace='wc_tr',
                         SensitivityOutputWorkspace='sens', OutputWorkspace='water')

        # Process the dark current Cd/B4C for sample
        SANSILLReduction(Run='027885', ProcessAs='Absorber', OutputWorkspace='Cd')

        # Process the empty beam for sample
        SANSILLReduction(Run='027916', ProcessAs='Beam', BeamRadius = 1., AbsorberInputWorkspace='Cd',
                         OutputWorkspace='Db', FluxOutputWorkspace='flux')

        # Process the empty beam for sample transmission
        SANSILLReduction(Run='027858', ProcessAs='Beam', AbsorberInputWorkspace='Cd', OutputWorkspace='Dbtr')

        # Sample container transmission
        SANSILLReduction(Run='027860', ProcessAs='Transmission',
                         AbsorberInputWorkspace='Cd', BeamInputWorkspace='Dbtr', OutputWorkspace='sc_tr')

        # Sample container
        SANSILLReduction(Run='027930', ProcessAs='Container',
                         AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db',
                         TransmissionInputWorkspace='sc_tr', OutputWorkspace='sc')

        # Sample transmission
        SANSILLReduction(Run='027985', ProcessAs='Transmission',
                         AbsorberInputWorkspace='Cd', BeamInputWorkspace='Dbtr', OutputWorkspace='s_tr')

        # Sample with flux
        SANSILLReduction(Run='027925', ProcessAs='Sample',  MaskedInputWorkspace='mask',
                         AbsorberInputWorkspace='Cd', ContainerInputWorkspace='sc', BeamInputWorkspace='Db',
                         TransmissionInputWorkspace='s_tr', OutputWorkspace='sample_flux', FluxInputWorkspace='flux')

        # I(Q)
        SANSILLIntegration(InputWorkspace='sample_flux', OutputWorkspace='iq', CalculateResolution='None')
