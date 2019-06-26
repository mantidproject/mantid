# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (CreateWorkspace, ReflectometryReductionOneAuto,
                              ReflectometryReductionOneLiveData)
from testhelpers import (assertRaisesNothing, create_algorithm)


class GetFakeLiveInstrumentValue(DataProcessorAlgorithm):
    def PyInit(self):
        self.declareProperty(name='Instrument', defaultValue='', direction=Direction.Input,
                             validator=StringListValidator(['CRISP', 'INTER', 'OFFSPEC', 'POLREF', 'SURF']),
                             doc='Instrument to find live value for.')

        self.declareProperty(name='PropertyType', defaultValue='Run', direction=Direction.Input,
                             validator=StringListValidator(['Run', 'Block']),
                             doc='The type of property to find')

        self.declareProperty(name='PropertyName', defaultValue='TITLE', direction=Direction.Input,
                             validator=StringMandatoryValidator(),
                             doc='Name of value to find.')

        self.declareProperty(name='Value', defaultValue='', direction=Direction.Output,
                             doc='The live value from the instrument, or an empty string if not found')

    def PyExec(self):
        propertyName = self.getProperty('PropertyName').value
        if propertyName == 'Theta':
            self.setProperty('Value', '0.5')
        elif propertyName == 'S1VG':
            self.setProperty('Value', '1.001')
        elif propertyName == 'S2VG':
            self.setProperty('Value', '0.5')
        else:
            raise RuntimeError('Requested live value for unexpected property name ' + propertyName)

AlgorithmFactory.subscribe(GetFakeLiveInstrumentValue)


class ReflectometryReductionOneLiveDataTest(unittest.TestCase):
    def setUp(self):
        self._setup_environment()
        self._setup_workspaces()

    def tearDown(self):
        self._reset_workspaces()
        self._reset_environment()

    def test_basic_reduction_works(self):
        workspace = self._run_algorithm_with_defaults()
        self.assertEqual(workspace.dataX(0).size, 55)
        self._assert_delta(workspace.dataX(0)[0],  0.006462)
        self._assert_delta(workspace.dataX(0)[33], 0.027376)
        self._assert_delta(workspace.dataX(0)[54], 0.066421)
        self._assert_delta(workspace.dataY(0)[4],  0.043630)
        self._assert_delta(workspace.dataY(0)[33], 0.000029)
        self._assert_delta(workspace.dataY(0)[53], 0.0)

    def test_missing_inputs(self):
        self.assertRaises(RuntimeError,
                          ReflectometryReductionOneLiveData)

    def test_invalid_input_workspace(self):
        self.assertRaises(ValueError,
                          ReflectometryReductionOneLiveData,
                          InputWorkspace='bad')

    def test_invalid_output_workspace(self):
        self.assertRaises(RuntimeError,
                          ReflectometryReductionOneLiveData,
                          InputWorkspace=self.__class__._input_ws,
                          OutputWorkspace='')

    def test_invalid_property(self):
        self.assertRaises(RuntimeError,
                          ReflectometryReductionOneLiveData,
                          InputWorkspace=self.__class__._input_ws,
                          OutputWorkspace='output',
                          Instrument='INTER',
                          GetLiveValueAlgorithm='GetFakeLiveInstrumentValue',
                          BadProperty='badvalue')

    def test_all_child_properties_are_present(self):
        # Get the properties for the child algorithm, apart from a list of known
        # exclusions
        child_alg = create_algorithm('ReflectometryReductionOneAuto')
        excluded = ['ThetaIn', 'ThetaLogName', 'Diagnostics',
                    'OutputWorkspaceBinned', 'OutputWorkspaceWavelength']
        child_props = set([prop.name for prop in child_alg.getProperties() if prop.name not in excluded])
        # Check the child properties exist in the parent algorithm
        actual_alg = create_algorithm('ReflectometryReductionOneLiveData')
        actual_props = set([prop.name for prop in actual_alg.getProperties()])
        if not child_props.issubset(actual_props):
            assert False, "The following child properties are not implemented in the parent algorithm:\n" \
            + str(child_props.difference(actual_props))

    def test_instrument_was_set_on_output_workspace(self):
        workspace = self._run_algorithm_with_defaults()
        self.assertEqual(self.__class__._input_ws.getInstrument().getName(), '')
        self.assertEqual(workspace.getInstrument().getName(), self._instrument_name)

    def test_sample_log_values_were_set_on_output_workspace(self):
        workspace = self._run_algorithm_with_defaults()

        names = ['Theta', 'S1VG', 'S2VG']
        values = [0.5, 1.001, 0.5]
        units = ['deg', 'm', 'm']
        logs = workspace.getRun().getProperties()
        matched_names = list()
        for log in logs:
            if log.name in names:
                matched_names.append(log.name)
                idx = names.index(log.name)
                self.assertEqual(log.value, values[idx])
                self.assertEqual(log.units, units[idx])
        self.assertEqual(sorted(matched_names), sorted(names))

    def test_slits_gaps_are_set_up_on_output_workspace(self):
        workspace = self._run_algorithm_with_defaults()
        slit1vg = workspace.getInstrument().getComponentByName('slit1').getNumberParameter('vertical gap')
        slit2vg = workspace.getInstrument().getComponentByName('slit2').getNumberParameter('vertical gap')
        self.assertEqual(slit1vg[0], 1.001)
        self.assertEqual(slit2vg[0], 0.5)

    def _setup_environment(self):
        self._old_facility = config['default.facility']
        if self._old_facility.strip() == '':
            self._old_facility = 'TEST_LIVE'
        config.setFacility('ISIS')

        self._instrument_name = 'INTER'
        self._old_instrument = config['default.instrument']
        config['default.instrument'] = self._instrument_name

    def _reset_environment(self):
        config.setFacility(self._old_facility)
        config['default.instrument'] = self._old_instrument

    def _setup_workspaces(self):
        self.__class__._input_ws = self._create_test_workspace()
        self._default_args = {
            'InputWorkspace': self.__class__._input_ws,
            'OutputWorkspace': 'output',
            'Instrument': 'INTER',
            'GetLiveValueAlgorithm': 'GetFakeLiveInstrumentValue'
        }

    def _reset_workspaces(self):
        mtd.clear()

    def _create_test_workspace(self):
        """Create a test workspace with reflectometry data but no instrument or sample logs"""
        nSpec = 4
        x = range(5, 100000, 1000)
        y = [6,1,0,0,0,3,22,40,59,135,241,406,645,932,1096,1348,1559,1640,1790,1933,2072,2349,2629,2838,2996,3141,3170,3250,3260,3359,3460,3316,3335,3470,3384,3367,3412,3431,3541,3569,3592,3526,3619,3757,3840,4031,4076,4342,4559,4998,5414,6260,8358,11083,11987,10507,9165,8226,6979,6068,5295,4587,4005,3489,3018,2728,2398,2220,1963,1753,1516,1384,1355,1192,1153,1002,944,926,788,778,687,618,586,531,456,305,77,0,0,0,0,0,0,0,0,0,0,0,0]
        monitor_y = [14,7,583,97285,1474787,3362154,4790669,6768912,9359711,10443338,10478945,9957954,9387695,8660614,8354808,8036977,7649065,7067769,6275327,5558998,4898033,4307711,3776162,3302736,2895372,2539087,2226028,1956162,1721463,1518854,1343139,1193432,1057989,942001,839505,748921,669257,598804,537145,482685,433356,390183,352178,318584,288155,261372,237360,216296,197369,180357,164538,151010,138316,126428,116943,107688,98584,90757,81054,59764,21151,372,18,4,3,1,3,3,0,3,1,0,2,1,1,1,3,1,1,0,1,0,1,1,2,1,0,1,2,2,0,0,0,0,1,0,1,2,0]
        dataX = list()
        dataY = list()
        for i in range(0, nSpec - 1):
            dataX += x
            dataY += monitor_y
        dataX += x
        dataY += y

        CreateWorkspace(NSpec=nSpec, UnitX='TOF', DataX=dataX, DataY=dataY, OutputWorkspace='input_ws')
        return mtd['input_ws']

    def _run_algorithm_with_defaults(self):
        alg = create_algorithm('ReflectometryReductionOneLiveData', **self._default_args)
        assertRaisesNothing(self, alg.execute)
        return mtd['output']

    def _assert_delta(self, value1, value2):
        self.assertEqual(round(value1, 6), round(value2, 6))


if __name__ == '__main__':
    unittest.main()
