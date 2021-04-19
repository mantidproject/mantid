# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from PelicanReduction import PelicanReduction


class PelicanReductionSOFQWTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        PelicanReduction('44464', OutputWorkspace='test', EnergyTransfer='-2,0.05,2', MomentumTransfer='0,0.05,2',
                         Processing='SOFQW1-Centre', ConfigurationFile='pelican_doctest.ini')

    def validate(self):
        self.disableChecking.append('Instrument')
        return 'test_qw1_2D', 'PelicanReductionExampleSOFQW.nxs'


class PelicanReductionNXSPETest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        PelicanReduction('44464', OutputWorkspace='test', EnergyTransfer='-2,0.05,2', MomentumTransfer='0,0.05,2',
                         Processing='NXSPE', ConfigurationFile='pelican_doctest.ini')

    def validate(self):
        self.disableChecking.append('Instrument')
        return 'test_spe_2D', 'PelicanReductionExampleNXSPE.nxs'
