# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

import mantid
from mantid.kernel import PropertyManagerDataService, PropertyManagerProperty
from mantid.api import Algorithm

from sans.gui_logic.presenter.property_manager_service import PropertyManagerService
from sans.state.data import get_data_builder
from sans.test_helper.test_director import TestDirector
from sans.common.enums import SANSFacility, SANSInstrument
from sans.test_helper.file_information_mock import SANSFileInformationMock


class FakeAlgorithm(Algorithm):
    def PyInit(self):
        self.declareProperty(PropertyManagerProperty("Args"))

    def PyExec(self):
        pass


def get_example_state():
    ws_name_sample = "SANS2D00022024"
    file_information = SANSFileInformationMock(instrument=SANSInstrument.SANS2D, run_number=22024)
    data_builder = get_data_builder(SANSFacility.ISIS, file_information)
    data_builder.set_sample_scatter(ws_name_sample)
    data = data_builder.build()

    # Get the sample state
    test_director = TestDirector()
    test_director.set_states(data_state=data)
    return test_director.construct()


class PropertyManagerServiceTest(unittest.TestCase):
    def test_that_add_states_to_pmds(self):
        self.assertEqual(len(PropertyManagerDataService.getObjectNames()),  0)
        states = {0: get_example_state(), 1: get_example_state()}
        pms = PropertyManagerService()
        pms.add_states_to_pmds(states)
        self.assertEqual(len(PropertyManagerDataService.getObjectNames()),  2)
        self._remove_all_property_managers()

    def test_that_removes_sans_property_managers_from_pmds(self):
        self.assertEqual(len(PropertyManagerDataService.getObjectNames()),  0)
        states = {0: get_example_state(), 1: get_example_state()}
        pms = PropertyManagerService()
        pms.add_states_to_pmds(states)
        pms.remove_sans_property_managers()
        self.assertEqual(len(PropertyManagerDataService.getObjectNames()),  0)
        self._remove_all_property_managers()

    def test_that_can_retrieve_states_from_pmds(self):
        self.assertEqual(len(PropertyManagerDataService.getObjectNames()),  0)
        states = {0: get_example_state(), 1: get_example_state()}
        pms = PropertyManagerService()
        pms.add_states_to_pmds(states)
        retrieved_states = pms.get_states_from_pmds()
        self.assertEqual(len(retrieved_states),  2)
        self.assertTrue(isinstance(retrieved_states[0], type(states[0])))
        self.assertEqual(len(PropertyManagerDataService.getObjectNames()),  2)
        self._remove_all_property_managers()

    def test_that_does_not_delete_pms_which_are_not_sans(self):
        property_manager = self._get_property_manager()
        PropertyManagerDataService.addOrReplace("test", property_manager)
        pms = PropertyManagerService()
        pms.remove_sans_property_managers()
        self.assertEqual(len(PropertyManagerDataService.getObjectNames()),  1)
        self._remove_all_property_managers()

    def test_that_it_adds_nothing_when_empty_list_is_passed_in(self):
        pms = PropertyManagerService()
        number_of_elements_on_pmds_after = len(PropertyManagerDataService.getObjectNames())
        pms.add_states_to_pmds({})
        self.assertEqual(number_of_elements_on_pmds_after,  0)
        self._remove_all_property_managers()

    def test_that_it_returns_empty_list_if_no_states_have_been_added(self):
        pms = PropertyManagerService()
        states = pms.get_states_from_pmds()
        self.assertEqual(states,  [])
        self._remove_all_property_managers()

    @staticmethod
    def _remove_all_property_managers():
        for element in PropertyManagerDataService.getObjectNames():
            PropertyManagerDataService.remove(element)

    @staticmethod
    def _get_property_manager():
        alg = FakeAlgorithm()
        alg.initialize()
        alg.setProperty("Args", {"test": 1})
        return alg.getProperty("Args").value


if __name__ == '__main__':
    unittest.main()
