# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import GroupWorkspaces, IndirectILLEnergyTransfer, config


class ILLIndirectEnergyTransferBATSTest(systemtesting.MantidSystemTest):

    # cache default instrument and datadirs
    facility = config['default.facility']
    instrument = config['default.instrument']
    datadirs = config['datasearch.directories']

    def __init__(self):
        super(ILLIndirectEnergyTransferBATSTest, self).__init__()
        self.setUp()

    def setUp(self):
        # these must be set, so the required files
        # without instrument name can be retrieved
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

        self.tolerance = 1e-2
        self.tolerance_rel_err = True
        # this fails the test every time a new instrument parameter is added
        # parameters file evolves quite often, so this is not checked
        self.disableChecking = ['Instrument']

    def cleanup(self):
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.datadirs

    def runTest(self):

        center, epp = IndirectILLEnergyTransfer(Run='215962', PulseChopper='34', GroupDetectors=False)

        offset_p100 = IndirectILLEnergyTransfer(Run='215967', InputElasticChannelWorkspace=epp, PulseChopper='34',
                                                GroupDetectors=False)

        offset_m275 = IndirectILLEnergyTransfer(Run='215968', InputElasticChannelWorkspace=epp, PulseChopper='34',
                                                GroupDetectors=False)

        GroupWorkspaces(InputWorkspaces=[center, offset_p100, offset_m275], OutputWorkspace='group')

    def validate(self):

        return ['group', 'ILLIN16B_BATS.nxs']


class ILLIndirectEnergyTransferEquatorialTest(systemtesting.MantidSystemTest):
    # cache default instrument and datadirs
    facility = config['default.facility']
    instrument = config['default.instrument']
    datadirs = config['datasearch.directories']

    def __init__(self):
        super(ILLIndirectEnergyTransferEquatorialTest, self).__init__()
        self.setUp()

    def setUp(self):
        # these must be set, so the required files
        # without instrument name can be retrieved
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

        self.tolerance = 1e-2
        self.tolerance_rel_err = True
        # this fails the test every time a new instrument parameter is added
        # parameters file evolves quite often, so this is not checked
        self.disableChecking = ['Instrument']

    def cleanup(self):
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.datadirs

    @staticmethod
    def runTest():
        center, epp = IndirectILLEnergyTransfer(Run='318308',
                                                PulseChopper='34',
                                                ElasticPeakFitting='FitEquatorialOnly',
                                                GroupDetectors=False)

        offset = IndirectILLEnergyTransfer(Run='315515',
                                           PulseChopper='34',
                                           GroupDetectors=False,
                                           InputElasticChannelWorkspace=epp)

        GroupWorkspaces(InputWorkspaces=[center, offset, epp], OutputWorkspace='grouped')

    @staticmethod
    def validate():
        return ['grouped', 'ILLIN16B_BATS_equatorial.nxs']
