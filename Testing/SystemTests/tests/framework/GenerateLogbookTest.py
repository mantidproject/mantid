# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import GenerateLogbook, SaveNexusProcessed, \
    LoadNexusProcessed, config, mtd
import systemtesting

import os


class D11_GenerateLogbook_Test(systemtesting.MantidSystemTest):
    """
    Tests generating logbook for D11 data.
    """

    _data_directory = None

    def __init__(self):
        super(D11_GenerateLogbook_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'
        config['logging.loggers.root.level'] = 'Warning'

        data_dirs = config['datasearch.directories'].split(';')
        unit_test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        d11_dir = 'ILL/D11'
        if 'ILL' in unit_test_data_dir:
            d11_dir = 'D11'
        self._data_directory = os.path.abspath(os.path.join(unit_test_data_dir,  d11_dir))

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['d11_logbook', 'D11_Logbook_Reference.nxs']

    def runTest(self):
        GenerateLogbook(Directory=self._data_directory, OutputWorkspace='d11_logbook',
                        Facility='ILL', Instrument='D11', NumorRange='017038,017039')


class D11B_GenerateLogbook_Test(systemtesting.MantidSystemTest):
    """
    Tests generating logbook for D11B data.
    """

    _data_directory = None

    def __init__(self):
        super(D11B_GenerateLogbook_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11B'
        config['logging.loggers.root.level'] = 'Warning'

        data_dirs = config['datasearch.directories'].split(';')
        unit_test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        d11_dir = 'ILL/D11'
        if 'ILL' in unit_test_data_dir:
            d11_dir = 'D11'
        self._data_directory = os.path.abspath(os.path.join(unit_test_data_dir,  d11_dir))

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['d11b_logbook', 'D11b_Logbook_Reference.nxs']

    def runTest(self):
        GenerateLogbook(Directory=self._data_directory, OutputWorkspace='d11b_logbook',
                        Facility='ILL', Instrument='D11B', NumorRange='000101,000102',
                        OptionalHeaders='all')



