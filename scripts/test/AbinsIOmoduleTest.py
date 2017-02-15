from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import numpy as np
from AbinsModules import IOmodule, AbinsTestHelpers


def old_modules():
    """" Check if there are proper versions of  Python and numpy."""
    is_python_old = AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsIOmoduleTest because Python is too old.")

    is_numpy_old = AbinsTestHelpers.is_numpy_valid(np.__version__)
    if is_numpy_old:
        logger.warning("Skipping AbinsIOmoduleTest because numpy is too old.")

    return is_python_old or is_numpy_old


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied function returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """

    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls

    return decorate


@skip_if(old_modules)
class AbinsIOmoduleTest(unittest.TestCase):

    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["Cars", "temphgfrt"])

    # noinspection PyMethodMayBeStatic
    def _save_stuff(self):
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

if __name__ == '__main__':
    unittest.main()
