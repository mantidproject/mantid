# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import six
import unittest
import numpy as np
from mantid.simpleapi import logger
from AbinsModules import IOmodule, AbinsTestHelpers


class AbinsIOmoduleTest(unittest.TestCase):

    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["Cars", "temphgfrt"])

    @staticmethod
    def _save_stuff():
        saver = IOmodule(input_filename="Cars.foo", group_name="Volksvagen")

        # add some attributes
        saver.add_attribute("Fuel", 100)
        saver.add_attribute("Speed", 200)

        # add some datasets
        saver.add_data("Passengers", np.array([4]))
        saver.add_data("FireExtinguishers", np.array([2]))

        # add some mode complex data sets
        wheels = [{"Winter":   False, "Punctured": False, "Brand": "Mercedes", "Age":  2},
                  {"Winter":   False, "Punctured": False, "Brand": "Mercedes", "Age":  3},
                  {"Winter":   False, "Punctured": False, "Brand": "Mercedes", "Age":  5},
                  {"Winter":   False, "Punctured": True,  "Brand": "Mercedes", "Age":  7}]
        chairs = {"AdjustableHeadrests": True, "ExtraPadding": True}

        saver.add_data("wheels", wheels)
        saver.add_data("chairs", chairs)

        # save attributes and datasets
        saver.save()

    def _save_wrong_attribute(self):
        poor_saver = IOmodule(input_filename="BadCars.foo", group_name="Volksvagen")
        poor_saver.add_attribute("BadPassengers", np.array([4]))
        self.assertRaises(ValueError, poor_saver.save)

    def _save_wrong_dataset(self):
        poor_saver = IOmodule(input_filename="BadCars.foo", group_name="Volksvagen")
        poor_saver.add_data("BadPassengers", 4)
        self.assertRaises(ValueError, poor_saver.save)

    def _wrong_filename(self):
        self.assertRaises(ValueError, IOmodule, input_filename=1, group_name="goodgroup")

    def _wrong_groupname(self):
        self.assertRaises(ValueError, IOmodule, input_filename="goodfile", group_name=1)

    def _wrong_file(self):
        poor_loader = IOmodule(input_filename="bumCars", group_name="nice_group")
        self.assertRaises(IOError, poor_loader.load, list_of_attributes="one_attribute")

    def _loading_attributes(self):
        data = self.loader.load(list_of_attributes=["Fuel", "Speed"])
        attr_data = data["attributes"]

        self.assertEqual(100, attr_data["Fuel"])
        self.assertEqual(200, attr_data["Speed"])
        self.assertRaises(ValueError, self.loader.load, list_of_attributes=["NiceFuel"])
        self.assertRaises(ValueError, self.loader.load, list_of_attributes=1)
        self.assertRaises(ValueError, self.loader.load, list_of_attributes=[1, "Speed"])

    def _loading_datasets(self):
        data = self.loader.load(list_of_datasets=["Passengers", "FireExtinguishers"])

        self.assertEqual(np.array([4]), data["datasets"]["Passengers"])
        self.assertEqual(np.array([2]), data["datasets"]["FireExtinguishers"])
        self.assertRaises(ValueError, self.loader.load, list_of_datasets=["NicePassengers"])
        self.assertRaises(ValueError, self.loader.load, list_of_datasets=1)
        self.assertRaises(ValueError, self.loader.load, list_of_datasets=[1, "Passengers"])

    def _loading_structured_datasets(self):
        """
        Loads more complicated data from the hdf file.
        """

        data = self.loader.load(list_of_datasets=["wheels", "chairs"])

        self.assertEqual([{"Winter": False, "Punctured": False, "Brand": "Mercedes", "Age": 2},
                          {"Winter": False, "Punctured": False, "Brand": "Mercedes", "Age": 3},
                          {"Winter": False, "Punctured": False, "Brand": "Mercedes", "Age": 5},
                          {"Winter": False, "Punctured": True,  "Brand": "Mercedes", "Age":  7}],
                         data["datasets"]["wheels"])

        self.assertEqual({"AdjustableHeadrests": True, "ExtraPadding": True},
                         data["datasets"]["chairs"])

        self.assertRaises(ValueError, self.loader.load, list_of_datasets=["WrongDataSet"])

        self.assertRaises(ValueError, self.loader.load, list_of_datasets=1)

    def _py2_unicode_laundering(self):
        if six.PY2:
            unit_data_list = [u'1 Å', u'2 Å', u'3 Å']
            cleaned_data_list = IOmodule._convert_unicode_to_str(unit_data_list)
            self.assertNotIn(u'2 Å', cleaned_data_list)
            self.assertIn('2 \xc3\x85', cleaned_data_list)

            unit_data_nested = [[u'1 Å', u'2 Å'], [u'3 Å', u'4 Å']]
            cleaned_data_nested = IOmodule._convert_unicode_to_str(unit_data_nested)
            self.assertNotIn(u'3 Å', cleaned_data_nested[1])
            self.assertIn('3 \xc3\x85', cleaned_data_nested[1])

            unit_data_dict = {u'1 Å': 1, u'2 Å': 2,
                              'tens': {u'10 Å': 10, u'20 Å': 20}}
            cleaned_data_dict = IOmodule._convert_unicode_to_str(unit_data_dict)
            self.assertNotIn(u'2 Å', cleaned_data_dict)
            self.assertNotIn(u'10 Å', cleaned_data_dict['tens'])
            self.assertIn('10 \xc3\x85', cleaned_data_dict['tens'])

            for item in ('x', 1, 2.34, (1, 2)):
                self.assertEqual(IOmodule._convert_unicode_to_str(item), item)

            self.assertIsInstance(IOmodule._convert_unicode_to_str(np.ones(5)),
                                  np.ndarray)

    def runTest(self):

        self._save_stuff()

        self._save_wrong_attribute()
        self._save_wrong_dataset()

        self.loader = IOmodule(input_filename="Cars.foo", group_name="Volksvagen")

        self._wrong_filename()
        self._wrong_groupname()
        self._wrong_file()

        self._loading_attributes()
        self._loading_datasets()
        self._loading_structured_datasets()

        self._py2_unicode_laundering()

if __name__ == '__main__':
    unittest.main()
