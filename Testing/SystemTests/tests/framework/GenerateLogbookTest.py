# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import GenerateLogbook, config, mtd
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
        test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        d11_dir = 'ILL/D11'
        if 'ILL' in test_data_dir:
            d11_dir = 'D11'
        self._data_directory = os.path.abspath(os.path.join(test_data_dir,  d11_dir))

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
        test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        d11_dir = 'ILL/D11'
        if 'ILL' in test_data_dir:
            d11_dir = 'D11'
        self._data_directory = os.path.abspath(os.path.join(test_data_dir,  d11_dir))

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


class D22_GenerateLogbook_Test(systemtesting.MantidSystemTest):
    """
    Tests generating logbook for D22 data.
    """

    _data_directory = None

    def __init__(self):
        super(D22_GenerateLogbook_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D22'
        config['logging.loggers.root.level'] = 'Warning'

        data_dirs = config['datasearch.directories'].split(';')
        test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        d22_dir = 'ILL/D22'
        if 'ILL' in test_data_dir:
            d22_dir = 'D22'
        self._data_directory = os.path.abspath(os.path.join(test_data_dir,  d22_dir))

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['d22_logbook', 'D22_Logbook_Reference.nxs']

    def runTest(self):
        GenerateLogbook(Directory=self._data_directory, OutputWorkspace='d22_logbook',
                        Facility='ILL', Instrument='D22', NumorRange='354717,354718',
                        OptionalHeaders='all')


class D22B_GenerateLogbook_Test(systemtesting.MantidSystemTest):
    """
    Tests generating logbook for D22B data.
    """

    _data_directory = None

    def __init__(self):
        super(D22B_GenerateLogbook_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D22B'
        config['logging.loggers.root.level'] = 'Warning'

        data_dirs = config['datasearch.directories'].split(';')
        test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        d22_dir = 'ILL/D22'
        if 'ILL' in test_data_dir:
            d22_dir = 'D22'
        self._data_directory = os.path.abspath(os.path.join(test_data_dir,  d22_dir))

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['d22b_logbook', 'D22b_Logbook_Reference.nxs']

    def runTest(self):
        GenerateLogbook(Directory=self._data_directory, OutputWorkspace='d22b_logbook',
                        Facility='ILL', Instrument='D22B', NumorRange='398672,398673',
                        OptionalHeaders='all')


class IN4_GenerateLogbook_Test(systemtesting.MantidSystemTest):
    """
    Tests generating logbook for IN4 data.
    """

    _data_directory = None

    def __init__(self):
        super(IN4_GenerateLogbook_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN4'
        config['logging.loggers.root.level'] = 'Warning'

        data_dirs = config['datasearch.directories'].split(';')
        test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        in4_dir = 'ILL/IN4'
        if 'ILL' in test_data_dir:
            in4_dir = 'IN4'
        self._data_directory = os.path.abspath(os.path.join(test_data_dir,  in4_dir))

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['in4_logbook', 'IN4_Logbook_Reference.nxs']

    def runTest(self):
        GenerateLogbook(Directory=self._data_directory, OutputWorkspace='in4_logbook',
                        Facility='ILL', Instrument='IN4', NumorRange='092375,092376',
                        OptionalHeaders='all')


class IN5_GenerateLogbook_Test(systemtesting.MantidSystemTest):
    """
    Tests generating logbook for IN5 data.
    """

    _data_directory = None

    def __init__(self):
        super(IN5_GenerateLogbook_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN5'
        config['logging.loggers.root.level'] = 'Warning'

        data_dirs = config['datasearch.directories'].split(';')
        test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        in5_dir = 'ILL/IN5'
        if 'ILL' in test_data_dir:
            in5_dir = 'IN5'
        self._data_directory = os.path.abspath(os.path.join(test_data_dir,  in5_dir))

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['in5_logbook', 'IN5_Logbook_Reference.nxs']

    def runTest(self):
        GenerateLogbook(Directory=self._data_directory, OutputWorkspace='in5_logbook',
                        Facility='ILL', Instrument='IN5', NumorRange='199728,199729',
                        OptionalHeaders='all')


class IN6_GenerateLogbook_Test(systemtesting.MantidSystemTest):
    """
    Tests generating logbook for IN6 data.
    """

    _data_directory = None

    def __init__(self):
        super(IN6_GenerateLogbook_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN6'
        config['logging.loggers.root.level'] = 'Warning'

        data_dirs = config['datasearch.directories'].split(';')
        test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        in6_dir = 'ILL/IN6'
        if 'ILL' in test_data_dir:
            in6_dir = 'IN6'
        self._data_directory = os.path.abspath(os.path.join(test_data_dir,  in6_dir))

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['in6_logbook', 'IN6_Logbook_Reference.nxs']

    def runTest(self):
        GenerateLogbook(Directory=self._data_directory, OutputWorkspace='in6_logbook',
                        Facility='ILL', Instrument='IN6', NumorRange='224436,224437',
                        OptionalHeaders='all')


class D33_GenerateLogbook_Test(systemtesting.MantidSystemTest):
    """
    Tests generating logbook for D33 data.
    """

    _data_directory = None

    def __init__(self):
        super(D33_GenerateLogbook_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D33'
        config['logging.loggers.root.level'] = 'Warning'

        data_dirs = config['datasearch.directories'].split(';')
        test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        d33_dir = 'ILL/D33'
        if 'ILL' in test_data_dir:
            d33_dir = 'D33'
        self._data_directory = os.path.abspath(os.path.join(test_data_dir,  d33_dir))

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['d33_logbook', 'D33_Logbook_Reference.nxs']

    def runTest(self):
        GenerateLogbook(Directory=self._data_directory, OutputWorkspace='d33_logbook',
                        Facility='ILL', Instrument='D33', NumorRange='162689,162690',
                        OptionalHeaders='all')


class D16_GenerateLogbook_Test(systemtesting.MantidSystemTest):
    """
    Tests generating logbook for D16 data.
    """

    _data_directory = None

    def __init__(self):
        super(D16_GenerateLogbook_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D16'
        config['logging.loggers.root.level'] = 'Warning'

        data_dirs = config['datasearch.directories'].split(';')
        test_data_dir = [p for p in data_dirs if 'SystemTest' in p][0]
        d33_dir = 'ILL/D16'
        if 'ILL' in test_data_dir:
            d33_dir = 'D16'
        self._data_directory = os.path.abspath(os.path.join(test_data_dir,  d33_dir))

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['d16_logbook', 'D16_Logbook_Reference.nxs']

    def runTest(self):
        GenerateLogbook(Directory=self._data_directory, OutputWorkspace='d16_logbook',
                        Facility='ILL', Instrument='D16', NumorRange='000245,000246',
                        OptionalHeaders='all')
