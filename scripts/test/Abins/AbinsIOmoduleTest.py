# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from pathlib import Path
import unittest

import numpy as np
from pydantic import ValidationError
from abins import IO, test_helpers


class IOTest(unittest.TestCase):
    def setUp(self):
        from mantid.kernel import ConfigService

        self._cache_directory = Path(ConfigService.getString("defaultsave.directory"))

    def tearDown(self):
        test_helpers.remove_output_files(list_of_names=["Cars", "temphgfrt"], directory=self._cache_directory)

    def _save_stuff(self):
        saver = IO(input_filename="Cars.foo", group_name="Volksvagen", cache_directory=self._cache_directory)

        # add some attributes
        saver.add_attribute("Fuel", 100)
        saver.add_attribute("Speed", 200)

        # add some datasets
        saver.add_data("Passengers", np.array([4]))
        saver.add_data("FireExtinguishers", np.array([2]))

        # add some mode complex data sets
        wheels = [
            {"Winter": False, "Punctured": False, "Brand": "Mercedes", "Age": 2},
            {"Winter": False, "Punctured": False, "Brand": "Mercedes", "Age": 3},
            {"Winter": False, "Punctured": False, "Brand": "Mercedes", "Age": 5},
            {"Winter": False, "Punctured": True, "Brand": "Mercedes", "Age": 7},
        ]
        chairs = {"AdjustableHeadrests": True, "ExtraPadding": True}

        saver.add_data("wheels", wheels)
        saver.add_data("chairs", chairs)

        # save attributes and datasets
        saver.save()

    def _add_wrong_attribute(self):
        poor_saver = IO(input_filename="BadCars.foo", group_name="Volksvagen", cache_directory=self._cache_directory)

        with self.assertRaisesRegex(ValidationError, "Input should be an instance of int64"):
            poor_saver.add_attribute("BadPassengers", np.array([4, 5], dtype=np.int64))

    def _save_wrong_dataset(self):
        poor_saver = IO(input_filename="BadCars.foo", group_name="Volksvagen", cache_directory=self._cache_directory)
        poor_saver.add_data("BadPassengers", 4)
        self.assertRaises(TypeError, poor_saver.save)

    def _wrong_filename_type(self):
        self.assertRaisesRegex(ValidationError, "Input should be a valid string", IO, input_filename=1, group_name="goodgroup")

    def _empty_filename(self):
        self.assertRaises(ValueError, IO, input_filename="", group_name="goodgroup")

    def _wrong_groupname(self):
        self.assertRaises(ValueError, IO, input_filename="goodfile", group_name=1)

    def _wrong_file(self):
        poor_loader = IO(input_filename="bumCars", group_name="nice_group", cache_directory=self._cache_directory)
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

        self.assertEqual(
            [
                {"Winter": False, "Punctured": False, "Brand": b"Mercedes", "Age": 2},
                {"Winter": False, "Punctured": False, "Brand": b"Mercedes", "Age": 3},
                {"Winter": False, "Punctured": False, "Brand": b"Mercedes", "Age": 5},
                {"Winter": False, "Punctured": True, "Brand": b"Mercedes", "Age": 7},
            ],
            data["datasets"]["wheels"],
        )

        self.assertEqual({"AdjustableHeadrests": True, "ExtraPadding": True}, data["datasets"]["chairs"])

        self.assertRaises(ValueError, self.loader.load, list_of_datasets=["WrongDataSet"])

        self.assertRaises(ValueError, self.loader.load, list_of_datasets=1)

    def runTest(self):
        self._save_stuff()

        self._add_wrong_attribute()
        self._save_wrong_dataset()

        self.loader = IO(input_filename="Cars.foo", group_name="Volksvagen", cache_directory=self._cache_directory)

        self._wrong_filename_type()
        self._empty_filename()
        self._wrong_groupname()
        self._wrong_file()

        self._loading_attributes()
        self._loading_datasets()
        self._loading_structured_datasets()


if __name__ == "__main__":
    unittest.main()
